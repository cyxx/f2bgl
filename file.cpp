/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <zlib.h>
#include "file.h"

struct File {
	virtual ~File() {
	}
	virtual bool open(const char *path, const char *mode) = 0;
	virtual void close() = 0;
	virtual int eof() = 0;
	virtual int err() = 0;
	virtual int tell() = 0;
	virtual int seek(int pos, int whence) = 0;
	virtual int read(void *, int) = 0;
	virtual int write(const void *, int) = 0;
};

struct StdioFile: File {
	FILE *_fp;

	StdioFile()
		: _fp(0) {
	}
	virtual bool open(const char *path, const char *mode) {
		_fp = fopen(path, mode);
		return _fp != 0;
	}
	virtual void close() {
		if (_fp) {
			fclose(_fp);
			_fp = 0;
		}
	}
	virtual int eof() {
		if (_fp) {
			return feof(_fp);
		}
		return 0;
	}
	virtual int err() {
		if (_fp) {
			return ferror(_fp);
		}
		return 0;
	}
	virtual int tell() {
		if (_fp) {
			return ftell(_fp);
		}
		return 0;
	}
	virtual int seek(int pos, int whence) {
		if (_fp) {
			return fseek(_fp, pos, whence);
		}
		return 0;
	}
	virtual int read(void *p, int size) {
		if (_fp) {
			return fread(p, 1, size, _fp);
		}
		return 0;
	}
	virtual int write(const void *p, int size) {
		if (_fp) {
			return fwrite(p, 1, size, _fp);
		}
		return 0;
	}
	static File *openIfExists(const char *path) {
		File *f = 0;
		struct stat st;
		if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) {
			f = new StdioFile;
			if (!f->open(path, "rb")) {
				delete f;
				f = 0;
			}
		}
		return f;
	}
};

struct GzipFile: File {
	gzFile _fp;

	virtual bool open(const char *path, const char *mode) {
		_fp = gzopen(path, mode);
		return _fp != 0;
	}
	virtual void close() {
		if (_fp) {
			gzclose(_fp);
			_fp = 0;
		}
	}
	virtual int eof() {
		if (_fp) {
			return gzeof(_fp);
		}
		return 0;
	}
	virtual int err() {
		if (_fp) {
			int errnum;
			gzerror(_fp, &errnum);
			return errnum != Z_OK;
		}
		return 0;
	}
	virtual int tell() {
		if (_fp) {
			return gztell(_fp);
		}
		return 0;
	}
	virtual int seek(int pos, int whence) {
		if (_fp) {
			const int ret = gzseek(_fp, pos, whence);
			if (ret == -1) {
				return ret;
			}
		}
		return 0;
	}
	virtual int read(void *p , int size) {
		if (_fp) {
			return gzread(_fp, p, size);
		}
		return 0;
	}
	virtual int write(const void *p, int size) {
		if (_fp) {
			return gzwrite(_fp, p, size);
		}
		return 0;
	}
};

bool g_isDemo = false;
static int _fileLanguage;
static int _fileVoice;
static const char *_fileDataPath;
static const char *_fileSavePath = ".";
static bool _exitOnError = true;

