/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#ifndef MIXER_H__
#define MIXER_H__

#include "util.h"

enum {
	kMaxSoundsCount = 32,
	kMaxQueuesCount = 1,
	kFracBits = 10,
};

enum {
	kDefaultVolume = 127,
	kDefaultPan = 64 // 0,128 range
};

enum {
	kMixerQueueType_D16,
	kMixerQueueType_XA, // stereo 37800Hz
};

struct MixerSound {
	int volumeL;
	int volumeR;
	int loopsCount;
	virtual ~MixerSound() {}
	virtual bool load(File *f, int dataSize, int mixerSampleRate) = 0;
	virtual bool readSamples(int16_t *, int len) = 0;
};

struct MixerQueue;
struct XmiPlayer;

struct Mixer {
	int _rate;
	MixerSound *_soundsTable[kMaxSoundsCount];
	MixerQueue *_queue;
	XmiPlayer *_xmiPlayer;
	uint32_t _idsMap[kMaxSoundsCount];
	void (*_lock)(int);
	int _soundVolume;
	int _musicVolume;
	int _voiceVolume;

	Mixer();
	~Mixer();

	void setSoundVolume(int volume);
	void setMusicVolume(int volume);
	void setVoiceVolume(int volume);

	void setFormat(int rate, int fmt);
	int findIndexById(uint32_t id) const;

	void playWav(File *, int dataSize, int volume, int pan, uint32_t id, bool isVoice, bool compressed = true);
	void stopWav(uint32_t);
	bool isWavPlaying(uint32_t) const;
	void loopWav(uint32_t, int count);

	void playQueue(int preloadSize, int type);
	void appendToQueue(const uint8_t *buf, int size);
	void stopQueue();

	void playXmi(File *, int size);
	void stopXmi();

	void playXa(File *, int size, uint32_t id);
	void stopXa(uint32_t);

	void mixBuf(int16_t *buf, int len);
	static void mixCb(void *param, uint8_t *buf, int len);
};

#endif // MIXER_H__
