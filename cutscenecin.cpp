/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include "cutscene.h"
#include "decoder.h"
#include "game.h"
#include "file.h"
#include "sound.h"
#include "render.h"

enum {
	kFrameBuffersCount = 3,
	kSubtitleMessagesCount = 16,
	kCutsceneFrameDelay = 1000 / 12,
	kCutsceneDisplayWidth = 320,
	kCutsceneDisplayHeight = 200,
};

struct CinFileHeader {
	uint32_t mark; /* 0x55AA0000 */
	uint32_t videoFrameSize;
	uint16_t videoFrameWidth;
	uint16_t videoFrameHeight;
	uint32_t soundFreq;
	uint8_t soundBits;
	uint8_t soundStereo;
	uint16_t soundFrameSize;
};

struct CinFrameHeader {
	uint8_t videoFrameType;
	uint8_t soundFrameType;
	int16_t palColorsCount;
	uint32_t videoFrameSize;
	uint32_t soundFrameSize;
	uint32_t mark; /* 0xAA55AA55 */
};

struct CutscenePlayer_Cin: CutscenePlayer {

	File *_fp;
	uint8_t _palette[256 * 3];
	CinFileHeader _fileHdr;
	CinFrameHeader _frameHdr;
	uint8_t *_frameBuffers[kFrameBuffersCount];
	uint8_t *_frameReadBuffer;
	int _frameCounter;
	int _duration;
	uint8_t *_soundReadBuffer;
	struct {
		const char *data;
		int duration;
		int linesCount;
	} _msgs[kSubtitleMessagesCount];
	int _msgsCount;
	uint32_t _updateTicks;
	uint32_t _frameTicks;

	CutscenePlayer_Cin();
	virtual ~CutscenePlayer_Cin();

	bool readFileHeader(CinFileHeader *hdr);
	bool readFrameHeader(CinFrameHeader *hdr);
	void setPaletteColor(int index, uint8_t r, uint8_t g, uint8_t b);
	void updatePalette(int palType, int colorsCount, const uint8_t *p);
	void decodeImage(const uint8_t *frameData);
	void updateMessages();

	virtual bool load(int num);
	virtual void unload();
	bool play();
	void drawFrame();
	virtual bool update(uint32_t ticks);
};

CutscenePlayer_Cin::CutscenePlayer_Cin() {
	_updateTicks = 0;
	_frameTicks = 0;
}

CutscenePlayer_Cin::~CutscenePlayer_Cin() {
}

bool CutscenePlayer_Cin::readFileHeader(CinFileHeader *hdr) {
	hdr->mark = fileReadUint32LE(_fp);
	hdr->videoFrameSize = fileReadUint32LE(_fp);
	hdr->videoFrameWidth = fileReadUint16LE(_fp);
	hdr->videoFrameHeight = fileReadUint16LE(_fp);
	hdr->soundFreq = fileReadUint32LE(_fp);
	hdr->soundBits = fileReadByte(_fp);
	hdr->soundStereo = fileReadByte(_fp);
	hdr->soundFrameSize = fileReadUint16LE(_fp);
	return !fileEof(_fp) && (hdr->mark >> 16) == 0x55AA;
}

bool CutscenePlayer_Cin::readFrameHeader(CinFrameHeader *hdr) {
	hdr->videoFrameType = fileReadByte(_fp);
	hdr->soundFrameType = fileReadByte(_fp);
	hdr->palColorsCount = fileReadUint16LE(_fp);
	hdr->videoFrameSize = fileReadUint32LE(_fp);
	hdr->soundFrameSize = fileReadUint32LE(_fp);
	hdr->mark = fileReadUint32LE(_fp);
	return !fileEof(_fp) && hdr->mark == 0xAA55AA55;
}

void CutscenePlayer_Cin::setPaletteColor(int index, uint8_t r, uint8_t g, uint8_t b) {
	assert(index >= 0 && index < 256);
	uint8_t *p = &_palette[index * 3];
	*p++ = r;
	*p++ = g;
	*p++ = b;
}

