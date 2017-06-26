
#include <sys/param.h>
#include <wildmidi_lib.h>
#include "resource.h"
#include "file.h"
#include "xmiplayer.h"

// convert drivers/custom.gus to wildmidi configuration file
static void writeConfigurationFile(File *fp, Resource *res) {
	fileWriteLine(fp, "\ndir %s/DATA/DRIVERS\n\n", g_fileDataPath);
	fileWriteLine(fp, "\nbank 0\n\n");
	for (int i = 0; i < 128; ++i) {
		if (res->_gusPatches[i][0]) {
			fileWriteLine(fp, "%3d\t%s.pat\n", i, res->_gusPatches[i]);
		}
	}
	fileWriteLine(fp, "\ndrumset 0\n\n");
	for (int i = 128; i < 256; ++i) {
		if (res->_gusPatches[i][0]) {
			fileWriteLine(fp, "%3d\t%s.pat\n", i - 128, res->_gusPatches[i]);
		}
	}
}

struct XmiPlayerImpl {

	midi *_midiHandle;
	uint8_t *_midiBuffer;

	XmiPlayerImpl() {
		_midiHandle = 0;
		_midiBuffer = 0;
	}

	void init(int mixingRate, Resource *res) {
		static const char *fname = "wildmidi.cfg";
		char path[MAXPATHLEN];
		snprintf(path, sizeof(path), "%s/%s", g_fileSavePath, fname);
		if (!fileExists(fname, kFileType_CONFIG)) {
			File *fp = fileOpen(fname, 0, kFileType_CONFIG, false);
			if (fp) {
				writeConfigurationFile(fp, res);
				fileClose(fp);
			}
		}
		const int ret = WildMidi_Init(path, mixingRate, WM_MO_ENHANCED_RESAMPLING);
		debug(kDebug_XMIDI, "WildMidi_Init() path '%s' ret %d", path, ret);
		if (ret != 0) {
			const char *err = WildMidi_GetError();
			warning("Error initializing WildMIDI ret %d '%s'", ret, err);
		}
	}

	void setVolume(int volume) {
		WildMidi_MasterVolume(volume);
	}

	void fini() {
		unload();
		const int ret = WildMidi_Shutdown();
		debug(kDebug_XMIDI, "WildMidi_Shutdown() ret %d", ret);
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
			WildMidi_GetOutput(_midiHandle, (int8_t *)buf, len * 2);
		}
	}
};

XmiPlayer::XmiPlayer(Resource *res)
	: _res(res) {
	_impl = new XmiPlayerImpl();
}

XmiPlayer::~XmiPlayer() {
	_impl->fini();
	delete _impl;
}

void XmiPlayer::setRate(int mixingRate) {
	debug(kDebug_XMIDI, "XmiPlayer::setRate() rate %d", mixingRate);
	_impl->init(mixingRate, _res);
}

void XmiPlayer::setVolume(int volume) {
	_impl->setVolume(volume);
}

void XmiPlayer::load(const uint8_t *data, int size) {
	debug(kDebug_XMIDI, "XmiPlayer::load() size %d", size);
	_impl->unload();
	return _impl->load(data, size);
}

void XmiPlayer::unload() {
	debug(kDebug_XMIDI, "XmiPlayer::unload()");
	_impl->unload();
}

void XmiPlayer::readSamples(int16_t *buf, int len) {
	_impl->readSamples(buf, len);
}
