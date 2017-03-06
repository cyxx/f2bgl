/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include <math.h>
#include "file.h"
#include "trigo.h"
#include "resource.h"

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
	_demoInputDataSize = 0;
	_demoInputData = 0;
	memset(&_userConfig, 0, sizeof(_userConfig));
	memset(_gusPatches, 0, sizeof(_gusPatches));
}

Resource::~Resource() {
	// TODO
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

static int rescompareIndexByObjectName(const void *p1, const void *p2) {
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
	qsort(_objectIndexesTable, _objectIndexesTableCount, sizeof(ResObjectIndex), rescompareIndexByObjectName);
}

void Resource::loadObjectText(File *fp, int dataSize) {
	free(_objectTextData);
	_objectTextData = ALLOC<uint8_t>(dataSize);
	_objectTextDataSize = dataSize;
	fileRead(fp, _objectTextData, dataSize);
}

static int rescompareKeyPaths(const void *p1, const void *p2) {
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
	qsort(_keyPathsTable, _keyPathsTableCount, sizeof(ResKeyPath), rescompareKeyPaths);
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
			g_sin[i] = (int)(cos(a) * 32767);
			g_cos[i] = (int)(sin(a) * 32767);
		}
		int count = 0;
		g_atan[0] = 0;
		for (int i = 1; i < 256; ++i) {
			g_atan[i] = (int)(atan(i / 256.) * (1024. / 2) / M_PI);
			if (g_atan[i - 1] != g_atan[i]) {
				++count;
			}
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
			uint8_t tmp[6];
			memcpy(tmp, p + 14, 6);
			memcpy(p + 14, tmp + 2, 2); // shootY
			p[16] = tmp[0]; // shootX
			p[17] = tmp[4]; // shootY
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

void Resource::loadLevelData(const char *levelName, int levelNum) {
	File *fp;
	int dataSize;
	char filename[32];

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
			if (size != 0) {
				node->dataSize = size;
				node->data = (uint8_t *)malloc(size);
				if (node->data) {
					const uint32_t pos = fileGetPos(fp);
					fileSetPos(fp, node->dataOffset, kFilePosition_SET);
					fileRead(fp, node->data, node->dataSize);
					if (g_isDemo && _resLoadDataTable[i].convert) {
						node->data = _resLoadDataTable[i].convert(node->data, &node->dataSize);
					}
					fileSetPos(fp, pos, kFilePosition_SET);
				}
			}
		}
		fileClose(fp);
	}

	for (uint32_t i = 0; i < ARRAYSIZE(_resLoadDataTable2); ++i) {
		snprintf(filename, sizeof(filename), "level%d.%s", levelNum, _resLoadDataTable2[i].ext);
		fp = fileOpen(filename, &dataSize, kFileType_DATA);
		(this->*_resLoadDataTable2[i].LoadData)(fp, dataSize);
		fileClose(fp);
	}

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
	loadObjectText(fp, dataSize);
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
	assert(type == kResType_ANI || (data && _treesTable[type][key].dataSize != 0));
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
}

int Resource::getOffsetForObjectKey(int16_t objectKey) {
	debug(kDebug_RESOURCE, "Resource::getOffsetForObjectKey() key %d", objectKey);
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
		uint32_t sz = READ_LE_UINT32(p); p += 4;
		if (val == value) {
			m->frameSync = READ_LE_UINT16(p); p += 2;
			m->duration = READ_LE_UINT16(p); p += 2;
			m->xPos = READ_LE_UINT16(p); p += 2;
			m->yPos = READ_LE_UINT16(p); p += 2;
			m->font = READ_LE_UINT32(p); p += 4;
			m->data = p;
			return true;
		}
		p += sz;
	}
	return false;
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
		{ "GREY_SCALE", &_userConfig.greyScale, convertParameterOnOff },
		{ "LIGHT_COEF", &_userConfig.lightCoef, 0 },
		{ "ICON_LR_INV_X", &_userConfig.iconLrInvX, 0 },
		{ "ICON_LR_INV_Y", &_userConfig.iconLrInvY, 0 },
		{ "PLAY_LEVEL", &_userConfig.skillLevel, convertParameterPlayLevel },
		{ 0, 0 }
	};

	for (char *p = iniData; *p; ) {
		char *next = strchr(p, '\n');
		if (p[0] != '#') {
			for (int i = 0; parameters[i].name; ++i) {
				const char *param = parameters[i].name;
				if (strncmp(param, p, strlen(param)) == 0) {
					const char *q = p + strlen(param);
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
