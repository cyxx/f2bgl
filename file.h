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
	kFileType_SCREENSHOT,
	kFileType_CONFIG,
};

enum FileLanguage {
	kFileLanguage_EN = 0,
	kFileLanguage_FR,
	kFileLanguage_GR,
	kFileLanguage_SP,
	kFileLanguage_IT,
};

enum FilePosition {
	kFilePosition_CUR = 0,
	kFilePosition_SET
};

struct File;

extern bool g_isDemo;
extern const char *g_fileDataPath;
extern const char *g_fileSavePath;

bool fileInit(int language, int voice, const char *dataPath, const char *savePath);
int fileLanguage();
bool fileExists(const char *fileName, int fileType);
File *fileOpen(const char *fileName, int *fileSize, int fileType, bool errorIfNotFound = true);
void fileClose(File *fp);
void fileRead(File *fp, void *buf, int size);
uint8_t fileReadByte(File *fp);
uint16_t fileReadUint16LE(File *fp);
uint32_t fileReadUint32LE(File *fp);
uint32_t fileGetPos(File *fp);
void fileSetPos(File *fp, uint32_t pos, int origin);
int fileEof(File *fp);
void fileWrite(File *fp, const void *buf, int size);
void fileWriteByte(File *fp, uint8_t value);
void fileWriteUint16LE(File *fp, uint16_t value);
void fileWriteUint32LE(File *fp, uint32_t value);
void fileWriteLine(File *fp, const char *s, ...);

#endif // __File_H__
