
#include "cutscenepsx.h"
#include "render.h"
#include "sound.h"
#include "mdec.h"

struct Mdec {
	uint8_t *_rgba;
	int _w, _h;

	Mdec();

	void fini();
	void init(int w, int h);
	void decode(const uint8_t *data, int size);
};

Mdec::Mdec() {
	_rgba = 0;
	_w = _h = 0;
}

void Mdec::fini() {
	if (_rgba) {
		free(_rgba);
		_rgba = 0;
	}
}

void Mdec::init(int w, int h) {
	_w = w;
	_h = h;
	_rgba = (uint8_t *)malloc(w * 4 * h);
}

static uint32_t yuv420_to_rgba(int y, int u, int v) {
	const int r = CLIP((int)(y + 1.402 * (v - 128)),                     0, 255);
	const int g = CLIP((int)(y - 0.344 * (u - 128) - 0.714 * (v - 128)), 0, 255);
	const int b = CLIP((int)(y + 1.772 * (u - 128)),                     0, 255);
	return 0xFF000000 | (b << 16) | (g << 8) | r;
}

static void outputMdecCb(const MdecOutput *output, void *userdata) {
	Mdec *mdec = (Mdec *)userdata;
	uint8_t *dst = mdec->_rgba + (mdec->_h - 1) * mdec->_w * 4;
	for (int y = 0; y < output->h; ++y) {
		const uint8_t *Y = output->planes[0].ptr + y * output->planes[0].pitch;
		const uint8_t *U = output->planes[1].ptr + y / 2 * output->planes[1].pitch;
		const uint8_t *V = output->planes[2].ptr + y / 2 * output->planes[2].pitch;
		for (int x = 0; x < output->w; ++x) {
			const uint32_t color = yuv420_to_rgba(Y[x], U[x / 2], V[x / 2]);
			memcpy(&dst[x * 4], &color, 4);
		}
		dst -= mdec->_w * 4;
	}
}

void Mdec::decode(const uint8_t *data, int size) {
	decodeMDEC(data, size, _w, _h, this, outputMdecCb);
}

CutscenePsx::CutscenePsx(Render *render, Game *g, Sound *sound)
	: Cutscene(render, g, sound) {
	_mdec = new Mdec();
}

CutscenePsx::~CutscenePsx() {
	delete _mdec;
	_mdec = 0;
}

static const char *_namesTable[] = {
	// 0
	"moar",
	"moba",
	"moex", // grex
	"moel",
	// 4
	"molz", // grlz
	"modp",
	"mopl",
	"momc", // grmc
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
	"mosm", // grsm
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
					_mdec->decode(videoData, videoSectorsCount * kVideoDataSize);
					const int y = (kCutscenePsxVideoHeight - _header.h) / 2;
					_render->copyToOverlay(0, y, _header.w, _header.h, _mdec->_rgba, true);
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

	char filename[16];
	snprintf(filename, sizeof(filename), "%s.dps", _namesTable[num]);
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
			_mdec->init(_header.w, _header.h);
			_render->clearScreen();
			_render->resizeOverlay(_header.w, _header.h, true, kCutscenePsxVideoWidth, kCutscenePsxVideoHeight);
		}
	}
	return _fp != 0;
}

void CutscenePsx::unload() {
	if (_mdec) {
		_mdec->fini();
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
