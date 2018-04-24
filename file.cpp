/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include <dirent.h>
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

struct FileSystem {
	char **_fileList;
	int _fileCount;
	int _filePathSkipLen;

	FileSystem() :
		_fileList(0), _fileCount(0), _filePathSkipLen(0) {
	}
	~FileSystem() {
		for (int i = 0; i < _fileCount; ++i) {
			free(_fileList[i]);
		}
		free(_fileList);
	}
	void buildFileListFromDirectory(const char *dir) {
		DIR *d = opendir(dir);
		if (d) {
			dirent *de;
			while ((de = readdir(d)) != NULL) {
				if (de->d_name[0] == '.') {
					continue;
				}
				char filePath[MAXPATHLEN];
				snprintf(filePath, sizeof(filePath), "%s/%s", dir, de->d_name);
				struct stat st;
				if (stat(filePath, &st) == 0) {
					if (S_ISDIR(st.st_mode)) {
						buildFileListFromDirectory(filePath);
					} else {
						_fileList = (char **)realloc(_fileList, (_fileCount + 1) * sizeof(char *));
						if (_fileList) {
							_fileList[_fileCount] = strdup(filePath + _filePathSkipLen);
							++_fileCount;
						}
					}
				}
			}
			closedir(d);
		}
	}
	void setDataDirectory(const char *dir) {
		_filePathSkipLen = strlen(dir) + 1;
		buildFileListFromDirectory(dir);
		debug(kDebug_FILE, "FileSystem::setDataDirectory('%s') %d files", dir, _fileCount);
	}
	const char *findPath(const char *file) const {
		for (int i = 0; i < _fileCount; ++i) {
			if (strcasecmp(_fileList[i], file) == 0) {
				debug(kDebug_FILE, "FileSystem::findPath() '%s'", file);
				return _fileList[i];
			}
		}
		return 0;
	}
};

bool g_isDemo = false;
bool g_hasPsx = false;
uint32_t g_level1ObjCrc;
static int _fileLanguage;
static int _fileVoice;
const char *g_fileDataPath;
const char *g_fileSavePath;
static bool _exitOnError = true;
static FileSystem *_fileSystem;
static const char *_psxDataPath;

