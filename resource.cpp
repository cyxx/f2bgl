/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include <math.h>
#include "file.h"
#include "trigo.h"
#include "resource.h"

static const char *_levelsPsx[] = {
	"1", "2a", "2b", "2c", "3", "4a", "4b", "4c", "5a", "5b", "5c", "6a", "6b"
};

// indexed by FileLanguage
static char _languagesPsx[] = {
	'u', 'f', 'g', 's', 'i', 'j'
};

static const bool kLoadPsxData = false;

Resource::Resource() {
	memset(_treesTable, 0, sizeof(_treesTable));
	memset(_treesTableCount, 0, sizeof(_treesTableCount));
	_msgOffsetsTableCount = 0;
	_msgOffsetsTable = 0;
	_msgData = 0;
	_cmdOffsetsTableCount = 0;
	_cmdOffsetsTable = 0;
	_cmdData = 0;
	_objectIndexesTableCount = 0;
	_objectIndexesTable = 0;
	_objectTextDataSize = 0;
	_objectTextData = 0;
	_keyPathsTableCount = 0;
	memset(_keyPathsTable, 0, sizeof(_keyPathsTable));
	_envAniDataCount = 0;
	_envAniData = 0;
	memset(_levelDescriptionsTable, 0, sizeof(_levelDescriptionsTable));
	memset(_soundKeyPathsTable, 0, sizeof(_soundKeyPathsTable));
	memset(_sndKeysTable, 0, sizeof(_sndKeysTable));
	memset(_musicKeyPathsTable, 0, sizeof(_musicKeyPathsTable));
	memset(_musicKeysTable, 0, sizeof(_musicKeysTable));
	_demoInputDataSize = 0;
	_demoInputData = 0;
	memset(&_userConfig, 0, sizeof(_userConfig));
	memset(_gusPatches, 0, sizeof(_gusPatches));
	_lastObjectKey = -1;
	_textIndexesTableCount = 0;
	_textIndexesTable = 0;
	_vrmLoadingBitmap = 0;
	_vagOffsetsTableSize = 0;
	memset(_levOffsetsTable, 0, sizeof(_levOffsetsTable));
	_fileLev = 0;
	memset(_sonOffsetsTable, 0, sizeof(_sonOffsetsTable));
	_fileSon = 0;
}

Resource::~Resource() {
	// TODO - reclaimed by the OS when the program exits
}

void Resource::loadCMD(File *fp, int dataSize) {
	_cmdOffsetsTableCount = fileReadUint32LE(fp);
	debug(kDebug_RESOURCE, "Resource::loadCMD(%d) count = %d", dataSize, _cmdOffsetsTableCount);

	free(_cmdOffsetsTable);
	_cmdOffsetsTable = ALLOC<uint32_t>(_cmdOffsetsTableCount);
	for (uint32_t i = 0; i < _cmdOffsetsTableCount; ++i) {
		_cmdOffsetsTable[i] = fileReadUint32LE(fp);
	}

	dataSize -= 4 + _cmdOffsetsTableCount * 4;

	free(_cmdData);
	_cmdData = ALLOC<uint8_t>(dataSize);
	fileRead(fp, _cmdData, dataSize);
}

void Resource::loadMSG(File *fp, int dataSize) {
	_msgOffsetsTableCount = fileReadUint16LE(fp);
	debug(kDebug_RESOURCE, "Resource::loadMSG(%d) count = %d", dataSize, _msgOffsetsTableCount);

	free(_msgOffsetsTable);
	_msgOffsetsTable = ALLOC<uint16_t>(_msgOffsetsTableCount);
	for (uint16_t i = 0; i < _msgOffsetsTableCount; ++i) {
		_msgOffsetsTable[i] = fileReadUint16LE(fp);
	}

	dataSize -= 2 + _msgOffsetsTableCount * 2;

	free(_msgData);
	_msgData = ALLOC<uint8_t>(dataSize);
	fileRead(fp, _msgData, dataSize);
}

void Resource::loadENV(File *fp, int dataSize) {
	_envAniDataCount = fileReadUint32LE(fp);
	debug(kDebug_RESOURCE, "Resource::loadENV(%d) count = %d", dataSize, _envAniDataCount);
	dataSize -= 4;

	free(_envAniData);
	_envAniData = ALLOC<uint8_t>(_envAniDataCount * kEnvAniDataSize);
	assert(dataSize == _envAniDataCount * kEnvAniDataSize);
	fileRead(fp, _envAniData, dataSize);
}

static int resCompareIndexByObjectName(const void *p1, const void *p2) {
	const ResObjectIndex *obj1 = (const ResObjectIndex *)p1;
	const ResObjectIndex *obj2 = (const ResObjectIndex *)p2;
	return strcmp(obj1->objectName, obj2->objectName);
}

void Resource::loadObjectIndexes(File *fp, int dataSize) {
	free(_objectIndexesTable);

	assert((dataSize % (64 + 4)) == 0);
	uint32_t count = dataSize / (64 + 4);
	_objectIndexesTable = ALLOC<ResObjectIndex>(count);
	_objectIndexesTableCount = count;
	for (uint32_t i = 0; i < count; ++i) {
		ResObjectIndex *objectIndex = &_objectIndexesTable[i];
		fileRead(fp, objectIndex->objectName, 64);
		objectIndex->objectKey = 0;
		objectIndex->dataOffs = fileReadUint32LE(fp);
	}
	qsort(_objectIndexesTable, _objectIndexesTableCount, sizeof(ResObjectIndex), resCompareIndexByObjectName);
}

