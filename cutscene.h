/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#ifndef CUTSCENE_H__
#define CUTSCENE_H__

#include "file.h"
#include "util.h"

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

enum {
	kCutsceneScenesCount = 54,
	kFrameBuffersCount = 3,
	kSubtitleMessagesCount = 16,
	kCutscenePlaybackQueueSize = 4,
	kCutsceneFrameDelay = 1000 / 12,
	kCutsceneDisplayWidth = 320,
	kCutsceneDisplayHeight = 200,
};

struct Game;
struct Sound;
struct Render;

struct Cutscene {
	Render *_render;
	Game *_game;
	Sound *_snd;

	int _playCounter;
	int _playedTable[kCutsceneScenesCount];
	int _numToPlayCounter;
	int _numToPlay;
	File *_fp;
	uint8_t _palette[256 * 3];
	CinFileHeader _fileHdr;
	CinFrameHeader _frameHdr;
	bool _interrupted;
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
	int _playQueue[kCutscenePlaybackQueueSize];
	int _playQueueSize;
	uint32_t _updateTicks;
	uint32_t _frameTicks;

	Cutscene(Render *render, Game *g, Sound *snd);
	virtual ~Cutscene();

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
	bool isInterrupted() const;
	void queue(int num, int counter = 0);
	int dequeue();
	int changeToNext();
};

#endif // CUTSCENE_H__
