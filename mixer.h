/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#ifndef MIXER_H__
#define MIXER_H__

#include "util.h"

enum {
	kMaxSoundsCount = 32,
	kMaxQueuesCount = 1
};

enum {
	kDefaultVolume = 127,
	kDefaultPan = 64
};

struct MixerSound;
struct MixerQueue;

struct Mixer {
	int _rate;
	MixerSound *_soundsTable[kMaxSoundsCount];
	MixerQueue *_queue;
	uint32_t _idsMap[kMaxSoundsCount];
	void (*_lock)(int);

	Mixer();
	~Mixer();

	void setFormat(int rate, int fmt);
	int findIndexById(uint32_t id) const;

	void playWav(FILE *, int dataSize, int volume, int pan, uint32_t id, bool compressed = true);
	void stopWav(uint32_t);
	bool isWavPlaying(uint32_t) const;

	void playQueue(int preloadSize);
	void appendToQueue(const uint8_t *buf, int size);
	void stopQueue();

	void mixBuf(int16_t *buf, int len);
	static void mixCb(void *param, uint8_t *buf, int len);
};

#endif // MIXER_H__
