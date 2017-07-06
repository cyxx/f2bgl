
#include <sys/param.h>
#include "resource.h"
#include "file.h"
#include "xmiplayer.h"

//
// WildMidi
//

#include <wildmidi_lib.h>

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

struct XmiPlayer_WildMidi : XmiPlayer {

	Resource *_res;
	midi *_midiHandle;
	uint8_t *_midiBuffer;

	XmiPlayer_WildMidi(Resource *res)
		: _res(res) {
		_midiHandle = 0;
		_midiBuffer = 0;
	}

	virtual ~XmiPlayer_WildMidi() {
		unload();
		const int ret = WildMidi_Shutdown();
		debug(kDebug_XMIDI, "WildMidi_Shutdown() ret %d", ret);
	}

	virtual void setRate(int mixingRate) {
		static const char *fname = "wildmidi.cfg";
		char path[MAXPATHLEN];
		snprintf(path, sizeof(path), "%s/%s", g_fileSavePath, fname);
		if (!fileExists(fname, kFileType_CONFIG)) {
			File *fp = fileOpen(fname, 0, kFileType_CONFIG, false);
			if (fp) {
				writeConfigurationFile(fp, _res);
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

	virtual void setVolume(int volume) {
		WildMidi_MasterVolume(volume);
	}

	virtual void load(const uint8_t *data, int dataSize) {
		_midiBuffer = (uint8_t *)malloc(dataSize);
		if (_midiBuffer) {
			memcpy(_midiBuffer, data, dataSize);
			_midiHandle = WildMidi_OpenBuffer(_midiBuffer, dataSize);
		}
	}

	virtual void unload() {
		if (_midiHandle) {
			WildMidi_Close(_midiHandle);
			_midiHandle = 0;
		}
		if (_midiBuffer) {
			free(_midiBuffer);
			_midiBuffer = 0;
		}
	}

	virtual void readSamples(int16_t *buf, int len) {
		if (_midiHandle) {
			WildMidi_GetOutput(_midiHandle, (int8_t *)buf, len * 2);
		}
	}
};

XmiPlayer *XmiPlayer_WildMidi_create(Resource *res) {
	return new XmiPlayer_WildMidi(res);
}
