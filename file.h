/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#ifndef FILE_H__
#define FILE_H__

#include "util.h"

enum FileType {
	kFileType_DATA = 0,
	kFileType_SOUND,
	kFileType_TEXT,
	kFileType_VOICE,
	kFileType_DRIVERS,
	kFileType_INSTDATA,
	kFileType_RUNTIME,
	kFileType_LOAD,
	kFileType_SAVE,
	kFileType_SCREENSHOT_LOAD,
	kFileType_SCREENSHOT_SAVE,
	kFileType_CONFIG,
	kFileType_PSX_IMG,
	kFileType_PSX_LEVELDATA,
	kFileType_PSX_VIDEO,
	kFileType_PSX_VOICE,
};

enum FileLanguage {
	kFileLanguage_EN = 0,
	kFileLanguage_FR,
	kFileLanguage_GR,
	kFileLanguage_SP,
	kFileLanguage_IT,
	kFileLanguage_JP,
};

enum FilePosition {
	kFilePosition_CUR = 0,
	kFilePosition_SET
};

struct File;

extern bool g_isDemo;
extern bool g_hasPsx;
extern uint32_t g_level1ObjCrc; // crc for savegame states
extern const char *g_fileDataPath;
extern const char *g_fileSavePath;

bool fileInit(int language, int voice, const char *dataPath, const char *savePath);
int fileLanguage();
int fileVoice();
bool fileExists(const char *fileName, int fileType);
File *fileOpen(const char *fileName, int *fileSize, int fileType, bool errorIfNotFound = true);
void fileClose(File *fp);
int fileRead(File *fp, void *buf, int size);
uint8_t fileReadByte(File *fp);
uint16_t fileReadUint16LE(File *fp);
uint32_t fileReadUint32LE(File *fp);
uint32_t fileGetPos(File *fp);
void fileSetPos(File *fp, uint32_t pos, int origin);
int fileEof(File *fp);
uint32_t fileCrc32(File *fp);
void fileWrite(File *fp, const void *buf, int size);
void fileWriteByte(File *fp, uint8_t value);
void fileWriteUint16LE(File *fp, uint16_t value);
void fileWriteUint32LE(File *fp, uint32_t value);
void fileWriteLine(File *fp, const char *s, ...);
int fileSize(File *fp);
bool fileInitPsx(const char *dataPath);
File *fileOpenPsx(const char *filename, int fileType, int levelNum = -1);

#endif // FILE_H__