void Resource::loadObjectText(File *fp, int dataSize, int levelNum) {
	free(_objectTextData);
	_objectTextData = ALLOC<uint8_t>(dataSize);
	_objectTextDataSize = dataSize;
	fileRead(fp, _objectTextData, dataSize);

	if (!g_hasPsx && fileLanguage() == kFileLanguage_SP && levelNum == 5) {

		// fix Spanish translation 'DESACTIVADOS' to 'ACTIVADOS'
		//
		//  sp : message 0 offset 0x625a value 0 len -43 'PANELES DEL SUELO DESACTIVADOS'
		//  en : message 0 offset 0x5828 value 0 len -35 'FLOOR PANELS ACTIVATED'
		//  fr : message 0 offset 0x6035 value 0 len -28 'DALLES ACTIVEES'

		uint8_t *p = _objectTextData + 0x625a;
		if (READ_LE_UINT32(p + 4) == 1) {
			if ((int32_t)READ_LE_UINT32(p + 12) == -43) {
				strcpy((char *)p + 28, "PANELES DEL SUELO ACTIVADOS");
			}
		}
	}
	if (0) {
		int offset = 0;
		while (offset < _objectTextDataSize) {
			const uint8_t *p = _objectTextData + offset;
			const int groupSize = READ_LE_UINT32(p); p += 4;
			const int messagesCount = READ_LE_UINT32(p); p += 4;
			for (int i = 0; i < messagesCount; ++i) {
				const uint32_t value = READ_LE_UINT32(p); p += 4;
				const int32_t len = (int32_t)READ_LE_UINT32(p); p += 4;
				debug(kDebug_RESOURCE, "message %d offset 0x%x value %d len %d '%s'", i, offset, value, len, &p[12]);
				p += ABS(len);
			}
			offset += groupSize + 4;
		}
	}
}

static int resCompareKeyPaths(const void *p1, const void *p2) {
	const ResKeyPath *obj1 = (const ResKeyPath *)p1;
	const ResKeyPath *obj2 = (const ResKeyPath *)p2;
	return strcmp(obj1->pathName, obj2->pathName);
}

static int resSearchKeyPath(const void *p1, const void *p2) {
	const char *pathName = (const char *)p1;
	const ResKeyPath *keyPath = (const ResKeyPath *)p2;
	return strcmp(pathName, keyPath->pathName);
}

void Resource::loadKeyPaths(File *fp, int dataSize) {
	_keyPathsTableCount = 0;
	memset(_keyPathsTable, 0, sizeof(_keyPathsTable));

	char *kpData = ALLOC<char>(dataSize + 1);
	fileRead(fp, kpData, dataSize);
	kpData[dataSize] = '\0';

	char *kpCur, *kpBuf = kpData;
	while ((kpCur = stringNextToken(&kpBuf)) && *kpCur) {
		assert(_keyPathsTableCount < kKeyPathsTableSize);
		ResKeyPath *keyPath = &_keyPathsTable[_keyPathsTableCount];
		++_keyPathsTableCount;
		strncpy(keyPath->pathName, kpCur, kKeyPathNameLength);
		keyPath->pathName[kKeyPathNameLength - 1] = '\0';
		kpCur = stringNextToken(&kpBuf);
		assert(kpCur);
		keyPath->key = strtol(kpCur, 0, 10);
	}
	free(kpData);

	debug(kDebug_RESOURCE, "_keyPathsTableCount %d", _keyPathsTableCount);
	qsort(_keyPathsTable, _keyPathsTableCount, sizeof(ResKeyPath), resCompareKeyPaths);
}

void Resource::loadINI(File *fp, int dataSize) {
	char *iniData = ALLOC<char>(dataSize + 1);
	fileRead(fp, iniData, dataSize);
	iniData[dataSize] = '\0';

	char *iniDataToken, *iniDataBuf = iniData;

	int levelsCount = 0;
	if ((iniDataToken = stringNextToken(&iniDataBuf)) && *iniDataToken) {
		levelsCount = strtol(iniDataToken, 0, 10);
	}
	assert(levelsCount == kLevelDescriptionsCount);
	for (int l = 0; l < levelsCount; ++l) {
		ResLevelDescription *levelDesc = &_levelDescriptionsTable[l];
		if ((iniDataToken = stringNextToken(&iniDataBuf)) && *iniDataToken) {
			strncpy(levelDesc->palKey, iniDataToken, kKeyPathNameLength - 1);
			levelDesc->palKey[kKeyPathNameLength - 1] = '\0';
		}
		if ((iniDataToken = stringNextToken(&iniDataBuf)) && *iniDataToken) {
			strncpy(levelDesc->mapKey, iniDataToken, kKeyPathNameLength - 1);
			levelDesc->mapKey[kKeyPathNameLength - 1] = '\0';
		}
		if ((iniDataToken = stringNextToken(&iniDataBuf)) && *iniDataToken) {
			strncpy(levelDesc->name, iniDataToken, sizeof(levelDesc->name) - 1);
			levelDesc->name[sizeof(levelDesc->name) - 1] = '\0';
		}
		for (int skipCount = 0; skipCount < 9; ++skipCount) {
			stringNextToken(&iniDataBuf);
		}
		if ((iniDataToken = stringNextToken(&iniDataBuf)) && *iniDataToken) {
			levelDesc->inventory = strtol(iniDataToken, 0, 10) != 0;
		}
		if ((iniDataToken = stringNextToken(&iniDataBuf)) && *iniDataToken) {
			levelDesc->map = strtol(iniDataToken, 0, 10) != 0;
		}
	}

	int soundsCount = 0;
	if ((iniDataToken = stringNextToken(&iniDataBuf)) && *iniDataToken) {
		soundsCount = strtol(iniDataToken, 0, 10);
	}
	assert(soundsCount <= kSoundKeyPathsTableSize);
	for (int i = 0; i < soundsCount; ++i) {
		if ((iniDataToken = stringNextToken(&iniDataBuf)) && *iniDataToken) {
			strncpy(_soundKeyPathsTable[i], iniDataToken, kKeyPathNameLength - 1);
			_soundKeyPathsTable[i][kKeyPathNameLength - 1] = '\0';
		}
	}

	int musicsCount = 0;
	if ((iniDataToken = stringNextToken(&iniDataBuf)) && *iniDataToken) {
		musicsCount = strtol(iniDataToken, 0, 10);
	}
	assert(musicsCount <= kMusicKeyPathsTableSize);
	for (int i = 0; i < musicsCount; ++i) {
		stringNextToken(&iniDataBuf);
		if ((iniDataToken = stringNextToken(&iniDataBuf)) && *iniDataToken) {
			strncpy(_musicKeyPathsTable[i], iniDataToken, kKeyPathNameLength - 1);
			_musicKeyPathsTable[i][kKeyPathNameLength - 1] = '\0';
		}
	}

	levelsCount = 0;
	if ((iniDataToken = stringNextToken(&iniDataBuf)) && *iniDataToken) {
		levelsCount = strtol(iniDataToken, 0, 10);
	}
	assert(levelsCount == kLevelDescriptionsCount);
	for (int l = 0; l < levelsCount; ++l) {
		int level = -1;
		if ((iniDataToken = stringNextToken(&iniDataBuf)) && *iniDataToken) {
			level = strtol(iniDataToken, 0, 10);
		}
		assert(level == l);
		ResLevelDescription *levelDesc = &_levelDescriptionsTable[level];
		for (int i = 0; i < (g_isDemo ? 11 : kLevelMusicTableSize); ++i) {
			int index = -1;
			if ((iniDataToken = stringNextToken(&iniDataBuf)) && *iniDataToken) {
				index = strtol(iniDataToken, 0, 10);
			}
			assert(index == i);
			int num = -1;
			if ((iniDataToken = stringNextToken(&iniDataBuf)) && *iniDataToken) {
				num = strtol(iniDataToken, 0, 10);
				assert(num >= 0 && num < kMusicKeyPathsTableSize);
			}
			levelDesc->musicKeys[i] = num;
		}
	}
	free(iniData);
}

