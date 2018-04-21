
#ifndef CUTSCENEPSX_H__
#define CUTSCENEPSX_H__

#include "file.h"

enum {
	kSectorSize = 2352,
	kAudioDataSize = 2304,
	kAudioHeaderSize = 24,
	kVideoDataSize = 2016,
	kVideoHeaderSize = 56,
};

struct DpsDecoder;
struct Render;
struct Sound;

struct DpsHeader {
	uint16_t w, h;
	uint16_t framesCount;
	bool stereo;
};

struct CutscenePsx {
	DpsDecoder *_decoder;
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
	bool load(int num);
	void unload();
};

#endif // CUTSCENEPSX_H__
