
#ifndef XMIPLAYER_H__
#define XMIPLAYER_H__

#include "util.h"

struct Resource;
struct XmiPlayerImpl;

struct XmiPlayer {

	XmiPlayerImpl *_impl;
	Resource *_res;

	XmiPlayer(Resource *res);
	~XmiPlayer();

	void setRate(int rate);
	void setVolume(int volume);

	void load(const uint8_t *data, int size);
	void unload();
	void readSamples(int16_t *buf, int len);
};

#endif