void Resource::loadTrigo() {
	if (fileExists("TRIGO.DAT", kFileType_RUNTIME)) {
		int dataSize;
		File *fp = fileOpen("TRIGO.DAT", &dataSize, kFileType_RUNTIME);
		assert(dataSize == 1024 * 8 + 256 * 4);
		for (int i = 0; i < 1024; ++i) {
			g_sin[i] = fileReadUint32LE(fp);
			g_cos[i] = fileReadUint32LE(fp);
		}
		for (int i = 0; i < 256; ++i) {
			g_atan[i] = fileReadUint32LE(fp);
		}
		fileClose(fp);
	} else {
		for (int i = 0; i < 1024; ++i) {
			const double a = i * 2 * M_PI / 1024.;
			g_sin[i] = (int32_t)(sin(a) * 32767);
			g_cos[i] = (int32_t)(cos(a) * 32767);
		}
		g_atan[0] = 0;
		for (int i = 1; i < 256; ++i) {
			g_atan[i] = (int32_t)(atan(i / 256.) * (1024. / 2) / M_PI);
		}
	}
}

void Resource::loadINM(int levelNum) {
	_textIndexesTableCount = 0;
	free(_textIndexesTable);
	_textIndexesTable = 0;

	char filename[32];
	File *fp = 0;
	int dataSize = 0;
	if (kLoadPsxData && g_hasPsx) {
		snprintf(filename, sizeof(filename), "level%d%c.lev", levelNum + 1, _languagesPsx[fileLanguage()]);
		fp = fileOpenPsx(filename, kFileType_PSX_LEVELDATA, levelNum + 1);
		if (!fp) {
			error("Resource::loadINM() Unable to open '%s'", filename);
		}
		dataSize = fileSize(fp);
	} else {
		snprintf(filename, sizeof(filename), "%s.inm", _levelDescriptionsTable[levelNum].name);
		if (fileExists(filename, kFileType_TEXT)) {
			fp = fileOpen(filename, &dataSize, kFileType_TEXT);
		}
	}
	if (fp) {
		_textIndexesTableCount = dataSize / sizeof(uint32_t);
		_textIndexesTable = ALLOC<uint32_t>(_textIndexesTableCount);
		for (uint32_t i = 0; i < _textIndexesTableCount; ++i) {
			_textIndexesTable[i] = fileReadUint32LE(fp);
		}
		fileClose(fp);
	} else {
		_textIndexesTableCount = _lastObjectKey + 1;
		_textIndexesTable = ALLOC<uint32_t>(_textIndexesTableCount);
		for (uint32_t i = 0; i < _textIndexesTableCount; ++i) {
			_textIndexesTable[i] = 0xFFFFFFFF;
		}
		for (uint32_t i = 0; i < _objectIndexesTableCount; ++i) {
			int16_t key = _objectIndexesTable[i].objectKey;
			assert(key > 0 && key < _textIndexesTableCount);
			_textIndexesTable[key - 1] = _objectIndexesTable[i].dataOffs;
		}
	}
}

