
#include <math.h>
#include "cutscenepsx.h"
#include "render.h"
#include "sound.h"
#include "mdec.h"

static uint32_t yuv420_to_rgba(int y, int u, int v) {
	const int r = CLIP((int)round(y + 1.402 * v),             0, 255);
	const int g = CLIP((int)round(y - 0.344 * u - 0.714 * v), 0, 255);
	const int b = CLIP((int)round(y + 1.772 * u),             0, 255);
	return 0xFF000000 | (b << 16) | (g << 8) | r;
}

static void outputMdecCb(const MdecOutput *output, void *userdata) {
	CutscenePsx *cut = (CutscenePsx *)userdata;
	uint32_t *dst = (uint32_t *)cut->_rgbaBuffer + (cut->_header.h - 1) * cut->_header.w;
	const uint8_t *luma = output->planes[0].ptr;
	const uint8_t *cb = output->planes[1].ptr;
	const uint8_t *cr = output->planes[2].ptr;
	for (int y = 0; y < output->h; ++y) {
		for (int x = 0; x < output->w; ++x) {
			dst[x] = yuv420_to_rgba(luma[x], cb[x / 2] - 128, cr[x / 2] - 128);
		}
		dst -= cut->_header.w;
		luma += output->planes[0].pitch;
		cb += output->planes[1].pitch * (y & 1);
		cr += output->planes[2].pitch * (y & 1);
	}
}

CutscenePsx::CutscenePsx(Render *render, Game *g, Sound *sound)
	: Cutscene(render, g, sound), _rgbaBuffer(0) {
}

CutscenePsx::~CutscenePsx() {
	free(_rgbaBuffer);
}

static const char *_namesTable[] = {
	// 0
	"moar",
	"moba",
	"moex",
	"moel",
	// 4
	"molz",
	"modp",
	"mopl",
	"momc",
	// 8
	"mogr",
	"mobu",
	"moir",
	"mott",
	// 12
	"g", // ?
	"intr",
	"evas",
	"evbm",
	// 16
	"prof",
	"dsta",
	"exbm",
	"exvb",
	// 20
	"agee",
	"anci",
	"supm",
	"aspi",
	// 24
	"bupp",
	"arbu",
	"cerv",
	"evam",
	// 28
	"bomb",
	"int2",
	"bast",
	"fuit",
	// 32
	"fin1",
	"boum",
	"mosm",
	"hcle",
	// 36
	"scle",
	"titl",
	"motv",
	"logo",
	// 40
	"movr",
	"mogl",
	"mogo",
	"cong",
	// 44
	"fad1",
	"fad2",
	"pakk",
	"ealo",
	// 48
	"mgmm",
	"exsh",
	"fin2",
	"bass",
	// 52
	"fui2",
	"gend"
};

static const struct {
	const char *name;
	int num;
} _namesTable_gr[] = {
	{ "grex", 2 },
	{ "grlz", 4 },
	{ "grmc", 7 },
	{ "grsm", 34 }
};

static const uint8_t _cdSync[12] = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00 };

bool CutscenePsx::readSector() {
	// mode2 form2 : cdSync (12 bytes) header (4 bytes) subheader (8 bytes) data (2324 bytes) edc (4 bytes)
	++_sectorCounter;
	const int count = fileRead(_fp, _sector, sizeof(_sector));
	return count == kSectorSize && memcmp(_sector, _cdSync, sizeof(_cdSync)) == 0;
}

