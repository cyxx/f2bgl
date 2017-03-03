
#ifndef XMIPLAYER_H__
#define XMIPLAYER_H__

#include "util.h"

struct XmiPlayerImpl;

struct XmiPlayer {

	XmiPlayerImpl *_impl;

	XmiPlayer();
	~XmiPlayer();

	void setRate(int mixingRate);

	void load(const uint8_t *data, int size);
	void unload();
	void readSamples(int16_t *buf, int len);
};

#endif
