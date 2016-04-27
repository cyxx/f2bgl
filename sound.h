/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#ifndef SOUND_H__
#define SOUND_H__

#include "file.h"
#include "util.h"
#include "mixer.h"

struct Resource;

struct DigiSnd {
	char name[16];
	uint32_t offset;
	uint32_t size;
};

struct Sound {

	Sound(Resource *res);
	~Sound();

	void init();

	void loadDigiSnd(File *fp);
	const DigiSnd *findDigiSndByName(const char *name) const;

	void setVolume(int volume);
	void setPan(int pan);

	void playSfx(int16_t objKey, int16_t sndKey);
	void stopSfx(int16_t objKey, int16_t sndKey);

	void playVoice(int16_t objKey, uint32_t crc);
	void stopVoice(int16_t objKey);
	bool isVoicePlaying(int16_t objKey) const;

	void playMidi(int16_t objKey, int index);
	void stopMidi(int16_t objKey, int index);

	int _sfxVolume;
	int _sfxPan;
	Resource *_res;
	Mixer _mix;
	bool _digiCompressed;
	int _digiCount;
	DigiSnd *_digiTable;
	File *_fpSnd;
	int _musicMode;
	int16_t _musicKey;
};

#endif // SOUND_H__