static uint8_t *convertANI(uint8_t *p, uint32_t *size) {
	switch (*size) {
	case 8: // "ANIFRAM"
		p[2] = p[6];
		p[3] = p[7];
		*size = 4;
		break;
	case 22: { // "ANIHEAD"
			uint8_t tmp[22];
			memcpy(tmp, p, 22);
			assert(tmp[1] == 0);
			p[0] = tmp[0];
//			assert(tmp[3] == 0);
			p[1] = tmp[2];
			static const int offsets[] = { 4, 8, 14, 2, 18, 4, -1 };
			int base = 2;
			for (int i = 0; offsets[i] != -1; i += 2) {
				memcpy(p + base, tmp + offsets[i], offsets[i + 1]);
				base += offsets[i + 1];
			}
		}
		*size = 16;
		break;
	case 20: { // "ANIKEYF"
			const int16_t anikeyf_dx = (int16_t)READ_LE_UINT16(p + 2);
			const int16_t anikeyf_dz = (int16_t)READ_LE_UINT16(p + 6);

			// from: int16_t shootX, shootY, shootZ;
			// to: int16_t shootY; int8_t shootX, shootZ;

			const int16_t shoot_x = (int16_t)READ_LE_UINT16(p + 14);
			const int16_t shoot_y = (int16_t)READ_LE_UINT16(p + 16);
			const int16_t shoot_z = (int16_t)READ_LE_UINT16(p + 18);

			p[14] = shoot_y & 255; // shootY
			p[15] = shoot_y >> 8;

			p[16] = (shoot_x - anikeyf_dx) / 8; // shootX
			p[17] = (shoot_z - anikeyf_dz) / 8; // shootZ
		}
		*size = 18;
		break;
	}
	return p;
}

static uint8_t *convertSTM(uint8_t *p, uint32_t *size) {
	if (*size == 8) { // "STMSTATE"
		const int msgNum = READ_LE_UINT16(p + 2);
		assert(msgNum < 256);
		const int msgCount = READ_LE_UINT16(p + 4);
		assert(msgCount < 256);
		p[2] = msgNum;
		p[3] = msgCount;
		*size = 4;
	}
	return p;
}

static uint8_t *convertOBJ(uint8_t *p, uint32_t *size) {
	if (*size == 304) { // "OBJ"
		memmove(p + 176, p + 184, 304 - 184);
		*size = 296;
	}
	return p;
}

static const struct {
	const char *ext;
	int type;
	uint8_t *(*convert)(uint8_t *p, uint32_t *size);
} _resLoadDataTable[] = {
	{ "spr", kResType_SPR, 0 },
	{ "pal", kResType_PAL, 0 },
	{ "map", kResType_MAP, 0 },
	{ "ani", kResType_ANI, &convertANI },
	{ "stm", kResType_STM, &convertSTM },
	{ "obj", kResType_OBJ, &convertOBJ },
	{ "snd", kResType_SND, 0 },
	{ "f3d", kResType_F3D, 0 },
	{ "p3d", kResType_P3D, 0 }
};

static const struct {
	const char *ext;
	void (Resource::*LoadData)(File *fp, int dataSize);
} _resLoadDataTable2[] = {
	{ "cmd", &Resource::loadCMD },
	{ "msg", &Resource::loadMSG }
};

void Resource::loadLevelData(int levelNum) {
	File *fp;
	int dataSize;
	char filename[32];

	const char *levelName = _levelDescriptionsTable[levelNum].name;
	for (uint32_t i = 0; i < ARRAYSIZE(_resLoadDataTable); ++i) {
		snprintf(filename, sizeof(filename), "%s.%s", levelName, _resLoadDataTable[i].ext);
		int type = _resLoadDataTable[i].type;
		fp = fileOpen(filename, &dataSize, kFileType_DATA);
		uint32_t count = fileReadUint32LE(fp);

		debug(kDebug_RESOURCE, "Resource::loadLevelData() file '%s' type %d count %d", filename, type, count);

		// free previously loaded data
		for (uint32_t j = 0; j < _treesTableCount[type]; ++j) {
			ResTreeNode *node = &_treesTable[type][j];
			free(node->data);
			memset(node, 0, sizeof(ResTreeNode));
		}
		free(_treesTable[type]);

		// load new level data
		_treesTable[type] = ALLOC<ResTreeNode>(count);
		_treesTableCount[type] = count;
		for (uint32_t j = 0; j < count; ++j) {
			ResTreeNode *node = &_treesTable[type][j];
			memset(node, 0, sizeof(ResTreeNode));
			const uint32_t offs = fileReadUint32LE(fp);
			const uint32_t size = fileReadUint32LE(fp);
			node->childKey = fileReadUint16LE(fp);
			node->nextKey = fileReadUint16LE(fp);
			node->dataOffset = 4 + count * 12 + offs;
			node->dataSize = size;
		}
		for (uint32_t j = 0; j < count; ++j) {
			ResTreeNode *node = &_treesTable[type][j];
			if (node->dataSize != 0) {
				node->data = (uint8_t *)malloc(node->dataSize);
				if (node->data) {
					fileSetPos(fp, node->dataOffset, kFilePosition_SET);
					fileRead(fp, node->data, node->dataSize);
					if (g_isDemo && _resLoadDataTable[i].convert) {
						node->data = _resLoadDataTable[i].convert(node->data, &node->dataSize);
					}
				}
			}
		}
		fileClose(fp);
	}

	for (uint32_t i = 0; i < ARRAYSIZE(_resLoadDataTable2); ++i) {
		snprintf(filename, sizeof(filename), "level%d.%s", levelNum + 1, _resLoadDataTable2[i].ext);
		fp = fileOpen(filename, &dataSize, kFileType_DATA);
		(this->*_resLoadDataTable2[i].LoadData)(fp, dataSize);
		fileClose(fp);
	}

	_conradVoiceCmdNum = -1;
	patchCmdData(levelNum + 1);

	_lastObjectKey = -1;

	snprintf(filename, sizeof(filename), "%s.env", levelName);
	fp = fileOpen(filename, &dataSize, kFileType_DATA);
	loadENV(fp, dataSize);
	fileClose(fp);

	snprintf(filename, sizeof(filename), "%s.ini", levelName);
	fp = fileOpen(filename, &dataSize, kFileType_DATA);
	loadKeyPaths(fp, dataSize);
	fileClose(fp);

	snprintf(filename, sizeof(filename), "%s.snt", levelName);
	fp = fileOpen(filename, &dataSize, kFileType_TEXT);
	loadObjectIndexes(fp, dataSize);
	fileClose(fp);

	snprintf(filename, sizeof(filename), "%s.dtt", levelName);
	fp = fileOpen(filename, &dataSize, kFileType_TEXT);
	loadObjectText(fp, dataSize, levelNum + 1);
	fileClose(fp);
}

