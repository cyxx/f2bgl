/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include <math.h>
#include "trigo.h"

int32_t g_cos[1024], g_sin[1024];
int32_t g_atan[256];

void Vec_xz::rotate(int angle, int shift, int dist) {
	angle &= 1023;
	const int cosa = g_cos[angle] * dist;
	const int sina = g_sin[angle] * dist;
	const int xr = fixedMul(cosa, x, shift) + fixedMul(sina, z, shift);
	const int zr = fixedMul(cosa, z, shift) - fixedMul(sina, x, shift);
	x = xr;
	z = zr;
}

int getAngleDiff(int angle1, int angle2) {
	static const int kPi = 512;
	if (angle1 != angle2) {
		const int sign = (angle1 < angle2) ? -1 : 1;
		const int diff = ABS(angle1 - angle2);
		if (diff > kPi) {
			return -sign * (2 * kPi - diff);
		} else {
			return sign * diff;
		}
	}
	return 0;
}

int fixedSqrt(int x) {
	assert(x >= 0);
	return (int)(sqrt(x) + .5);
}

int getSquareDistance(int x1, int z1, int x2, int z2, int shift) {
	const int dx = (x2 >> shift) - (x1 >> shift);
	const int dz = (z2 >> shift) - (z1 >> shift);
	return dx * dx + dz * dz;
}

int getAngleFromPos(int x, int z) {
	if (0) { // use pre-computed table
		const int angle = (int)(atan2(x, z) * 512 / M_PI);
		if (angle < 0) {
			return angle + 1024;
		} else {
			return angle;
		}
	}
	static const int kTangent = 256;

	if (x == 0) {
		if (z > 0) {
			return 0;
		} else {
			return 512;
		}
	} else if (x > 0) {
		if (z > x) {
			return 1 + g_atan[x * kTangent / z];
		} else if (z == x) {
			return 128;
		} else if (z > 0) {
			return 256 - 1 - g_atan[z * kTangent / x];
		} else if (z == 0) {
			return 256;
		} else if (z > -x) {
			return 256 + 1 + g_atan[-z * kTangent / x];
		} else if (z == -x) {
			return 384;
		} else {
			return 512 - 1 - g_atan[-x * kTangent/ z];
		}
	} else {
		if (z < x) {
			return 512 + 1 + g_atan[x * kTangent/ z];
		} else if (z == x) {
			return 640;
		} else if (z < 0) {
			return 768 - 1 - g_atan[z * kTangent / x];
		} else if (z == 0) {
			return 768;
		} else if (z < -x) {
			return 768 + 1 + g_atan[-z * kTangent / x];
		} else if (z == -x) {
			return 896;
		} else {
			return 1024 - 1 - g_atan[-x * kTangent / z];
		}
	}
}
