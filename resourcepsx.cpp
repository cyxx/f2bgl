
#include "resourcepsx.h"
#include "textureconvert.h"

static const char *_levels[] = {
	"1", "2a", "2b", "2c", "3", "4a", "4b", "4c", "5a", "5b", "5c", "6a", "6b"
};

ResourcePsx::ResourcePsx() {
	_vrmLoadingBitmap = 0;
}

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
					int w = fileReadUint16LE(fp);
					int h = fileReadUint16LE(fp);
					if (fileEof(fp)) {
						break;
					}
					debug(kDebug_RESOURCE, "VRAM %d w %d h %d", count, w, h);
					if (count == 0) {
						assert(w == 640 && h == 272);
						_vrmLoadingBitmap = (uint8_t *)malloc(kVrmLoadingScreenWidth * kVrmLoadingScreenHeight * 4);
						if (_vrmLoadingBitmap) {
							const int dstPitch = kVrmLoadingScreenWidth * 4;
							for (int y = 0; y < kVrmLoadingScreenHeight; ++y) {
								uint8_t *dst = _vrmLoadingBitmap + (kVrmLoadingScreenHeight - 1 - y) * dstPitch;
								for (int x = 0; x < kVrmLoadingScreenWidth; ++x) {
									const uint32_t color = bgr555_to_rgba(fileReadUint16LE(fp));
									memcpy(dst + x * 4, &color, 4);
								}
							}
							h -= kVrmLoadingScreenHeight;
						}
					} else {
						assert(w == 256 && h == 256);
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

void ResourcePsx::unloadLevelData(int resType) {
	switch (resType) {
	case kResTypePsx_VRM:
		free(_vrmLoadingBitmap);
		_vrmLoadingBitmap = 0;
		break;
	}
}
