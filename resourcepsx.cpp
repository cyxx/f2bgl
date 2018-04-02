
#include "resourcepsx.h"

void ResourcePsx::loadLevelData(int level, int resType) {
	// PSX datafiles are 1-indexed
	++level;

	char name[16];
	switch (resType) {
	case kResTypePsx_VRM: {
			snprintf(name, sizeof(name), "level%d.vrm", level);
			File *fp = fileOpenPsx(name, kFileType_PSX_LEVELDATA, level);
			if (fp) {
				fileReadUint32LE(fp);
				int count = 0;
				while (1) {
					const int w = fileReadUint16LE(fp);
					const int h = fileReadUint16LE(fp);
					if (fileEof(fp)) {
						break;
					}
					debug(kDebug_INFO, "VRAM %d w %d h %d", count, w, h);
					if (count == 0) {
						assert(w == 640 && h == 272);
					}
					fileSetPos(fp, w * h, kFilePosition_CUR);
					++count;
				}
				fileClose(fp);
			}
		}
		break;
	}
}