void CutscenePlayer_Cin::updatePalette(int palType, int colorsCount, const uint8_t *p) {
	if (colorsCount != 0) {
		if (palType == 0) {
			for (int i = 0; i < colorsCount; ++i) {
				setPaletteColor(i, p[2], p[1], p[0]);
				p += 3;
			}
		} else {
			for (int i = 0; i < colorsCount; ++i) {
				setPaletteColor(p[0], p[3], p[2], p[1]);
				p += 4;
			}
		}
	}
}

static int decodeHuffman(const uint8_t *src, int srcSize, uint8_t *dst) {
	const uint8_t *dstStart = dst;
	const uint8_t *srcEnd = src + srcSize;
	uint8_t buf[15];
	memcpy(buf, src, 15); src += 15;
	while (src < srcEnd) {
		uint8_t code = *src++;
		if ((code & 0xF0) == 0xF0) {
			const uint8_t data = code << 4;
			code = *src++;
			*dst++ = data + (code >> 4);
		} else {
			*dst++ = buf[code >> 4];
		}
		if ((code &= 0xF) == 0xF) {
			if (src < srcEnd) {
				*dst++ = *src++;
			}
		} else {
			code = buf[code];
			*dst++ = code;
		}
	}
	return dst - dstStart;
}

static void decodeRLE(const uint8_t *src, int srcSize, uint8_t *dst) {
	const uint8_t *srcEnd = src + srcSize;
	while (src < srcEnd) {
		int len;
		const int code = *src++;
		if (code & 0x80) {
			len = code - 0x7F;
			memset(dst, *src++, len);
		} else {
			len = code + 1;
			memcpy(dst, src, len);
			src += len;
		}
		dst += len;
	}
	assert(src == srcEnd);
}

static void decodeADD(const uint8_t *src, int srcSize, uint8_t *dst) {
	for (int i = 0; i < srcSize; ++i) {
		dst[i] += src[i];
	}
}

void CutscenePlayer_Cin::decodeImage(const uint8_t *frameData) {
	int frameSize;
	if (_frameHdr.videoFrameType != 1) {
		switch (_frameHdr.videoFrameType) {
		case 9:
			decodeRLE(frameData, _frameHdr.videoFrameSize, _frameBuffers[0]);
			break;
		case 34:
			decodeRLE(frameData, _frameHdr.videoFrameSize, _frameBuffers[0]);
			decodeADD(_frameBuffers[1], _fileHdr.videoFrameSize, _frameBuffers[0]);
			break;
		case 35:
			decodeHuffman(frameData, _frameHdr.videoFrameSize, _frameBuffers[2]);
			decodeRLE(_frameBuffers[2], _frameHdr.videoFrameSize, _frameBuffers[0]);
			break;
		case 36:
			frameSize = decodeHuffman(frameData, _frameHdr.videoFrameSize, _frameBuffers[2]);
			decodeRLE(_frameBuffers[2], frameSize, _frameBuffers[0]);
			decodeADD(_frameBuffers[1], _fileHdr.videoFrameSize, _frameBuffers[0]);
			break;
		case 37:
			decodeHuffman(frameData, _frameHdr.videoFrameSize, _frameBuffers[0]);
			break;
		case 38:
			decodeLZSS(frameData, _frameBuffers[0], _fileHdr.videoFrameSize);
			break;
		case 39:
			decodeLZSS(frameData, _frameBuffers[0], _fileHdr.videoFrameSize);
			decodeADD(_frameBuffers[1], _fileHdr.videoFrameSize, _frameBuffers[0]);
			break;
		default:
			warning("CutscenePlayer_Cin::decodeImage() Unhandled video frame type 0x%X", _frameHdr.videoFrameType);
			break;
		}
	}
	memcpy(_frameBuffers[1], _frameBuffers[0], _fileHdr.videoFrameSize);
}

