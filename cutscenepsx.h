
#ifndef CUTSCENEPSX_H__
#define CUTSCENEPSX_H__

#include "cutscene.h"

enum {
	kSectorSize = 2352,
	kAudioDataSize = 2304,
	kAudioHeaderSize = 24,
	kVideoDataSize = 2016,
	kVideoHeaderSize = 56,
	kCutscenePsxVideoWidth = 320,
	kCutscenePsxVideoHeight = 240,
};

struct DpsHeader {
	uint16_t w, h;
	uint16_t framesCount;
	uint8_t xaStereo;
	uint16_t xaSampleRate;
	uint8_t xaBits;
};

struct CutscenePsx: Cutscene {
	uint8_t _sector[kSectorSize];
	DpsHeader _header;
	int _frameCounter;
	int _sectorCounter;
	uint8_t *_rgbaBuffer;

	CutscenePsx(Render *render, Game *g, Sound *sound);
	virtual ~CutscenePsx();

	bool readSector();
	bool play();
	bool readHeader(DpsHeader *header);
	virtual bool load(int num);
	virtual void unload();
	virtual bool update(uint32_t ticks);
};

#endif // CUTSCENEPSX_H__
