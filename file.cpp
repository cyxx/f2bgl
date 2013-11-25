/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include "file.h"

bool g_isDemo = false;
static int _fileLanguage;
static int _fileVoice;
static const char *_fileDataPath;
static bool _exitOnError = true;

static void fileMakeFilePath(const char *fileName, int fileType, int fileLang, char *filePath) {
	#ifdef __AROS__
	static const char *dataDirsTable[] = { "DATA", "DATA/SOUND", "DATA/TEXT", "DATA/VOICE" ,"DATA/DRIVERS"};
	#else
	static const char *dataDirsTable[] = { "DATA", "DATA/SOUND", "TEXT", "VOICE" };
	#endif
	static const char *langDirsTable[] = { "US", "FR", "GR", "SP", "IT" };

	if (fileType == kFileType_RUNTIME) 
	{
		sprintf(filePath, "%s/", _fileDataPath);
	} 
	else 
	{
		assert(fileType >= 0 && fileType < ARRAYSIZE(dataDirsTable));
	#ifdef __AROS__
		sprintf(filePath, "%s/",dataDirsTable[fileType]);
	#else
		sprintf(filePath, "%s/%s/", _fileDataPath, dataDirsTable[fileType]);
	#endif
	}
	
	switch (fileLang) 
	{
	case kFileLanguage_SP:
	case kFileLanguage_IT:
		if (fileType == kFileType_TEXT) {
			assert(fileLang >= 0 && fileLang < ARRAYSIZE(langDirsTable));
			strcat(filePath, langDirsTable[fileLang]);
			strcat(filePath, "/");
		}
		fileLang = _fileVoice;
		break;
	}
	if (fileType == kFileType_TEXT || fileType == kFileType_VOICE) {
		assert(fileLang >= 0 && fileLang < ARRAYSIZE(langDirsTable));
		strcat(filePath, langDirsTable[fileLang]);
		strcat(filePath, "/");
	}
	strcat(filePath, fileName);
}

static FILE *fileOpenIntern(const char *fileName, int fileType) {
	char filePath[512];

	fileMakeFilePath(fileName, fileType, _fileLanguage, filePath);
	char *p = strrchr(filePath, '/');
	if (p) {
		++p;
	} else {
		p = filePath;
	}
	stringToUpperCase(p);
	FILE *fp = fopen(filePath, "rb");
	if (!fp) {
		stringToLowerCase(p);
		fp = fopen(filePath, "rb");
	}
	if (!fp && fileType == kFileType_TEXT) {
		fp = fileOpenIntern(fileName, kFileType_DATA);
	}
	return fp;
}

bool fileExists(const char *fileName, int fileType) {
	FILE *fp = fileOpenIntern(fileName, fileType);
	if (fp) {
		fclose(fp);
	}
	return fp != 0;
}

bool fileInit(int language, int voice, const char *dataPath) {
	_fileLanguage = language;
	_fileVoice = voice;
	_fileDataPath = dataPath;
	bool ret = fileExists("player.ini", kFileType_DATA);
	if (ret) {
		g_isDemo = fileExists("ddtitle.cin", kFileType_DATA);
	}
	debug(kDebug_FILE, "fileInit() dataPath '%s' isDemo %d", _fileDataPath, g_isDemo);
	return ret;
}

FILE *fileOpen(const char *fileName, int *fileSize, int fileType, bool errorIfNotFound) {
	if (g_isDemo) {
		if (fileType == kFileType_VOICE) {
			return 0;
		} else if (fileType == kFileType_TEXT) {
			fileType = kFileType_DATA;
		}
	}
	FILE *fp = fileOpenIntern(fileName, fileType);
	if (!fp) {
		if (errorIfNotFound) {
			error("Unable to open '%s'", fileName);
		} else {
			warning("Unable to open '%s'", fileName);
		}
		return 0;
	}
	fseek(fp, 0, SEEK_END);
	if (fileSize) {
		*fileSize = ftell(fp);
	}
	fseek(fp, 0, SEEK_SET);
	return fp;
}

void fileClose(FILE *fp) {
	fclose(fp);
}

void fileRead(FILE *fp, void *buf, int size) {
	int count = fread(buf, size, 1, fp);
	if (count != 1) {
		if (_exitOnError && ferror(fp)) {
			error("I/O error on reading %d bytes", size);
		}
	}
}

uint8_t fileReadByte(FILE *fp) {
	uint8_t b;
	fileRead(fp, &b, 1);
	return b;
}

uint16_t fileReadUint16LE(FILE *fp) {
	uint8_t buf[2];
	fileRead(fp, buf, 2);
	return READ_LE_UINT16(buf);
}

uint32_t fileReadUint32LE(FILE *fp) {
	uint8_t buf[4];
	fileRead(fp, buf, 4);
	return READ_LE_UINT32(buf);
}

uint32_t fileGetPos(FILE *fp) {
	return ftell(fp);
}

void fileSetPos(FILE *fp, uint32_t pos, int origin) {
	static const int originTable[] = { SEEK_CUR, SEEK_SET };
	if (origin >= ARRAYSIZE(originTable)) {
		error("Invalid file seek origin", origin);
	} else {
		int r = fseek(fp, pos, originTable[origin]);
		if (r != 0 && _exitOnError) {
			error("I/O error on seeking to offset %d", pos);
		}
	}
}

int fileEof(FILE *fp) {
	return feof(fp);
}
