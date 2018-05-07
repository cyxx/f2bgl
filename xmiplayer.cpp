
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
		if (_midiHandle) {
			WildMidi_Close(_midiHandle);
			_midiHandle = 0;
		}
		if (_midiBuffer) {
			free(_midiBuffer);
			_midiBuffer = 0;
		}
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
		//
		// Enabling 'enhanced resampling' option will make the WildMidi code
		// occupying most of the CPU time when playing (tested on 0.4.2).
		// Linear resampling (default) sounds good enough in my tests, so
		// let's use that.
		//
		// 330,005,146  ???:WildMidi_GetOutput [/usr/lib/x86_64-linux-gnu/libWildMidi.so.2.0.0]
		// 197,166,524  /build/glibc-MECilU/glibc-2.24/math/../sysdeps/ieee754/dbl-64/s_sin.c:__sin_avx [/lib/x86_64-linux-gnu/libm-2.24.so]
		//
		const int options = 0; // WM_MO_REVERB | WM_MO_ENHANCED_RESAMPLING;
		const int ret = WildMidi_Init(path, mixingRate, options);
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
			WildMidi_SetOption(_midiHandle, WM_MO_LOOP, WM_MO_LOOP);
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

//
// FluidSynth
//

#include <fluidsynth.h>

struct XmiEvent {
	int deltaTime;
	uint8_t command;
	uint8_t param1;
	uint8_t param2;
};

static int compareXmiEvent(const void *p1, const void *p2) {
	const XmiEvent *event1 = (const XmiEvent *)p1;
	const XmiEvent *event2 = (const XmiEvent *)p2;
	return event1->deltaTime - event2->deltaTime;
}

struct XmiParser {

	enum {
		kMaxXmiEventsCount = 24576
	};

	XmiEvent _events[kMaxXmiEventsCount];
	int _eventsCount;
	int _ticks;

	const uint8_t *_data;
	int _dataOffset;
	int _dataSize;

	XmiParser() {
		memset(&_events, 0, sizeof(_events));
		_eventsCount = 0;
		_ticks = 0;
	}

	uint32_t readVLQ() {
		uint32_t value = 0;
		for (int i = 0; i < 4 && _dataOffset < _dataSize; ++i) {
			const int b = _data[_dataOffset++];
			value = (value << 7) | (b & 0x7F);
			if (!(b & 0x80)) {
				break;
			}
		}
		return value;
	}

	bool readTag(const char *tag) {
		const bool matches = memcmp(_data + _dataOffset, tag, strlen(tag)) == 0;
		if (!matches) {
			warning("XMI tag '%s' not found, offset 0x%x", tag, _dataOffset);
		}
		_dataOffset += strlen(tag);
		return matches;
	}

	void parseXmi_TIMB(int len) {
		const int timbresCount = READ_LE_UINT16(_data + _dataOffset); _dataOffset += 2;
		for (int i = 0; i < timbresCount; ++i) {
			// patch = _data[_dataOffset];
			// bank = _data[_dataOffset + 1];
			_dataOffset += 2;
		}
	}

