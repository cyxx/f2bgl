
#include "resourcepsx.h"
#include "textureconvert.h"

static const char *_levels[] = {
	"1", "2a", "2b", "2c", "3", "4a", "4b", "4c", "5a", "5b", "5c", "6a", "6b"
};

ResourcePsx::ResourcePsx() {
	_vrmLoadingBitmap = 0;
	_vagOffsetsTableSize = 0;
	memset(_levOffsetsTable, 0, sizeof(_levOffsetsTable));
	_fileLev = 0;
	memset(_sonOffsetsTable, 0, sizeof(_sonOffsetsTable));
	_fileSon = 0;
}

void ResourcePsx::loadLevelData(int level, int resType) {
	switch (resType) {
	case kResTypePsx_LEV: {
			char name[16];
			snprintf(name, sizeof(name), "level%d.lev", level + 1);
			_fileLev = fileOpenPsx(name, kFileType_PSX_LEVELDATA, level + 1);
			if (_fileLev) {
				readDataOffsetsTable(_fileLev, kResOffsetType_LEV, _levOffsetsTable);
			}
		}
		break;
	case kResTypePsx_SON: {
			char name[16];
			snprintf(name, sizeof(name), "level%d.son", level + 1);
			_fileSon = fileOpenPsx(name, kFileType_PSX_LEVELDATA, level + 1);
			if (_fileSon) {
				readDataOffsetsTable(_fileSon, kResOffsetType_SON, _sonOffsetsTable);
				loadVAB(_fileSon);
			}
		}
		break;
	case kResTypePsx_VRM: {
			char name[16];
			snprintf(name, sizeof(name), "level%s.vrm", _levels[level]);
			File *fp = fileOpenPsx(name, kFileType_PSX_LEVELDATA, level + 1);
			if (fp) {
				loadVRM(fp);
				fileClose(fp);
			}
		}
		break;
	}
}

void ResourcePsx::unloadLevelData(int resType) {
	switch (resType) {
	case kResTypePsx_LEV:
		memset(_levOffsetsTable, 0, sizeof(_levOffsetsTable));
		if (_fileLev) {
			fileClose(_fileLev);
			_fileLev = 0;
		}
		break;
	case kResTypePsx_SON:
		memset(_sonOffsetsTable, 0, sizeof(_sonOffsetsTable));
		if (_fileSon) {
			fileClose(_fileSon);
			_fileSon = 0;
		}
		_vagOffsetsTableSize = 0;
		break;
	case kResTypePsx_VRM:
		if (_vrmLoadingBitmap) {
			free(_vrmLoadingBitmap);
			_vrmLoadingBitmap = 0;
		}
		break;
	}
}

void ResourcePsx::readDataOffsetsTable(File *fp, int offsetType, ResPsxOffset *offsetsTable) {
	const int dataSize = fileSize(fp);
	const int count = fileReadUint32LE(fp);
	const uint32_t baseOffset = sizeof(uint32_t) + count * (sizeof(uint32_t) + 4);
	for (int i = 0; i < count; ++i) {
		offsetsTable[i].offset = fileReadUint32LE(fp) + baseOffset;
	}
	for (int i = 0; i < count; ++i) {
		fileRead(fp, offsetsTable[i].ext, sizeof(offsetsTable[i].ext));
		const uint32_t nextOffset = (i < count - 1) ? offsetsTable[i + 1].offset : dataSize;
		offsetsTable[i].size = nextOffset - offsetsTable[i].offset;
	}
}

uint32_t ResourcePsx::seekData(const char *ext, File *fp, int offsetType, uint32_t offset) {
	assert(offsetType == kResOffsetType_SON);
	for (int i = 0; i < kResPsxSonOffsetsTableSize; ++i) {
		if (strcmp(_sonOffsetsTable[i].ext, ext) == 0) {
			fileSetPos(fp, _sonOffsetsTable[i].offset + offset, kFilePosition_SET);
			return _sonOffsetsTable[i].size;
		}
	}
	return 0;
}

void ResourcePsx::loadVAB(File *fp) {
	_vagOffsetsTableSize = 0;
	memset(_vagOffsetsTable, 0, sizeof(_vagOffsetsTable));

	const uint32_t vhSize = seekData("VH", fp, kResOffsetType_SON);
	if (vhSize == 0) {
		warning("'VH' data resource not found");
		return;
	}
	uint8_t header[32];
	fileRead(fp, header, sizeof(header));
	if (memcmp(header, "pBAV", 4) != 0 || READ_LE_UINT32(header + 4) != 7) {
		warning("Unexpected VAB data header '%08x'", READ_LE_UINT32(header));
		return;
	}
	const int programsCount = READ_LE_UINT16(header + 18);
	const int vagCount = READ_LE_UINT16(header + 22);
	// const uint8_t volume = header[24];
	// const uint8_t pan = header[25];

	// skip the audio attribute and tone attribute tables
	fileSetPos(fp, 16 * 128 + 32 * 16 * programsCount, kFilePosition_CUR);

	uint32_t vbOffset = 0;
	for (int i = 0; i < vagCount; ++i) {
		const uint32_t vbSize = fileReadUint16LE(fp) << 3;
		_vagOffsetsTable[i].offset = vbOffset;
		_vagOffsetsTable[i].size = vbSize;
		vbOffset += vbSize;
	}
	_vagOffsetsTableSize = vagCount;
}

void ResourcePsx::loadVRM(File *fp) {
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
						const uint16_t color = fileReadUint16LE(fp);
						*dst++ = ( color        & 31) << 3; // r
						*dst++ = ((color >>  5) & 31) << 3; // g
						*dst++ = ((color >> 10) & 31) << 3; // b
						*dst++ = 255;
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
}

uint32_t ResourcePsx::seekVag(int num) {
	if (num >= 0 && num < _vagOffsetsTableSize) {
		const uint32_t vbSize = seekData("VB", _fileSon, kResOffsetType_SON, _vagOffsetsTable[num].offset);
		if (vbSize != 0) {
			return _vagOffsetsTable[num].size;
		}
		warning("'VB' data resource not found");
	}
	return 0;
}