bool CutscenePsx::play() {
	if (!_fp) {
		return false;
	}
	bool err = false;
	// demux audio and video frames
	uint8_t *videoData = 0;
	int videoCurrentSector = -1;
	int videoSectorsCount = 0;
	do {
		if (!readSector()) {
			warning("CutscenePsx::play() Invalid sector %d", _sectorCounter);
			err = true;
			break;
		}
		const int type = _sector[0x12] & 0xE;
		if (type == 2 && READ_LE_UINT32(_sector + 0x18) == 0x80010160) {
			const int currentSector = READ_LE_UINT16(&_sector[0x1C]);
			if (videoCurrentSector != -1 && videoCurrentSector != currentSector - 1) {
				err = true;
				break;
			}
			videoCurrentSector = currentSector;
			const int sectorsCount = READ_LE_UINT16(&_sector[0x1E]);
			if (videoSectorsCount != 0 && videoSectorsCount != sectorsCount) {
				err = true;
				break;
			}
			videoSectorsCount = sectorsCount;
			const int frameSize = READ_LE_UINT32(&_sector[0x24]);
			if (frameSize != 0 && currentSector < sectorsCount) {
				if (currentSector == 0) {
					videoData = (uint8_t *)malloc(sectorsCount * kVideoDataSize);
				}
				memcpy(videoData + currentSector * kVideoDataSize, _sector + kVideoHeaderSize, kVideoDataSize);
				if (currentSector == videoSectorsCount - 1) {
					decodeMDEC(videoData, videoSectorsCount * kVideoDataSize, _header.w, _header.h, this, outputMdecCb);
					const int y = (kCutscenePsxVideoHeight - _header.h) / 2;
					_render->copyToOverlay(0, y, _header.w, _header.h, _rgbaBuffer, true);
					++_frameCounter;
					free(videoData);
					videoData = 0;
				}
			}
		} else if (type == 4) {
			if (_header.xaSampleRate == 0) { // this is the first audio sector, get the format
				_header.xaStereo = (_sector[0x13] & 1) != 0;
				_header.xaSampleRate = (_sector[0x13] & 4) != 0 ? 18900 : 37800;
				_header.xaBits = (_sector[0x13] & 0x10) != 0 ? 4 : 8;
				if (!_header.xaStereo || _header.xaSampleRate != 37800 || _header.xaBits != 8) {
					warning("CutscenePsx::play() Unsupported audio format, stereo %d bits %d freq %d", _header.xaStereo, _header.xaBits, _header.xaSampleRate);
					err = true;
					break;
				}
				_snd->_mix.playQueue(4, kMixerQueueType_XA);
			}
			_snd->_mix.appendToQueue(_sector + kAudioHeaderSize, kAudioDataSize);
		}
	} while (videoCurrentSector != videoSectorsCount - 1 && !fileEof(_fp) && !err);
	return !err;
}

bool CutscenePsx::readHeader(DpsHeader *hdr) {
	if (!readSector()) {
		warning("CutscenePsx::readHeader() Invalid first sector");
	} else {
		const uint8_t *header = _sector + 0x18;
		if (memcmp(header, "DPS", 3) != 0) {
			warning("CutscenePsx::readHeader() Invalid header signature");
		} else {
			hdr->w = READ_LE_UINT16(header + 0x10);
			hdr->h = READ_LE_UINT16(header + 0x12);
			hdr->framesCount = READ_LE_UINT16(header + 0x1C);
			int sector = 1;
			for (; sector < 5; ++sector) {
				if (!readSector()) {
					warning("CutscenePsx::readHeader() Invalid sector %d", sector);
					break;
				}
			}
			return sector == 5;
		}
	}
	return false;
}

bool CutscenePsx::load(int num) {
	memset(&_header, 0, sizeof(_header));
	_frameCounter = 0;
	_sectorCounter = -1;
	const char *name = _namesTable[num];
	if (fileLanguage() == kFileLanguage_GR) {
		for (int i = 0; i < ARRAYSIZE(_namesTable_gr); ++i) {
			if (_namesTable_gr[i].num == num) {
				name = _namesTable_gr[i].name;
				break;
			}
		}
	}
	char filename[16];
	snprintf(filename, sizeof(filename), "%s.dps", name);
	_fp = fileOpenPsx(filename, kFileType_PSX_VIDEO);
	if (_fp) {
		const int size = fileSize(_fp);
		if ((size % kSectorSize) != 0) {
			warning("Unexpected file size (%d bytes) for '%s', not a multiple of a raw CD sector size", size, filename);
			fileClose(_fp);
			_fp = 0;
		} else if (!readHeader(&_header)) {
			warning("Unable to read cutscene file header");
			fileClose(_fp);
			_fp = 0;
		} else {
			_rgbaBuffer = (uint8_t *)malloc(_header.w * _header.h * sizeof(uint32_t));
			_render->clearScreen();
			_render->resizeOverlay(_header.w, _header.h, true, kCutscenePsxVideoWidth, kCutscenePsxVideoHeight);
		}
	}
	return _fp != 0;
}

void CutscenePsx::unload() {
	if (_rgbaBuffer) {
		free(_rgbaBuffer);
		_rgbaBuffer = 0;
	}
	if (_fp) {
		fileClose(_fp);
		_fp = 0;
	}
	_snd->_mix.stopQueue();
}

bool CutscenePsx::update(uint32_t ticks) {
	return play();
}