static void fileMakeFilePath(const char *fileName, int fileType, int fileLang, char *filePath) {
	static const char *dataDirsTable[] = { "DATA", "DATA/SOUND", "TEXT", "VOICE" };
	static const char *langDirsTable[] = { "US", "FR", "GR", "SP", "IT" };

	if (fileType == kFileType_RUNTIME) {
		sprintf(filePath, "%s/", _fileDataPath);
	} else {
		assert(fileType >= 0 && fileType < ARRAYSIZE(dataDirsTable));
		sprintf(filePath, "%s/%s/", _fileDataPath, dataDirsTable[fileType]);
	}
	switch (fileLang) {
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

static File *fileOpenIntern(const char *fileName, int fileType) {
	char filePath[MAXPATHLEN];
	if (fileType == kFileType_SAVE || fileType == kFileType_LOAD) {
		snprintf(filePath, sizeof(filePath), "%s/%s", _fileSavePath, fileName);
		File *fp = new GzipFile;
		if (!fp->open(filePath, (fileType == kFileType_SAVE) ? "wb" : "rb")) {
			delete fp;
			fp = 0;
		}
		return fp;
	}
	fileMakeFilePath(fileName, fileType, _fileLanguage, filePath);
	char *p = strrchr(filePath, '/');
	if (p) {
		++p;
	} else {
		p = filePath;
	}
	stringToUpperCase(p);
	File *fp = StdioFile::openIfExists(filePath);
	if (!fp) {
		stringToLowerCase(p);
		fp = StdioFile::openIfExists(filePath);
	}
	if (!fp && fileType == kFileType_TEXT) {
		fp = fileOpenIntern(fileName, kFileType_DATA);
	}
	return fp;
}

bool fileExists(const char *fileName, int fileType) {
	bool exists = false;
	File *fp = fileOpenIntern(fileName, fileType);
	if (fp) {
		fp->close();
		delete fp;
		exists = true;
	}
	return exists;
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

File *fileOpen(const char *fileName, int *fileSize, int fileType, bool errorIfNotFound) {
	if (g_isDemo) {
		if (fileType == kFileType_VOICE) {
			return 0;
		} else if (fileType == kFileType_TEXT) {
			fileType = kFileType_DATA;
		}
	}
	File *fp = fileOpenIntern(fileName, fileType);
	if (!fp) {
		if (errorIfNotFound) {
			error("Unable to open '%s'", fileName);
		} else {
			warning("Unable to open '%s'", fileName);
		}
		return 0;
	}
	fp->seek(0, SEEK_END);
	if (fileSize) {
		*fileSize = fp->tell();
	}
	fp->seek(0, SEEK_SET);
	return fp;
}

void fileClose(File *fp) {
	if (fp) {
		fp->close();
		delete fp;
	}
}

void fileRead(File *fp, void *buf, int size) {
	int count = fp->read(buf, size);
	if (count != size) {
		if (_exitOnError && fp->err()) {
			error("I/O error on reading %d bytes", size);
		}
	}
}

uint8_t fileReadByte(File *fp) {
	uint8_t b;
	fileRead(fp, &b, 1);
	return b;
}

uint16_t fileReadUint16LE(File *fp) {
	uint8_t buf[2];
	fileRead(fp, buf, 2);
	return READ_LE_UINT16(buf);
}

uint32_t fileReadUint32LE(File *fp) {
	uint8_t buf[4];
	fileRead(fp, buf, 4);
	return READ_LE_UINT32(buf);
}

uint32_t fileGetPos(File *fp) {
	return fp->tell();
}

void fileSetPos(File *fp, uint32_t pos, int origin) {
	static const int originTable[] = { SEEK_CUR, SEEK_SET };
	if (origin >= ARRAYSIZE(originTable)) {
		error("Invalid file seek origin", origin);
	} else {
		int r = fp->seek(pos, originTable[origin]);
		if (r != 0 && _exitOnError) {
			error("I/O error on seeking to offset %d", pos);
		}
	}
}

int fileEof(File *fp) {
	return fp->eof();
}

void fileWrite(File *fp, const void *buf, int size) {
	const int count = fp->write(buf, size);
	if (count != size) {
		if (_exitOnError && fp->err()) {
			error("I/O error writing %d bytes", size);
		}
	}
}

void fileWriteByte(File *fp, uint8_t value) {
	fileWrite(fp, &value, 1);
}

void fileWriteUint16LE(File *fp, uint16_t value) {
	uint8_t buf[2];
	for (int i = 0; i < 2; ++i) {
		buf[i] = value & 255;
		value >>= 8;
	}
	fileWrite(fp, buf, sizeof(buf));
}

void fileWriteUint32LE(File *fp, uint32_t value) {
	uint8_t buf[4];
	for (int i = 0; i < 4; ++i) {
		buf[i] = value & 255;
		value >>= 8;
	}
	fileWrite(fp, buf, sizeof(buf));
}
