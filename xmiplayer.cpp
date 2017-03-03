
#include <sys/param.h>
#include <wildmidi_lib.h>
#include "file.h"
#include "xmiplayer.h"

enum {
	kCustomGusNone,
	kCustomGusMelodicPatches,
	kCustomGusDrumPatches
};

static char *readFile(const char *name, int fileType) {
	char *data = 0;
	int dataSize;
	File *fp = fileOpen(name, &dataSize, fileType, false);
	if (fp) {
		data = ALLOC<char>(dataSize + 1);
		fileRead(fp, data, dataSize);
		data[dataSize] = '\0';
		fileClose(fp);
	}
	return data;
}

static void fileWriteLine(File *fp, const char *s, ...) {
	char buf[256];
	va_list va;
	va_start(va, s);
	vsprintf(buf, s, va);
	va_end(va);
	fileWrite(fp, buf, strlen(buf));
}

// convert drivers/custom.gus to wildmidi configuration file
static void convertCustomGus(char *gusData, File *fp) {
	fileWriteLine(fp, "\ndir %s/DATA/DRIVERS\n\n", g_fileDataPath);

	int state = kCustomGusNone;
	int prevState = state;
	for (char *p = gusData; *p; ) {
		char *next = strchr(p, '\n');
		if (p[0] == '#') {
			// ignore
		} else if (strncmp(p, "[Melodic Patches]", 17) == 0) {
			state = kCustomGusMelodicPatches;
		} else if (strncmp(p, "[Drum Patches]", 14) == 0) {
			state = kCustomGusDrumPatches;
		} else {
			if (state != prevState) {
				switch (state) {
				case kCustomGusMelodicPatches:
					fileWriteLine(fp, "\nbank 0\n\n");
					break;
				case kCustomGusDrumPatches:
					fileWriteLine(fp, "\ndrum 0\n\n");
					break;
				}
				prevState = state;
			}
			char *q = strchr(p, '=');
			if (q && q < next) {
				const int index = atoi(p);
				char *nl = strpbrk(q + 1, "\r\n");
				if (nl) {
					*nl = 0;
					fileWriteLine(fp, "%3d\t%s.pat\n", index, q + 1);
				}
			}
                }
		if (!next) {
			break;
		}
		p = next + 1;
	}
}

struct XmiPlayerImpl {

	midi *_midiHandle;
	uint8_t *_midiBuffer;
	int _samples;

	XmiPlayerImpl() {
		_midiHandle = 0;
		_midiBuffer = 0;
		_samples = 0;
	}

	void init(int mixingRate) {
		char *gusData = readFile("CUSTOM.GUS", kFileType_DRIVERS);
		if (gusData) {
			char path[MAXPATHLEN];
			snprintf(path, sizeof(path), "%s/wildmidi.cfg", g_fileSavePath);
			File *fp = fileOpen(path, 0, kFileType_CONFIG, false);
			if (fp) {
				convertCustomGus(gusData, fp);
				fileClose(fp);
			}
			WildMidi_Init(path, mixingRate, WM_MO_ENHANCED_RESAMPLING);
			free(gusData);
		}
	}

	void fini() {
		unload();
		WildMidi_Shutdown();
	}

	void load(const uint8_t *data, int dataSize) {
		_midiBuffer = (uint8_t *)malloc(dataSize);
		if (_midiBuffer) {
			memcpy(_midiBuffer, data, dataSize);
			_midiHandle = WildMidi_OpenBuffer(_midiBuffer, dataSize);
		}
	}

	void unload() {
		if (_midiHandle) {
			WildMidi_Close(_midiHandle);
			_midiHandle = 0;
		}
		if (_midiBuffer) {
			free(_midiBuffer);
			_midiBuffer = 0;
		}
	}

	void readSamples(int16_t *buf, int len) {
		if (_midiHandle) {
			_samples = WildMidi_GetOutput(_midiHandle, (int8_t *)buf, len * 2);
		} else {
			_samples = 0;
		}
	}
};

XmiPlayer::XmiPlayer() {
	_impl = new XmiPlayerImpl();
}

XmiPlayer::~XmiPlayer() {
	_impl->fini();
	delete _impl;
}

void XmiPlayer::setRate(int mixingRate) {
	_impl->init(mixingRate);
}

void XmiPlayer::load(const uint8_t *data, int size) {
	_impl->unload();
	return _impl->load(data, size);
}

void XmiPlayer::unload() {
	_impl->unload();
}

void XmiPlayer::readSamples(int16_t *buf, int len) {
	_impl->readSamples(buf, len);
}