static int _drawSubCharRectHeight;

static void drawSubChar(DrawBuffer *buf, int x, int y, int w, int h, const uint8_t *src) {
	uint8_t *dst = buf->ptr + (_drawSubCharRectHeight - y) * buf->pitch + x;
	src += h * w;
	while (h--) {
		src -= w;
		for (int i = 0; i < w; ++i) {
			if (src[i] != 0) {
				dst[i] = src[i];
			}
		}
		dst += buf->pitch;
	}
}

static int countMessageLines(const char *data) {
	const char *sep = strchr(data, '@');
	int linesCount = 1;
	for (const char *p = data; *p && (p = strchr(p, '|')) != 0 && (!sep || p < sep); ++p) {
		++linesCount;
	}
	return linesCount;
}

void CutscenePlayer_Cin::updateMessages() {
	const int count = _game->_cutsceneMessagesCount;
	for (int i = 0; i < count; ++i) {
		GamePlayerMessage *msg = &_game->_cutsceneMessagesTable[i];
		if (msg->desc.frameSync == _frameCounter) {
			if (msg->crc != 0) {
				_snd->playVoice(msg->objKey, msg->crc);
			}
			if (_game->_params.subtitles) {
				const char *p = (const char *)msg->desc.data;
				assert(p);
				_msgs[0].data = p;
				_msgs[0].linesCount = countMessageLines(p);
				_msgsCount = 1;
				while (*p && (p = strchr(p, '@')) != 0) {
					++p;
					assert(_msgsCount < kSubtitleMessagesCount);
					_msgs[_msgsCount].data = p;
					_msgs[_msgsCount].linesCount = countMessageLines(p);
					++_msgsCount;
				}
				for (int i = 0; i < _msgsCount; ++i) {
					_msgs[i].duration = msg->desc.duration / _msgsCount;
				}
				const int font = (msg->desc.font & 31);
				if (font != kFontNameCineTypo) {
					warning("Unhandled font %d for cutscene subtitles", font);
				}
			}
		}
	}
	for (int i = 0; i < _msgsCount; ++i) {
		if (_msgs[i].duration != 0) {
			_game->_drawCharBuf.ptr = _frameBuffers[0];
			_game->_drawCharBuf.w = _game->_drawCharBuf.pitch = _fileHdr.videoFrameWidth;
			_game->_drawCharBuf.h = _fileHdr.videoFrameHeight;
			_game->_drawCharBuf.draw = drawSubChar;
			_drawSubCharRectHeight = _msgs[i].linesCount * _game->_fontsTable[kFontNameCineTypo].h - 12;
			const char *str = _msgs[i].data;
			int y = 0;
			while (1) {
				int w, h;
				const int offset = _game->getStringRect(str, kFontNameCineTypo, &w, &h);
				assert(w < _fileHdr.videoFrameWidth && h < _fileHdr.videoFrameHeight);
				const int x = (_fileHdr.videoFrameWidth - w) / 2;
				_game->drawString(x, y, str, kFontNameCineTypo, 0);
				if (str[offset] == 0 || str[offset] == '@') {
					break;
				}
				y += h;
				str += offset + 1; // skip new line character
			}
			--_msgs[i].duration;
			if (_msgs[i].duration == 0 && i == _msgsCount - 1) {
				_msgsCount = 0;
			}
			break;
		}
	}
}