void Resource::unload(int type, int16_t key) {
	assert(key > 0 && key < _treesTableCount[type]);
	ResTreeNode *node = &_treesTable[type][key];
	if (node->data) {
		free(node->data);
		node->data = 0;
	}
	node->dataSize = 0;
}

int16_t Resource::getPrevious(int type, int16_t key) {
	if (key > 0 && key < _treesTableCount[type]) {
		for (int16_t k = key - 1; k > 0; --k) {
			if (_treesTable[type][k].nextKey == key) {
				return k;
			}
		}
	}
	return 0;
}

int16_t Resource::getNext(int type, int16_t key) {
	debug(kDebug_RESOURCE, "Resource::getNext() type %d key %d", type, key);
	if (key != 0 && key < _treesTableCount[type]) {
		return _treesTable[type][key].nextKey;
	}
	return 0;
}

int16_t Resource::getChild(int type, int16_t key) {
	debug(kDebug_RESOURCE, "Resource::getChild() type %d key %d", type, key);
	if (key != 0 && key < _treesTableCount[type]) {
		return _treesTable[type][key].childKey;
	}
	return 0;
}

int16_t Resource::getRoot(int type) {
	debug(kDebug_RESOURCE, "Resource::getRoot() type %d", type);
	return 1;
}

static const struct {
	const char *name;
	uint32_t offset;
} _resDataOffsetTable[] = {
	/* .spr */
	{ "BTMDESC", 0 },
	{ "SPRDATA", 6 },
	/* .obj */
	{ "OBJ", 0 },
	/* .ani */
	{ "ANIFRAM", 0 },
	{ "ANIKEYF", 0 },
	{ "ANIHEAD", 0 },
	/* .stm */
	{ "STMHEADE", 0 },
	{ "STMSTATE", 0 },
	{ "STMCOND", 0 },
	/* .f3d */
	{ "FORM3D", 0 },
	{ "F3DDATA", 20 },
	/* .p3d */
	{ "POLY3D", 0 },
	{ "P3DDATA", 20 },
	/* .map */
	{ "MAP3D", 0 },
	{ "MAPDATA", 64 },
	{ "GDATA", 81984 },
	{ "ANIDATA", 98368 },
	{ "TEX3D", 0 },
	{ "TEX3DANI", 4 },
	/* .pal */
	{ "MRKCOLOR", 0 },
	{ "PALDATA", 0 },
	/* .snd */
	{ "SNDTYPE", 0 },
	{ "SNDINFO", 4 },
	{ "SNDDATA", 104 }
};

uint8_t *Resource::getEnvAni(int16_t key, int num) {
	uint8_t *p = _envAniData;
	for (int i = 0; i < _envAniDataCount; ++i) {
		if (READ_LE_UINT16(p) == key && READ_LE_UINT16(p + 2) == num) {
			return p + 4;
		}
		p += kEnvAniDataSize;
	}
	return 0;
}

uint8_t *Resource::getData(int type, int16_t key, const char *name) {
	assert(name);
	uint32_t i, offset = 0;
	debug(kDebug_RESOURCE, "Resource::getData() type %d key %d/%d name '%s'", type, key, _treesTableCount[type], name ? name : "()");
	assert(key != 0 && key < _treesTableCount[type]);
	uint8_t *data = _treesTable[type][key].data;
	assert(type == kResType_ANI || type == kResType_P3D || (data && _treesTable[type][key].dataSize != 0));
	if (strcmp(name, "CAMDATA") == 0) {
		offset = READ_LE_UINT32(data + 12) * 52 + 98368;
	} else if (data) {
		for (i = 0; i < ARRAYSIZE(_resDataOffsetTable); ++i) {
			if (strcmp(_resDataOffsetTable[i].name, name) == 0) {
				offset = _resDataOffsetTable[i].offset;
				break;
			}
		}
		if (i == ARRAYSIZE(_resDataOffsetTable)) {
			error("Resource::getData() Unhandled resource data type %d name %s", type, name);
		}
	}
	debug(kDebug_RESOURCE, "Resource::getData() dataSize %d offset 0x%X", _treesTable[type][key].dataSize, offset);
	return data + offset;
}

static int resSearchIndex(const void *p1, const void *p2) {
	const char *objName = (const char *)p1;
	const ResObjectIndex *obj = (const ResObjectIndex *)p2;
	return strcmp(objName, obj->objectName);
}

void Resource::setObjectKey(const char *objectName, int16_t objectKey) {
	debug(kDebug_RESOURCE, "Resource::setObjectKey() name '%s' key %d", objectName, objectKey);
	ResObjectIndex *objectIndex = (ResObjectIndex *)bsearch(objectName, _objectIndexesTable, _objectIndexesTableCount, sizeof(ResObjectIndex), resSearchIndex);
	if (objectIndex) {
		objectIndex->objectKey = objectKey;
	}
	if (_lastObjectKey < objectKey) {
		_lastObjectKey = objectKey;
	}
}