	void parseXmi_EVNT(int len) {
		const int endOffset = _dataOffset + len;
		while (_dataOffset < endOffset) {
			int intervalCounter = 0;
			int b = _data[_dataOffset++];
			while ((b & 0x80) == 0) {
				intervalCounter += b;
				b = _data[_dataOffset++];
				if (_dataOffset >= endOffset) {
					return;
				}
			}
			_ticks += intervalCounter;
			const int command = b;
			debug(kDebug_XMIDI, "delta %d event 0x%x (cmd 0x%x channel 0x%x)", intervalCounter, command, command >> 4, command & 15);
			if (_eventsCount >= kMaxXmiEventsCount) {
				warning("Maximum number of MIDI events, count %d", _eventsCount);
				return;
			}
			XmiEvent *event = &_events[_eventsCount++];
			event->deltaTime = _ticks;
			event->command = command;
			switch (command & 0xF0) {
			case 0x80: // __note_off
				event->param1 = _data[_dataOffset++];
				event->param2 = _data[_dataOffset++];
				debug(kDebug_XMIDI, "NoteOff %d %d", event->param1, event->param2);
				break;
			case 0x90: // __note_on
				event->param1 = _data[_dataOffset++];
				event->param2 = _data[_dataOffset++];
				{ // hanging note
					const int interval = readVLQ();
					debug(kDebug_XMIDI, "NoteOn %d %d %d", event->param1, event->param2, interval);
					if (_eventsCount >= kMaxXmiEventsCount) {
						warning("Maximum number of MIDI events, count %d", _eventsCount);
						return;
					}
					XmiEvent *eventNoteOff = &_events[_eventsCount++];
					eventNoteOff->deltaTime = _ticks + interval;
					eventNoteOff->command = 0x80 | (command & 0xF);
					eventNoteOff->param1 = event->param1;
					eventNoteOff->param2 = 0;
				}
				break;
			case 0xB0: // _ctrl_change
				event->param1 = _data[_dataOffset++];
				event->param2 = _data[_dataOffset++];
				debug(kDebug_XMIDI, "ControlChange %d %d", event->param1, event->param2);
				break;
			case 0xC0: // __prg_change
				event->param1 = _data[_dataOffset++];
				debug(kDebug_XMIDI, "ProgramChange %d", event->param1);
				break;
			case 0xE0: // __pitch_wheel
				event->param1 = _data[_dataOffset++];
				event->param2 = _data[_dataOffset++];
				debug(kDebug_XMIDI, "PitchWheel %d %d", event->param1, event->param2);
				break;
			case 0xF0: // __sysex
				if ((command & 0xF) == 0xF) {
					const int type = _data[_dataOffset++];
					const int len = readVLQ();
					debug(kDebug_XMIDI, "SysEx type 0x%x length %d", type, len);
					if (type == 0x2F) { // EndOfTrack
						return;
					}
					_dataOffset += len;
					break;
				}
				// fall-through
			default:
				warning("Unhandled XMI command 0x%x offset 0x%x", command, _dataOffset);
				break;
			}
		}
	}

	enum {
		kStage_INFO,
		kStage_CAT
	};

	int parseXmiTags(int stage) {
		char tag[4];
		memcpy(tag, _data + _dataOffset, sizeof(tag)); _dataOffset += sizeof(tag);
		const int len = READ_BE_UINT32(_data + _dataOffset); _dataOffset += 4;
		debug(kDebug_XMIDI, "Found XMI tag '%c%c%c%c', len %d", tag[0], tag[1], tag[2], tag[3], len);
		switch (stage) {
		case kStage_INFO:
			if (memcmp(tag, "INFO", 4) == 0) {
				const int numTracks = READ_LE_UINT16(_data + _dataOffset);
				if (numTracks != 1) {
					warning("Unhandled XMI numTracks %d", numTracks);
				}
				_dataOffset += len;
				break;
			}
			// fall-through
		case kStage_CAT:
			if (memcmp(tag, "TIMB", 4) == 0) {
				parseXmi_TIMB(len);
				break;
			}
			if (memcmp(tag, "EVNT", 4) == 0) {
				parseXmi_EVNT(len);
				qsort(_events, _eventsCount, sizeof(XmiEvent), compareXmiEvent);
				break;
			}
			// fall-through
		default:
			warning("Unhandled XMI tag '%c%c%c%c' stage %d offset 0x%x", tag[0], tag[1], tag[2], tag[3], stage, _dataOffset);
			_dataOffset += len;
			break;
		}
	        return 8 + len;
	}

	void loadXmi(const uint8_t *data, int dataSize) {
		_data = data;
		_dataOffset = 0;
		_dataSize = dataSize;

		_eventsCount = 0;
		_ticks = 0;

		readTag("FORM");
		int formLen = READ_BE_UINT32(_data + _dataOffset); _dataOffset += 4;
		int offset = 0;
		readTag("XDIR"); offset += 4;
		while (offset < formLen) {
			offset += parseXmiTags(kStage_INFO);
		}
		readTag("CAT ");
		const int catLen = READ_BE_UINT32(_data + _dataOffset); _dataOffset += 4;
		offset = 0;
		readTag("XMID"); offset += 4;
		while (offset < catLen) {
			readTag("FORM");
			formLen = READ_BE_UINT32(_data + _dataOffset); _dataOffset += 4;
			readTag("XMID");
			int offset2 = 4;
			while (offset2 < formLen) {
				offset2 += parseXmiTags(kStage_CAT);
			}
			offset += formLen + 8;
		}
	}
};

struct XmiPlayer_FluidSynth : XmiPlayer {

	const char *_sf2;

	XmiParser _xmiParser;