static const char *_scenesTable[kCutsceneScenesCount] = {
	// 0
	"mor_arak.cin",
	"mor_bal.cin",
	"mor_expl.cin",
	"mor_elec.cin",
	// 4
	"mor_lasr.cin",
	"mor_depr.cin",
	"mor_plas.cin",
	"mor_mc.cin",
	// 8
	"mor_rray.cin",
	"boum2.cin",
	"mor_irra.cin",
	"mor_tent.cin",
	// 12
	"gameover.cin",
	"intro.cin",
	"evas.cin",
	"evbm.cin",
	// 16
	"prof.cin",
	"dest_a.cin",
	"excav.cin",
	"expvb.cin",
	// 20
	"ageer.cin",
	"anciens.cin",
	"smorph.cin",
	"aspi.cin",
	// 24
	"bup.cin",
	"arbu.cin",
	"cerv.cin",
	"evami.cin",
	// 28
	"desabomb.cin",
	"intro2.cin",
	"baston.cin",
	"fuite.cin",
	// 32
	"final.cin",
	"boum.cin",
	"mor_sumo.cin",
	"hankcle.cin",
	// 36
	"saracle.cin",
	"title.cin",
	"mor_tver.cin",
	"logo.cin",
	// 40
	"mor_vr.cin",
	"mor_glue.cin",
	"mor_gol.cin",
	"congrat.cin",
	// 44
	"fade1.cin",
	"fade2.cin",
	"pak.cin",
	"ealogo.cin",
	// 48
	"mgm.cin",
	"expsha.cin",
	"final2.cin",
	"bassin.cin",
	// 52
	"fuite2.cin",
	"gendeb.cin"
};

static const struct {
	const char *name;
	uint8_t duration;
} _scenesTable_demo[] = {
	// 37
	{ "ddtitle.cin", 5 },
	{ 0, 0 },
	{ "ddlogo.cin", 6 },
	{ 0, 0 },
	// 41
	{ "ddgameov.cin", 4 },
	{ 0, 0 },
	{ "ddcongr.cin", 12 },
	{ "ddtitle.cin", 1 },
	// 45
	{ "ddtitle.cin", 1 },
	{ 0, 0 },
	{ "ddealogo.cin", 7 }
};

static const struct {
	const char *name;
	int num;
} _scenesTable_gr[] = {
	{ "gr_expl.cin", 2 },
	{ "gr_lasr.cin", 4 },
	{ "gr_mc.cin", 7 },
	{ "gr_int2.cin", 29 },
	{ "gr_sumo.cin", 34 }
};

bool CutscenePlayer_Cin::load(int num) {
	debug(kDebug_CUTSCENE, "CutscenePlayer_Cin::load() num %d", num);
	if (num == 44) {
		setPaletteColor(1, 255, 255, 255);
	}
	_game->getCutsceneMessages(num);
	const char *name = _scenesTable[num];
	_duration = 0;
	if (g_isDemo) {
		if ((num >= 0 && num <= 8) || num == 12) {
			num = 4;
		} else if (num >= 37 && num <= 47) {
			num -= 37;
		} else {
			return false;
		}
		name = _scenesTable_demo[num].name;
		if (!name) {
			return false;
		}
		_duration = _scenesTable_demo[num].duration * 1000 / kCutsceneFrameDelay;
	} else {
		if (fileLanguage() == kFileLanguage_GR) {
			for (int i = 0; i < ARRAYSIZE(_scenesTable_gr); ++i) {
				if (_scenesTable_gr[i].num == num) {
					name = _scenesTable_gr[i].name;
					debug(kDebug_CUTSCENE, "GR cutscene '%s'", name);
					break;
				}
			}
		}
		// look for 'b' prefixed videos (higher bitrate)
		static char bname[32];
		snprintf(bname, sizeof(bname), "b%s", name);
		if (fileExists(bname, kFileType_DATA)) {
			name = bname;
			debug(kDebug_CUTSCENE, "Found '%s'", name);
		}
	}
	_fp = fileOpen(name, 0, kFileType_DATA);
	if (!readFileHeader(&_fileHdr)) {
		warning("CutscenePlayer_Cin::load() Invalid header");
		fileClose(_fp); _fp = 0;
		return false;
	}
	if (!g_isDemo && _fileHdr.soundFrameSize != 0 && (_fileHdr.soundStereo != 0 || _fileHdr.soundBits != 16 || _fileHdr.soundFreq != 22050)) {
		warning("CutscenePlayer_Cin::load() Unsupported sound format, stereo %d bits %d freq %d", _fileHdr.soundStereo, _fileHdr.soundBits, _fileHdr.soundFreq);
		fileClose(_fp); _fp = 0;
		return false;
	}
	for (int i = 0; i < kFrameBuffersCount; ++i) {
		_frameBuffers[i] = (uint8_t *)malloc(_fileHdr.videoFrameSize);
	}
	_frameReadBuffer = (uint8_t *)malloc(_fileHdr.videoFrameSize + 1024);
	_soundReadBuffer = 0;
	_snd->_mix.playQueue(4, kMixerQueueType_D16);
	_frameCounter = 0;
	_msgsCount = 0;
	memset(&_msgs, 0, sizeof(_msgs));
	_frameTicks = 0;
	return true;
}

