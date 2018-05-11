
#ifndef RESOURCEPSX_H__
#define RESOURCEPSX_H__

#include "file.h"
#include "util.h"

enum {
	kResTypePsx_SON,
	kResTypePsx_VRM,
};

enum {
	kResOffsetType_SON
};

enum {
	kVrmLoadingScreenWidth = 320,
	kVrmLoadingScreenHeight = 240,
	kResPsxSonOffsetsTableSize = 3, // .SPU, .VH, .VB
	kVagOffsetsTableSize = 256,
};

struct ResPsxOffset {
	char ext[4];
	uint32_t offset;
	uint32_t size;
};

struct VagOffset {
	uint32_t offset;
	uint32_t size;
};

struct ResourcePsx {

	uint8_t *_vrmLoadingBitmap;
	ResPsxOffset _sonOffsetsTable[kResPsxSonOffsetsTableSize];
	File *_fileSon;
	uint32_t _vagOffsetsTableSize;
	VagOffset _vagOffsetsTable[kVagOffsetsTableSize];

	ResourcePsx();

	void loadLevelData(int level, int resType);
	void unloadLevelData(int resType);

	void readDataOffsetsTable(File *fp, int offsetType);
	uint32_t seekData(const char *ext, File *fp, int offsetType, uint32_t offset = 0);

	void loadVAB(File *fp);
	void loadVRM(File *fp);

	uint32_t seekVag(int num);
};

#endif // RESOURCEPSX_H__
