/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#ifndef INTERN_H__
#define INTERN_H__

#undef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

struct Vertex {
	int x, y, z;
	int nx, ny, nz;
};

inline int sext32(int x, int bits) {
	const int shift = 32 - bits;
	return (((int32_t)x) << shift) >> shift;
}

inline uint16_t READ_BE_UINT16(const void *ptr) {
	const uint8_t *p = (const uint8_t *)ptr;
	return p[1] | (p[0] << 8);
}

inline uint16_t READ_LE_UINT16(const void *ptr) {
	const uint8_t *p = (const uint8_t *)ptr;
	return p[0] | (p[1] << 8);
}

inline uint32_t READ_BE_UINT32(const void *ptr) {
	const uint8_t *p = (const uint8_t *)ptr;
	return p[3] | (p[2] << 8) | (p[1] << 16) | (p[0] << 24);
}

inline uint32_t READ_LE_UINT32(const void *ptr) {
	const uint8_t *p = (const uint8_t *)ptr;
	return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

inline Vertex READ_VERTEX32(const void *ptr) {
	uint32_t n = READ_LE_UINT32(ptr);
	Vertex v;
	v.nz = sext32(n & 7, 3); n >>= 3;
	v.ny = sext32(n & 7, 3); n >>= 3;
	v.nx = sext32(n & 7, 3); n >>= 3;
	v.y = n & 127; n >>= 7;
	v.z = (int8_t)(n & 255); n >>= 8;
	v.x = (int8_t)(n & 255); n >>= 8;
	return v;
}

#endif // INTERN_H__
