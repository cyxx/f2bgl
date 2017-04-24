/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#ifndef TRIGO_H__
#define TRIGO_H__

#include "util.h"

struct Vec_xz {
	int x, z;
	Vec_xz(int x_, int z_) : x(x_), z(z_) {}
	void rotate(int angle, int shift = 15, int dist = 1);
};

int getAngleDiff(int angle1, int angle2);
int fixedSqrt(int x);
int getSquareDistance(int x1, int z1, int x2, int z2, int shift = 0);
int getAngleFromPos(int x, int z);

static inline int fixedInt(int a, int b) {
	return a << b;
}

static inline int fixedMul(int a, int b, int c) {
	int64_t r = a;
	r *= b;
	r >>= c;
	return r;
}

static inline int fixedDiv(int a, int b, int c) {
	int64_t r = a;
	r <<= b;
	r /= c;
	return r;
}

extern int32_t g_sin[1024], g_cos[1024];
extern int32_t g_atan[256];

#endif // TRIGO_H__
