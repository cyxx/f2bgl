
#ifndef RESOURCEPSX_H__
#define RESOURCEPSX_H__

#include "file.h"
#include "util.h"

enum {
	kResTypePsx_VRM
};

struct ResourcePsx {

	void loadLevelData(int level, int resType);
};

#endif // RESOURCEPSX_H__
