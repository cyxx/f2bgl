
#ifndef XMIPLAYER_H__
#define XMIPLAYER_H__

#include "util.h"

struct Resource;

struct XmiPlayer {

	XmiPlayer() {}
	virtual ~XmiPlayer() {}

	virtual void setRate(int rate) = 0;
	virtual void setVolume(int volume) = 0;

	virtual void load(const uint8_t *data, int size) = 0;
	virtual void unload() = 0;
	virtual void readSamples(int16_t *buf, int len) = 0;
};

XmiPlayer *XmiPlayer_WildMidi_create(Resource *res);
XmiPlayer *XmiPlayer_FluidSynth_create(const char *sf2);

#endif
