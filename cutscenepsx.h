
#ifndef CUTSCENEPSX_H__
#define CUTSCENEPSX_H__

#include "file.h"

enum {
	kSectorSize = 2352,
	kAudioDataSize = 2304,
	kAudioHeaderSize = 24,
	kVideoDataSize = 2016,
	kVideoHeaderSize = 56,
	kCutscenePsxVideoWidth = 320,
	kCutscenePsxVideoHeight = 240,
};

struct Mdec;
struct Render;
struct Sound;

struct DpsHeader {
	uint16_t w, h;
	uint16_t framesCount;
	uint8_t xaStereo;
	uint16_t xaSampleRate;
	uint8_t xaBits;
};

struct CutscenePsx {
	Mdec *_mdec;
	Render *_render;
	Sound *_sound;
	File *_fp;
	uint8_t _sector[kSectorSize];
	DpsHeader _header;
	int _frameCounter;
	int _sectorCounter;

	CutscenePsx(Render *render, Sound *sound);
	~CutscenePsx();

	bool readSector();
	bool play();
	bool readHeader(DpsHeader *header);
	bool load(int num);
	void unload();
};

#endif // CUTSCENEPSX_H__