void CutscenePlayer_Cin::unload() {
	for (int i = 0; i < kFrameBuffersCount; ++i) {
		free(_frameBuffers[i]);
		_frameBuffers[i] = 0;
	}
	free(_frameReadBuffer);
	_frameReadBuffer = 0;
	free(_soundReadBuffer);
	_soundReadBuffer = 0;
	if (_fp) {
		fileClose(_fp);
		_fp = 0;
	}
	_snd->_mix.stopQueue();
}

bool CutscenePlayer_Cin::play() {
	if (_frameCounter == 0) {
		_render->resizeOverlay(_fileHdr.videoFrameWidth, _fileHdr.videoFrameHeight, false, kCutsceneDisplayWidth, kCutsceneDisplayHeight);
	}
	++_frameCounter;
	if (g_isDemo && _frameCounter == _duration) {
		_frameCounter = 1;
	}
	if (!g_isDemo || _frameCounter == 1) {
		if (!readFrameHeader(&_frameHdr)) {
			return false;
		}
		int palType = 0;
		if (_frameHdr.palColorsCount < 0) {
			_frameHdr.palColorsCount = -_frameHdr.palColorsCount;
			palType = 1;
		}
		const int palSize = (palType + 3) * _frameHdr.palColorsCount;
		assert(palSize + _frameHdr.videoFrameSize < _fileHdr.videoFrameSize + 1024);
		fileRead(_fp, _frameReadBuffer, palSize + _frameHdr.videoFrameSize);
		if (!g_isDemo && _frameHdr.soundFrameSize != 0) {
			_soundReadBuffer = (uint8_t *)realloc(_soundReadBuffer, _frameHdr.soundFrameSize);
			if (_soundReadBuffer) {
				fileRead(_fp, _soundReadBuffer, _frameHdr.soundFrameSize);
				_snd->_mix.appendToQueue(_soundReadBuffer, _frameHdr.soundFrameSize);
			}
		}
		updatePalette(palType, _frameHdr.palColorsCount, _frameReadBuffer);
		decodeImage(_frameReadBuffer + palSize);
		updateMessages();
	}
	drawFrame();
	return true;
}

void CutscenePlayer_Cin::drawFrame() {
	_render->clearScreen();
	if (_frameCounter != 0) {
		const int y = (kCutsceneDisplayHeight - _fileHdr.videoFrameHeight) / 2;
		_render->copyToOverlay(0, y, _fileHdr.videoFrameWidth, _fileHdr.videoFrameHeight, _frameBuffers[0], false, _palette);
	}
}

bool CutscenePlayer_Cin::update(uint32_t ticks) {
	_frameTicks += ticks - _updateTicks;
	const bool pause = (_frameTicks < kCutsceneFrameDelay);
	_updateTicks = ticks;
	_frameTicks %= kCutsceneFrameDelay;
	if (pause) {
		drawFrame();
		return true;
	} else {
		return play();
	}
}

CutscenePlayer *CutscenePlayer_Cin_create() {
	return new CutscenePlayer_Cin();
}