int Resource::getOffsetForObjectKey(int16_t objectKey) {
	debug(kDebug_RESOURCE, "Resource::getOffsetForObjectKey() key %d", objectKey);
	if (objectKey < _textIndexesTableCount) {
		assert(objectKey > 0);
		return (int32_t)_textIndexesTable[objectKey - 1];
	}
	for (int i = 0; i < _objectIndexesTableCount; ++i) {
		ResObjectIndex *objectIndex = &_objectIndexesTable[i];
		if (objectKey == objectIndex->objectKey) {
			return objectIndex->dataOffs;
		}
	}
	return -1;
}

int16_t Resource::getKeyFromPath(const char *path) {
	int16_t key = 0;
	ResKeyPath *keyPath = (ResKeyPath *)bsearch(path, _keyPathsTable, _keyPathsTableCount, sizeof(ResKeyPath), resSearchKeyPath);
	if (keyPath) {
		key = keyPath->key;
	}
	debug(kDebug_RESOURCE, "Resource::getKeyFromPath('%s') key %d", path, key);
	return key;
}

const uint8_t *Resource::getCmdData(int num) {
	debug(kDebug_RESOURCE, "Resource::getCmdData() num %d", num);
	assert(num >= 0 && num < _cmdOffsetsTableCount);
	const uint8_t *p = _cmdData + _cmdOffsetsTable[num];
	if (g_isDemo) {
		p += 4; // skip length
	}
	return p;
}

const uint8_t *Resource::getMsgData(int num) {
	debug(kDebug_RESOURCE, "Resource::getMsgData() num %d", num);
	assert(num >= 0 && num < _msgOffsetsTableCount);
	return _msgData + _msgOffsetsTable[num];
}

bool Resource::getMessageDescription(ResMessageDescription *m, uint32_t value, uint32_t offset) {
	const uint8_t *p = _objectTextData + offset;
	/*int groupSize = READ_LE_UINT32(p);*/ p += 4;
	int messagesCount = READ_LE_UINT32(p); p += 4;
	for (int i = 0; i < messagesCount; ++i) {
		uint32_t val = READ_LE_UINT32(p); p += 4;
		int32_t len = (int32_t)READ_LE_UINT32(p); p += 4;
		if (val == value) {
			m->frameSync = READ_LE_UINT16(p); p += 2;
			m->duration = READ_LE_UINT16(p); p += 2;
			m->xPos = READ_LE_UINT16(p); p += 2;
			m->yPos = READ_LE_UINT16(p); p += 2;
			m->font = READ_LE_UINT32(p); p += 4;
			m->data = p;
			return true;
		}
		// last message has negative length
		p += ABS(len);
	}
	return false;
}

void Resource::patchCmdData(int levelNum) {
	if (g_isDemo) {
		return;
	}
	if (levelNum == 1) {
		//
		// Some messages are randomly displayed in Level 1. It is unclear what was the original intent.
		//
		// script 328 (0x7004) - COND rnd( 3 ) STMT msg ( 1, 146 ) : 'order to activate security drones'
		// script 399 (0x8280) - COND rnd( 5 ) STMT msg ( 1, 152 ) : 'secure all exits'
		//
		// The messages are now always displayed. rnd(x) calls are replaced with rnd(1) (eg. 'true')
		//
		static const int kCmd328 = 328;
		if (kCmd328 < _cmdOffsetsTableCount) {
			uint8_t *p = _cmdData + _cmdOffsetsTable[kCmd328];
			if (READ_LE_UINT32(p + 0x30) == 0x13 && READ_LE_UINT32(p + 0x34) == 0 && READ_LE_UINT32(p + 0x38) == 3) {
				p[0x38] = 1;
			}
		}
		static const int kCmd399 = 399;
		if (kCmd399 < _cmdOffsetsTableCount) {
			uint8_t *p = _cmdData + _cmdOffsetsTable[kCmd399];
			if (READ_LE_UINT32(p + 0x24) == 0x13 && READ_LE_UINT32(p + 0x28) == 0 && READ_LE_UINT32(p + 0x2C) == 5) {
				p[0x2C] = 1;
			}
		}
	}

	//
	// Conrad mocking voices are conditionned by a NOT rnd ( 20 ) which is quite unlikely to occur.
	//
	// LEVEL1.CMD  script 835 (0x11E94) - COND NOT rnd ( 20 ) STMT set_obj ( 0, 267, 2 )
	// LEVEL2.CMD  script 712 (0xFA78)
	// LEVEL3.CMD  script 695 (0xF6F0)
	// LEVEL4.CMD  script 721 (0xFF20)
	// LEVEL5.CMD  script 949 (0x148EC)
	// LEVEL6.CMD  script 513 (0xBCC8)
	// LEVEL7.CMD  script 457 (0xAC38)
	// LEVEL8.CMD  script 837 (0x12830)
	// LEVEL9.CMD  script 873 (0x1314C)
	// LEVEL10.CMD script 701 (0xF6B8)
	// LEVEL11.CMD script 551 (0xCA64)
	// LEVEL12.CMD script 778 (0x11628)
	//
	// As the voices are in the datafiles, it is possible to re-enable them. The condition is changed to
	// NOT rnd( 1 ) to always skip the set_obj statement and execute the following voiced statments.
	//

	for (int i = 0; i < _cmdOffsetsTableCount; ++i) {
		uint8_t *p = _cmdData + _cmdOffsetsTable[i];
		if (READ_LE_UINT32(p) == 0x93 && READ_LE_UINT32(p + 4) == 0 && READ_LE_UINT32(p + 8) == 20) {
			p[8] = 1;
			debug(kDebug_RESOURCE, "Found mocking voice check cmd %d", i);
			_conradVoiceCmdNum = i;
		}
	}
}

