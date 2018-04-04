
#ifndef RESOURCEPSX_H__
#define RESOURCEPSX_H__

#include "file.h"
#include "util.h"

enum {
	kResTypePsx_VRM
};

enum {
	kVrmLoadingScreenWidth = 320,
	kVrmLoadingScreenHeight = 240
};

struct ResourcePsx {

	uint8_t *_vrmLoadingBitmap;

	ResourcePsx();

	void loadLevelData(int level, int resType);
	void unloadLevelData(int resType);
};

#endif // RESOURCEPSX_H__