	fluid_settings_t *_fluidSettings;
	fluid_synth_t *_fluidSynth;
	int _soundFont;

	int _samplesPerTick;
	int _tickDuration;

	int _samplesLeft;
	int _currentTick;
	int _currentXmiEvent;

	XmiPlayer_FluidSynth(const char *sf2)
		: _sf2(sf2) {
		_fluidSettings = 0;
		_fluidSynth = 0;

		_soundFont = -1;

		_samplesPerTick = 0;
		_tickDuration = 0;
	}

	~XmiPlayer_FluidSynth() {
		if (_fluidSynth) {
			delete_fluid_synth(_fluidSynth);
		}
		if (_fluidSettings) {
			delete_fluid_settings(_fluidSettings);
		}
		if (!(_soundFont < 0)) {
			fluid_synth_sfunload(_fluidSynth, _soundFont, 1);
		}
	}

	virtual void setRate(int rate) {
		// tempo = 500000 (microseconds per quarter note)
		// ppqn = 60 (pulses per quarter note)
		// tickHz = 1000000 / ( 500000 / 60 )
		_samplesPerTick = rate / 120;
		_tickDuration = 500000 / 60;

		_fluidSettings = new_fluid_settings();
		fluid_settings_setnum(_fluidSettings, "synth.sample-rate", rate);
		fluid_settings_setstr(_fluidSettings, "synth.midi-bank-select", "gm");

		_fluidSynth = new_fluid_synth(_fluidSettings);
		_soundFont = fluid_synth_sfload(_fluidSynth, _sf2, 1);
		if (_soundFont < 0) {
			warning("Failed to load soundfont '%s', ret %d", _sf2, _soundFont);
		}
	}
	virtual void setVolume(int volume) {
		if (_fluidSettings) {
			fluid_settings_setnum(_fluidSettings, "synth.gain", .8 * volume / 255);
		}
	}

	virtual void load(const uint8_t *data, int size) {
		_xmiParser.loadXmi(data, size);

		_samplesLeft = 0;
		_currentTick = 0;
		_currentXmiEvent = 0;
	}
	virtual void unload() {
		if (_fluidSynth) {
			for (int ch = 0; ch < 16; ++ch) {
				fluid_synth_all_sounds_off(_fluidSynth, ch); // MIDI CC 120
			}
		}
	}

	void handleTick() {
		const int nextTick = _currentTick + _tickDuration;
		while (_currentXmiEvent < _xmiParser._eventsCount && _xmiParser._events[_currentXmiEvent].deltaTime * _tickDuration < nextTick) {
			const XmiEvent *event = &_xmiParser._events[_currentXmiEvent];
			++_currentXmiEvent;
			debug(kDebug_XMIDI, "Handling XMI event %d cmd 0x%x", _currentXmiEvent, event->command);
			const int channel = event->command & 0xF;
			switch (event->command & 0xF0) {
			case 0x80:
				fluid_synth_noteoff(_fluidSynth, channel, event->param1);
				break;
			case 0x90:
				fluid_synth_noteon(_fluidSynth, channel, event->param1, event->param2);
				break;
			case 0xB0:
				fluid_synth_cc(_fluidSynth, channel, event->param1, event->param2);
				break;
			case 0xC0:
				fluid_synth_program_change(_fluidSynth, channel, event->param1);
				break;
			case 0xE0:
				fluid_synth_pitch_bend(_fluidSynth, channel, (event->param2 << 7) | event->param1);
				break;
			}
		}
		if (_currentXmiEvent >= _xmiParser._eventsCount) { // loop, rewind at the beginning of the song
			_currentXmiEvent = 0;
			_currentTick = 0;
		}
	}

	virtual void readSamples(int16_t *buf, int len) {
		len /= 2; // stereo samples
		int16_t *buffer = buf;
		while (len > 0) {
			if (_samplesLeft == 0) {
				handleTick();
				_currentTick += _tickDuration;
				_samplesLeft = _samplesPerTick;
			}
			const int count = MIN(_samplesLeft, len);
			fluid_synth_write_s16(_fluidSynth, count, buffer, 0, 2, buffer, 1, 2);
			buffer += count * 2;
			_samplesLeft -= count;
			len -= count;
		}
	}
};

XmiPlayer *XmiPlayer_FluidSynth_create(const char *sfPath) {
	return new XmiPlayer_FluidSynth(sfPath);
}