void Resource::loadDEM(File *fp, int dataSize) {
	_demoInputDataSize = dataSize / 8;
	free(_demoInputData);
	_demoInputData = ALLOC<ResDemoInput>(_demoInputDataSize);
	for (int i = 0; i < _demoInputDataSize; ++i) {
		ResDemoInput *input = &_demoInputData[i];
		input->ticks = fileReadUint32LE(fp);
		input->key = fileReadUint16LE(fp);
		input->pressed = fileReadUint16LE(fp) != 0;
	}
}

static int convertParameterYesNo(const char *value) {
	if (strncmp(value, "YES", 3) == 0) {
		return 1;
	}
	return 0;
}

static int convertParameterOnOff(const char *value) {
	if (strncmp(value, "ON", 2) == 0) {
		return 1;
	}
	return 0;
}

static int convertParameterPlayLevel(const char *value) {
	if (strncmp(value, "EASY", 4) == 0) {
		return kSkillEasy;
	} else if (strncmp(value, "HARD", 4) == 0) {
		return kSkillHard;
	}
	return kSkillNormal;
}

void Resource::loadDelphineINI() {
	int dataSize;
	File *fp = fileOpen("DELPHINE.INI", &dataSize, kFileType_RUNTIME);

	char *iniData = ALLOC<char>(dataSize + 1);
	fileRead(fp, iniData, dataSize);
	iniData[dataSize] = '\0';

	fileClose(fp);

	struct {
		const char *name;
		int *p;
		int (*convert)(const char *);
	} parameters[] = {
		{ "GREY_SCALE", &_userConfig.greyScale, convertParameterYesNo },
		{ "LIGHT_COEF", &_userConfig.lightCoef, 0 },
		{ "SUB_TITLES", &_userConfig.subtitles, convertParameterYesNo },
		{ "ICON_LR_MOVE_X", &_userConfig.iconLrMoveX, 0 },
		{ "ICON_LR_MOVE_Y", &_userConfig.iconLrMoveY, 0 },
		{ "ICON_LR_STEP_X", &_userConfig.iconLrStepX, 0 },
		{ "ICON_LR_STEP_Y", &_userConfig.iconLrStepY, 0 },
		{ "ICON_LR_TOOLS_X", &_userConfig.iconLrToolsX, 0 },
		{ "ICON_LR_TOOLS_Y", &_userConfig.iconLrToolsY, 0 },
		{ "ICON_LR_CAB_X", &_userConfig.iconLrCabX, 0 },
		{ "ICON_LR_CAB_Y", &_userConfig.iconLrCabY, 0 },
		{ "ICON_LR_INV_X", &_userConfig.iconLrInvX, 0 },
		{ "ICON_LR_INV_Y", &_userConfig.iconLrInvY, 0 },
		{ "ICON_LR_SCAN_X", &_userConfig.iconLrScanX, 0 },
		{ "ICON_LR_SCAN_Y", &_userConfig.iconLrScanY, 0 },
		{ "PLAY_LEVEL", &_userConfig.skillLevel, convertParameterPlayLevel },
		{ "NOISE_VOLUME", &_userConfig.soundVolume, 0 },
		{ "MUSIC_VOLUME", &_userConfig.musicVolume, 0 },
		{ "VOICE_VOLUME", &_userConfig.voiceVolume, 0 },
		{ "MUSIC", &_userConfig.musicOn, convertParameterOnOff },
		{ "SOUND", &_userConfig.soundOn, convertParameterOnOff },
		{ "VOICE", &_userConfig.voiceOn, convertParameterOnOff },
		{ 0, 0 }
	};

	for (char *p = iniData; *p; ) {
		char *next = strchr(p, '\n');
		if (p[0] != '#') {
			for (int i = 0; parameters[i].name; ++i) {
				const char *param = parameters[i].name;
				if (strncmp(param, p, strlen(param)) == 0) {
					const char *q = p + strlen(param);
					if (*q != ' ') {
						continue;
					}
					while (q < next && *q == ' ') {
						++q;
					}
					if (parameters[i].convert) {
						*parameters[i].p = parameters[i].convert(q);
					} else {
						*parameters[i].p = strtol(q, 0, 10);
					}
					break;
				}
			}
		}
		if (!next) {
			break;
		}
		p = next + 1;
	}

	free(iniData);
}

void Resource::loadCustomGUS() {
	int dataSize;
	File *fp = fileOpen("CUSTOM.GUS", &dataSize, kFileType_DRIVERS, false);
	if (!fp) {
		return;
	}

	char *gusData = ALLOC<char>(dataSize + 1);
	fileRead(fp, gusData, dataSize);
	gusData[dataSize] = '\0';

	fileClose(fp);

	static const int kOffsetBankPatches = 0;
	static const int kOffsetDrumPatches = 128;

	int offset = 0;
	for (char *p = gusData; *p; ) {
		char *next = strchr(p, '\n');
		if (p[0] == '#') {
			// ignore
		} else if (strncmp(p, "[Melodic Patches]", 17) == 0) {
			offset = kOffsetBankPatches;
		} else if (strncmp(p, "[Drum Patches]", 14) == 0) {
			offset = kOffsetDrumPatches;
		} else {
			char *q = strchr(p, '=');
			if (q && q < next) {
				const int index = atoi(p);
				char *nl = strpbrk(q + 1, "\r\n");
				if (nl) {
					*nl = 0;
					assert(offset + index <= kGusPatchesTableSize);
					strcpy(_gusPatches[offset + index], q + 1);
					debug(kDebug_RESOURCE, "GUS patch %d '%s.pat'", offset + index, _gusPatches[offset + index]);
				}
			}
		}
		if (!next) {
			break;
		}
		p = next + 1;
	}

	free(gusData);
}

