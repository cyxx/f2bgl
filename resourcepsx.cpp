
#include "resourcepsx.h"

static const char *_levels[] = {
	"1", "2a", "2b", "2c", "3", "4a", "4b", "4c", "5a", "5b", "5c", "6a", "6b", 0
};

void ResourcePsx::loadLevelData(int level, int resType) {
	char name[16];
	switch (resType) {
	case kResTypePsx_VRM: {
			snprintf(name, sizeof(name), "level%s.vrm", _levels[level]);
			File *fp = fileOpenPsx(name, kFileType_PSX_LEVELDATA, level + 1);
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
