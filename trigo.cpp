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
	if (angle1 != angle2) {
		int mul = 1;
		if (angle1 < angle2) {
			SWAP(angle1, angle2);
			mul = -1;
		}
		int ret = angle1 - angle2;
		if (ret > 512) {
			ret = 1024 - ret;
			mul = -mul;
		}
		return mul * ret;
	}
	return 0;
}

int fixedSqrt(int x, int shift) {
	assert(x >= 0);
	return (int)(sqrt(x) + .5);
}

int getSquareDistance(int x1, int z1, int x2, int z2, int shift) {
	const int dx = (x2 >> shift) - (x1 >> shift);
	const int dz = (z2 >> shift) - (z1 >> shift);
	return dx * dx + dz * dz;
}

int getAngleFromPos(int X, int Z) {
	static const int maxRy = 256;
	int A;

	if (X == 0) {
		if (Z > 0) {
			A = 0;
		} else {
			A = 512;
		}
	} else if (X > 0) {
		if (Z > X) {
			A = 1 + g_atan[fixedMul(X, maxRy << 15, 15) / Z];
		} else if (Z == X) {
			A = 128;
		} else if (Z > 0) {
			A = 256 - 1 - g_atan[fixedMul(Z, maxRy << 15, 15) / X];
		} else if (Z == 0) {
			A = 256;
		} else if (Z > -X) {
			A = 256 + 1 + g_atan[-fixedMul(Z, maxRy << 15, 15) / X];
		} else if (Z == -X) {
			A = 384;
		} else {
			A = 512 - 1 - g_atan[-fixedMul(X, maxRy << 15, 15) / Z];
		}
	} else {
		if (Z < X) {
			A = 512 + 1 + g_atan[fixedMul(X, maxRy << 15, 15) / Z];
		} else if (Z == X) {
			A = 640;
		} else if (Z < 0) {
			A = 768 - 1 - g_atan[fixedMul(Z, maxRy << 15, 15) / X];
		} else if (Z == 0) {
			A = 768;
		} else if (Z < -X) {
			A = 768 + 1 + g_atan[-fixedMul(Z, maxRy << 15, 15) / X];
		} else if (Z == -X) {
			A = 896;
		} else {
			A = 1024 - 1 - g_atan[-fixedMul(X, maxRy << 15, 15) / Z];
		}
	}
	return A;
}