static const struct {
	const char *ext;
	void (Resource::*LoadData)(File *fp, int dataSize);
} _resLoadDataTablePsx[] = {
	{ "CMD", &Resource::loadCMD },
	{ "ENV", &Resource::loadENV },
	{ "MSG", &Resource::loadMSG }
};

static const struct {
	const char *ext;
	int type;
} _resTreeTablePsx[] = {
	{ "PAL", kResType_PAL },
	{ "SPR", kResType_SPR },
	{ "F3D", kResType_F3D },
	{ "P3D", kResType_P3D }
};

void Resource::loadLevelDataPsx(int level, int resType) {
	switch (resType) {
	case kResTypePsx_DTT: {
			char name[16];
			snprintf(name, sizeof(name), "level%d%c.lev", level + 1, _languagesPsx[fileLanguage()]);
			File *fp = fileOpenPsx(name, kFileType_PSX_LEVELDATA, level + 1);
			if (fp) {
				const uint32_t dataSize = fileSize(fp);
				loadObjectText(fp, dataSize, level + 1);
				fileClose(fp);
			}
		}
		break;
	case kResTypePsx_LEV: {
			char name[16];
			snprintf(name, sizeof(name), "level%d.lev", level + 1);
			_fileLev = fileOpenPsx(name, kFileType_PSX_LEVELDATA, level + 1);
			if (_fileLev) {
				readDataOffsetsTable(_fileLev, kResOffsetType_LEV, _levOffsetsTable);
				if (kLoadPsxData) {
					for (uint32_t i = 0; i < ARRAYSIZE(_resLoadDataTablePsx); ++i) {
						const uint32_t dataSize = seekDataPsx(_resLoadDataTablePsx[i].ext, _fileLev, kResOffsetType_LEV);
						(this->*_resLoadDataTablePsx[i].LoadData)(_fileLev, dataSize);
					}
					for (uint32_t i = 0; i < ARRAYSIZE(_resTreeTablePsx); ++i) {
						const uint32_t dataSize = seekDataPsx(_resTreeTablePsx[i].ext, _fileLev, kResOffsetType_LEV);
						loadTreePsx(_fileLev, dataSize, _resTreeTablePsx[i].type);
					}
				}
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
			snprintf(name, sizeof(name), "level%s.vrm", _levelsPsx[level]);
			File *fp = fileOpenPsx(name, kFileType_PSX_LEVELDATA, level + 1);
			if (fp) {
				loadVRM(fp);
				fileClose(fp);
			}
		}
		break;
	}
}

void Resource::unloadLevelDataPsx(int resType) {
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

void Resource::readDataOffsetsTable(File *fp, int offsetType, ResPsxOffset *offsetsTable) {
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

uint32_t Resource::seekDataPsx(const char *ext, File *fp, int offsetType, uint32_t offset) {
	switch (offsetType) {
	case kResOffsetType_LEV:
		for (int i = 0; i < kResPsxLevOffsetsTableSize; ++i) {
			if (strcmp(_levOffsetsTable[i].ext, ext) == 0) {
				fileSetPos(fp, _levOffsetsTable[i].offset + offset, kFilePosition_SET);
				return _levOffsetsTable[i].size;
			}
		}
		break;
	case kResOffsetType_SON:
		for (int i = 0; i < kResPsxSonOffsetsTableSize; ++i) {
			if (strcmp(_sonOffsetsTable[i].ext, ext) == 0) {
				fileSetPos(fp, _sonOffsetsTable[i].offset + offset, kFilePosition_SET);
				return _sonOffsetsTable[i].size;
			}
		}
		break;
	}
	return 0;
}

void Resource::loadVAB(File *fp) {
	_vagOffsetsTableSize = 0;
	memset(_vagOffsetsTable, 0, sizeof(_vagOffsetsTable));

	const uint32_t vhSize = seekDataPsx("VH", fp, kResOffsetType_SON);
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

void Resource::loadVRM(File *fp) {
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

uint32_t Resource::seekVag(int num) {
	if (num >= 0 && num < _vagOffsetsTableSize) {
		const uint32_t vbSize = seekDataPsx("VB", _fileSon, kResOffsetType_SON, _vagOffsetsTable[num].offset);
		if (vbSize != 0) {
			return _vagOffsetsTable[num].size;
		}
		warning("'VB' data resource not found");
	}
	return 0;
}

void Resource::loadTreePsx(File *fp, int dataSize, int type) {

	const bool hasKeys = (type == kResType_ANI || type == kResType_STM);

	uint8_t header[0x20];
	fileRead(fp, header, sizeof(header));

	const int count = READ_LE_UINT32(header + 4);
	const int sizeOfOffset = READ_LE_UINT32(header + 8);

	uint32_t offset = sizeof(header) + count * sizeOfOffset;
	if (hasKeys) {
		offset += 2 * sizeof(uint16_t) * count;
	}

	_treesTable[type] = ALLOC<ResTreeNode>(count);
	_treesTableCount[type] = count;
	for (uint32_t j = 0; j < count; ++j) {
		ResTreeNode *node = &_treesTable[type][j];
		memset(node, 0, sizeof(ResTreeNode));
		node->dataOffset = offset;
		const uint32_t size = (sizeOfOffset == sizeof(uint32_t)) ? fileReadUint32LE(fp) : fileReadUint16LE(fp);
		node->dataSize = size;
		offset += size;
	}
	if (hasKeys) {
		for (uint32_t j = 0; j < count; ++j) {
			ResTreeNode *node = &_treesTable[type][j];
			node->childKey = fileReadUint16LE(fp);
			node->nextKey = fileReadUint16LE(fp);
		}
	}
}