static void fileMakeFilePath(const char *fileName, int fileType, int fileLang, char *filePath) {
	static const char *dataDirsTable[] = { "DATA", "DATA/SOUND", "TEXT", "VOICE", "DATA/DRIVERS", "INSTDATA" };
	static const char *langDirsTable[] = { "US", "FR", "GR", "SP", "IT" };

	if (fileType != kFileType_RUNTIME) {
		assert(fileType >= 0 && fileType < ARRAYSIZE(dataDirsTable));
		strcat(filePath, dataDirsTable[fileType]);
		strcat(filePath, "/");
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

static File *fileOpenIntern(const char *fileName, int fileType, const char *prefixPath = "") {
	char filePath[MAXPATHLEN];
	strcpy(filePath, prefixPath);
	fileMakeFilePath(fileName, fileType, _fileLanguage, filePath);
	debug(kDebug_FILE, "fileOpenIntern() path '%s'", filePath);
	const char *path = _fileSystem->findPath(filePath);
	if (path) {
		snprintf(filePath, sizeof(filePath), "%s/%s", g_fileDataPath, path);
		File *fp = new StdioFile;
		if (!fp->open(filePath, "rb")) {
			delete fp;
			fp = 0;
		}
		return fp;
	}
	// on the original CD-ROM, the TEXT/ and VOICE/ directories are under DATA/.
	// the original installer would copy them at the same level as DATA/.
	if ((fileType == kFileType_TEXT || fileType == kFileType_VOICE) && !prefixPath[0]) {
		return fileOpenIntern(fileName, fileType, "DATA/");
	}
	return 0;
}

bool fileExists(const char *fileName, int fileType) {
	if (fileType == kFileType_SAVE || fileType == kFileType_LOAD || fileType == kFileType_SCREENSHOT_LOAD || fileType == kFileType_CONFIG) {
		char filePath[MAXPATHLEN];
		snprintf(filePath, sizeof(filePath), "%s/%s", g_fileSavePath, fileName);
		struct stat st;
		return stat(filePath, &st) == 0 && S_ISREG(st.st_mode);
	}
	if (g_isDemo) {
		if (fileType == kFileType_VOICE) {
			return false;
		} else if (fileType == kFileType_TEXT) {
			fileType = kFileType_DATA;
		}
	}
	bool exists = false;
	File *fp = fileOpenIntern(fileName, fileType);
	if (fp) {
		fp->close();
		delete fp;
		exists = true;
	}
	return exists;
}

bool fileInit(int language, int voice, const char *dataPath, const char *savePath) {
	_fileLanguage = language;
	_fileVoice = voice;
	g_fileDataPath = dataPath;
	g_fileSavePath = savePath;
	_fileSystem = new FileSystem;
	_fileSystem->setDataDirectory(dataPath);
	bool ret = fileExists("player.ini", kFileType_DATA);
	if (ret) {
		g_isDemo = fileExists("ddtitle.cin", kFileType_DATA);
	}
	File *fp = fileOpen("level1.obj", 0, kFileType_DATA, true);
	if (fp) {
		g_level1ObjCrc = fileCrc32(fp);
		fileClose(fp);
	}
	debug(kDebug_FILE, "fileInit() dataPath '%s' isDemo %d level1Crc 0x%08x", g_fileDataPath, g_isDemo, g_level1ObjCrc);
	return ret;
}

int fileLanguage() {
	return _fileLanguage;
}

int fileVoice() {
	return _fileVoice;
}

File *fileOpen(const char *fileName, int *fileSize, int fileType, bool errorIfNotFound) {
	if (fileType == kFileType_SAVE || fileType == kFileType_LOAD || fileType == kFileType_SCREENSHOT_SAVE || fileType == kFileType_SCREENSHOT_LOAD || fileType == kFileType_CONFIG) {
		char filePath[MAXPATHLEN];
		snprintf(filePath, sizeof(filePath), "%s/%s", g_fileSavePath, fileName);
		File *fp = 0;
		switch (fileType) {
		case kFileType_LOAD:
		case kFileType_SAVE:
			fp = new GzipFile;
			break;
		case kFileType_SCREENSHOT_LOAD:
		case kFileType_SCREENSHOT_SAVE:
		case kFileType_CONFIG:
			fp = new StdioFile;
			break;
		default:
			break;
		}
		const char *mode = 0;
		switch (fileType) {
		case kFileType_LOAD:
		case kFileType_SCREENSHOT_LOAD:
			mode = "rb";
			break;
		default:
			mode = "wb";
			break;
		}
		if (fp && !fp->open(filePath, mode)) {
			delete fp;
			fp = 0;
		}
		return fp;
	}
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

int fileRead(File *fp, void *buf, int size) {
	const int count = fp->read(buf, size);
	if (count != size) {
		if (_exitOnError && fp->err()) {
			error("I/O error on reading %d bytes", size);
		}
	}
	return count;
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

uint32_t fileCrc32(File *fp) {
	uint32_t crc = 0;
	const uint32_t pos = fp->seek(0, SEEK_SET);
	int count;
	uint8_t buf[4096];
	while ((count = fp->read(buf, sizeof(buf))) > 0) {
		crc = crc32(crc, buf, count);
	}
	fp->seek(pos, SEEK_SET);
	return crc;
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

void fileWriteLine(File *fp, const char *s, ...) {
	char buf[256];
	va_list va;
	va_start(va, s);
	vsprintf(buf, s, va);
	va_end(va);
	fileWrite(fp, buf, strlen(buf));
}

int fileSize(File *fp) {
	const int pos = fp->seek(0, SEEK_END);
	const int size = fp->tell();
	fp->seek(pos, SEEK_SET);
	return size;
}

bool fileInitPsx(const char *dataPath) {
	_psxDataPath = dataPath;
	g_hasPsx = true;
	return g_hasPsx;
}

static const char *_psxLanguages[] = { "us", "fr", "gr", "sp", "it", "jp" };

File *fileOpenPsx(const char *filename, int fileType, int levelNum) {
	File *fp = new StdioFile;
	char path[MAXPATHLEN];
	switch (fileType) {
	case kFileType_PSX_IMG:
		snprintf(path, sizeof(path), "%s/img/%s", _psxDataPath, filename);
		break;
	case kFileType_PSX_LEVELDATA:
		snprintf(path, sizeof(path), "%s/data%d/%s", _psxDataPath, levelNum, filename);
		if (fp->open(path, "rb")) {
			return fp;
		}
		break;
	case kFileType_PSX_VIDEO:
		if (strlen(filename) != 8) {
			warning("Invalid PSX video filename '%s'", filename);
			break;
		}
		// voiced videos
		snprintf(path, sizeof(path), "%s/videos/%s/%s", _psxDataPath, _psxLanguages[_fileLanguage], filename);
		if (fp->open(path, "rb")) {
			return fp;
		}
		if (_fileLanguage == kFileLanguage_GR) {
			// german specific videos
			snprintf(path, sizeof(path), "%s/video2/gr%s", _psxDataPath, &filename[2]);
			if (fp->open(path, "rb")) {
				return fp;
			}
		}
		snprintf(path, sizeof(path), "%s/video2/%s", _psxDataPath, filename);
		if (fp->open(path, "rb")) {
			return fp;
		}
		snprintf(path, sizeof(path), "%s/video/%s", _psxDataPath, filename);
		if (fp->open(path, "rb")) {
			return fp;
		}
		break;
	case kFileType_PSX_VOICE:
		snprintf(path, sizeof(path), "%s/xa_%s/%s", _psxDataPath, _psxLanguages[_fileLanguage], filename);
		break;
	default:
		break;
	}
	// fallback, files dumped without directory structure
	snprintf(path, sizeof(path), "%s/%s", _psxDataPath, filename);
	if (fp->open(path, "rb")) {
		return fp;
	}
	warning("Unable to open '%s' type %d", filename, fileType);
	delete fp;
	return 0;
}
