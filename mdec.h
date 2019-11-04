
#ifndef MDEC_H__
#define MDEC_H__

#include "util.h"

struct MdecOutput {
	int w, h;
	struct {
		uint8_t *ptr;
		int pitch;
	} planes[3];
};

int decodeMDEC(const uint8_t *src, int len, int w, int h, void *userdata, void (*output)(const struct MdecOutput *, void *));

#endif // MDEC_H__
