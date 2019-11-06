/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#ifndef RESOURCE_H__
#define RESOURCE_H__

#include "file.h"
#include "util.h"

enum {
	kResType_SPR,
	kResType_PAL,
	kResType_MAP,
	kResType_ANI,
	kResType_STM,
	kResType_OBJ,
	kResType_SND,
	kResType_F3D,
	kResType_P3D,
	kResTypeCount
};

enum {
	kKeyPathsTableSize = 400,
	kEnvAniDataSize = 38,
	kIniMaxLineLength = 256,
	kLevelDescriptionsCount = 13,
	kSoundKeyPathsTableSize = 10,
	kMusicKeyPathsTableSize = 52,
	kKeyPathNameLength = 96,
	kGusPatchesTableSize = 256,
	kLevelMusicTableSize = 14
};

enum {
	kResTypePsx_DTT,
	kResTypePsx_LEV,
	kResTypePsx_SON,
	kResTypePsx_VRM,
};

enum {
	kResOffsetType_LEV,
	kResOffsetType_SON,
};

enum {
	kVrmLoadingScreenWidth = 320,
	kVrmLoadingScreenHeight = 240,
	kResPsxLevOffsetsTableSize = 11, // .STM, .ANI, .F3D, .P3D, .SPR, .PAL, .MSG, .CMD, .ENV, .KEY, .SNK
	kResPsxSonOffsetsTableSize = 3, // .SPU, .VH, .VB
	kVagOffsetsTableSize = 256,
};

struct ResTreeNode {
	int16_t childKey;
	int16_t nextKey;
	uint8_t *data;
	uint32_t dataSize;
	uint32_t dataOffset;
};

struct ResObjectIndex {
	char objectName[64];
	int16_t objectKey;
	uint32_t dataOffs;
};

struct ResKeyPath {
	char pathName[kKeyPathNameLength];
	int16_t key;
};

struct ResLevelDescription {
	char name[16];
	char palKey[kKeyPathNameLength];
	char mapKey[kKeyPathNameLength];
	bool inventory;
	bool map;
	int16_t musicKeys[kLevelMusicTableSize];
};

struct ResMessageDescription {
	const uint8_t *data;
	int16_t frameSync;
	int16_t duration;
	int16_t xPos, yPos;
	int font;
};

struct ResDemoInput {
	int ticks;
	uint16_t key;
	bool pressed;
};

enum {
	kSkillEasy = 0,
	kSkillNormal,
	kSkillHard
};

struct ResUserConfig {
	int greyScale;
	int lightCoef;
	int subtitles;
	int iconLrMoveX;
	int iconLrMoveY;
	int iconLrStepX;
	int iconLrStepY;
	int iconLrToolsX;
	int iconLrToolsY;
	int iconLrCabX;
	int iconLrCabY;
	int iconLrInvX;
	int iconLrInvY;
	int iconLrScanX;
	int iconLrScanY;
	int skillLevel;
	int soundVolume;
	int musicVolume;
	int voiceVolume;
	int soundOn;
	int musicOn;
	int voiceOn;
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

struct Resource {
	ResTreeNode *_treesTable[kResTypeCount];
	uint16_t _treesTableCount[kResTypeCount];
	uint16_t _msgOffsetsTableCount;
	uint16_t *_msgOffsetsTable;
	uint8_t *_msgData;
	uint32_t _cmdOffsetsTableCount;
	uint32_t *_cmdOffsetsTable;
	uint8_t *_cmdData;
	uint32_t _objectIndexesTableCount;
	ResObjectIndex *_objectIndexesTable;
	uint32_t _objectTextDataSize;
	uint8_t *_objectTextData;
	uint16_t _keyPathsTableCount;
	ResKeyPath _keyPathsTable[kKeyPathsTableSize];
	uint32_t _envAniDataCount;
	uint8_t *_envAniData;
	ResLevelDescription _levelDescriptionsTable[kLevelDescriptionsCount];
	char _soundKeyPathsTable[kSoundKeyPathsTableSize][kKeyPathNameLength];
	int16_t _sndKeysTable[kSoundKeyPathsTableSize];
	char _musicKeyPathsTable[kMusicKeyPathsTableSize][kKeyPathNameLength];
	int16_t _musicKeysTable[kMusicKeyPathsTableSize];
	int _demoInputDataSize;
	ResDemoInput *_demoInputData;
	ResUserConfig _userConfig;
	char _gusPatches[kGusPatchesTableSize][16]; // bank0, drum0
	int16_t _conradVoiceCmdNum;
	int16_t _lastObjectKey;
	uint32_t _textIndexesTableCount;
	uint32_t *_textIndexesTable; // offsets to .dtt indexed by (objKey - 1)
	uint8_t *_vrmLoadingBitmap;
	uint32_t _vagOffsetsTableSize;
	VagOffset _vagOffsetsTable[kVagOffsetsTableSize];
	ResPsxOffset _sonOffsetsTable[kResPsxSonOffsetsTableSize];
	File *_fileSon;
	ResPsxOffset _levOffsetsTable[kResPsxLevOffsetsTableSize];
	File *_fileLev;

	Resource();
	~Resource();

	void loadLevelData(int levelNum);
	void unload(int type, int16_t key);
	int16_t getPrevious(int type, int16_t key);
	int16_t getNext(int type, int16_t key);
	int16_t getChild(int type, int16_t key);
	int16_t getRoot(int type);
	uint8_t *getEnvAni(int16_t key, int num);
	uint8_t *getData(int type, int16_t key, const char *name);
	void setObjectKey(const char *objectName, int16_t objectKey);
	int getOffsetForObjectKey(int16_t objectKey);
	int16_t getKeyFromPath(const char *path);
	const uint8_t *getCmdData(int num);
	const uint8_t *getMsgData(int num);
	bool getMessageDescription(ResMessageDescription *m, uint32_t value, uint32_t offset);
	void patchCmdData(int levelNum);

	void loadINI(File *fp, int dataSize);
	void loadCMD(File *fp, int dataSize);
	void loadMSG(File *fp, int dataSize);
	void loadENV(File *fp, int dataSize);
	void loadKeyPaths(File *fp, int dataSize);
	void loadObjectIndexes(File *fp, int dataSize);
	void loadObjectText(File *fp, int dataSize, int levelNum);
	void loadINM(int levelNum);
	void loadTrigo();
	void loadDEM(File *fp, int dataSize);
	void loadDelphineINI();
	void loadCustomGUS();

	void loadLevelDataPsx(int level, int resType);
	void unloadLevelDataPsx(int resType);
	void readDataOffsetsTable(File *fp, int offsetType, ResPsxOffset *offsetsTable);
	uint32_t seekDataPsx(const char *ext, File *fp, int offsetType, uint32_t offset = 0);
	void loadVAB(File *fp);
	void loadVRM(File *fp);
	uint32_t seekVag(int num);
	void loadTreePsx(File *fp, int dataSize, int type);
};

#endif // RESOURCE_H__
