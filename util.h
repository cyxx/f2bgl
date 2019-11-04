/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#ifndef UTIL_H__
#define UTIL_H__

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include "intern.h"

enum {
	kDebug_INFO     = 1 << 0,
	kDebug_GAME     = 1 << 1,
	kDebug_RESOURCE = 1 << 2,
	kDebug_FILE     = 1 << 3,
	kDebug_CUTSCENE = 1 << 4,
	kDebug_OPCODES  = 1 << 5,
	kDebug_SOUND    = 1 << 6,
	kDebug_SAVELOAD = 1 << 7,
	kDebug_XMIDI    = 1 << 8,
	kDebug_INSTALL  = 1 << 9,
};

extern const char *g_caption;
extern int g_utilDebugMask;

void stringToLowerCase(char *p);
void stringToUpperCase(char *p);
char *stringTrimLeft(char *p);
char *stringNextToken(char **p);
void debug(int debugChannel, const char *msg, ...);
void warning(const char *msg, ...);
void error(const char *msg, ...);
uint32_t getStringHash(const char *s);

void saveTGA(const char *filepath, const uint8_t *rgb, int w, int h, bool thumbnail);
uint8_t *loadTGA(const char *filepath, int *w, int *h);

#undef MIN
template<typename T>
inline T MIN(T v1, T v2) {
	return (v1 < v2) ? v1 : v2;
}

#undef MAX
template<typename T>
inline T MAX(T v1, T v2) {
	return (v1 > v2) ? v1 : v2;
}

#undef ABS
template<typename T>
inline T ABS(T t) {
	return (t < 0) ? -t : t;
}

template<typename T>
inline T CLIP(T t, T tmin, T tmax) {
	return (t < tmin ? tmin : (t > tmax ? tmax : t));
}

template<typename T>
inline bool INRANGE(T t, T tmin, T tmax) {
	return (t >= tmin && t <= tmax);
}

template<typename T>
inline void SWAP(T &a, T &b) {
	T tmp = a; a = b; b = tmp;
}

template<typename T>
inline T *ALLOC(int count) {
	return (T *)calloc(count, sizeof(T));
}

#endif // UTIL_H__
