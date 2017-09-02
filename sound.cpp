/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include "file.h"
#include "resource.h"
#include "sound.h"

Sound::Sound(Resource *res)
	: _res(res), _mix() {
	_sfxVolume = kDefaultVolume;
	_sfxPan = kDefaultPan;
	_digiCount = 0;
	_digiTable = 0;
	_fpSnd = 0;
	_midiCount = 0;
	_midiTable = 0;
	_fpSng = 0;
}

Sound::~Sound() {
	free(_digiTable);
	_digiTable = 0;
	if (_fpSnd) {
		fileClose(_fpSnd);
		_fpSnd = 0;
	}
	free(_midiTable);
	_midiTable = 0;
	if (_fpSng) {
		fileClose(_fpSng);
		_fpSng = 0;
	}
}

static const char *getSngName(int type) {
	const char *name = 0;
	switch (type) {
	case MIDI_AWE32:
		name = "AWE32";
		break;
	case MIDI_MT32:
		name = "MT32";
		break;
	case MIDI_GUS:
		if (!g_isDemo) {
			name = "GUS";
		}
		// fall-through
	default:
		name = "FM";
		break;
	}
	static char fname[16];
	snprintf(fname, sizeof(fname), "%s%s.SNG", g_isDemo ? "L1" : "", name);
	return fname;
}

void Sound::init(int midiType) {
	int dataSize;
	_digiCompressed = !g_isDemo;
	_fpSnd = fileOpen(g_isDemo ? "L1DIGI.SND" : "DIGICMP.SND", &dataSize, kFileType_SOUND);
	loadDigiSnd(_fpSnd);
	_fpSng = fileOpen(getSngName(midiType), &dataSize, kFileType_SOUND);
	loadMidiSng(_fpSng);
}

void Sound::loadDigiSnd(File *fp) {
	struct {
		uint32_t name;
		uint32_t data;
	} offsets[1024];
	_digiCount = 0;
	for (int i = 0; i < 1024; ++i) {
		offsets[i].name = fileReadUint32LE(fp);
		offsets[i].data = fileReadUint32LE(fp);
		if (i != 0 && offsets[i].name == offsets[0].data) {
			_digiCount = i;
			break;
		}
	}
	_digiTable = (DigiSnd *)calloc(_digiCount, sizeof(DigiSnd));
	if (_digiTable) {
		for (int i = 0; i < _digiCount; ++i) {
			fileSetPos(fp, offsets[i].name, kFilePosition_SET);
			char *p = _digiTable[i].name;
			for (int j = 0; j < 16; ++j) {
				p[j] = fileReadByte(fp);
				if (p[j] == 0) {
					break;
				}
			}
			_digiTable[i].offset = offsets[i].data;
			_digiTable[i].size = offsets[i + 1].data - offsets[i].data;
		}
		debug(kDebug_SOUND, "loadDigiSnd() count %d", _digiCount);
	}
}

void Sound::loadMidiSng(File *fp) {
	struct {
		uint32_t name;
		uint32_t data;
	} offsets[1024];
	_midiCount = 0;
	for (int i = 0; i < 1024; ++i) {
		offsets[i].name = fileReadUint32LE(fp);
		offsets[i].data = fileReadUint32LE(fp);
		if (i != 0 && offsets[i].name == offsets[0].data) {
			_midiCount = i;
			break;
		}
	}
	_midiTable = (MidiSng *)calloc(_midiCount, sizeof(MidiSng));
	if (_midiTable) {
		for (int i = 0; i < _midiCount; ++i) {
			fileSetPos(fp, offsets[i].name, kFilePosition_SET);
			char *p = _midiTable[i].name;
			for (int j = 0; j < 16; ++j) {
				p[j] = fileReadByte(fp);
				if (p[j] == 0) {
					break;
				}
			}
			_midiTable[i].offset = offsets[i].data;
			_midiTable[i].size = offsets[i + 1].data - offsets[i].data;
		}
		debug(kDebug_SOUND, "loadMidiSng() count %d", _midiCount);
	}
}

const DigiSnd *Sound::findDigiSndByName(const char *name) const {
	for (int i = 0; i < _digiCount; ++i) {
		if (strcasecmp(_digiTable[i].name, name) == 0) {
			return &_digiTable[i];
		}
	}
	return 0;
}

const MidiSng *Sound::findMidiSngByName(const char *name) const {
	for (int i = 0; i < _midiCount; ++i) {
		if (strcasecmp(_midiTable[i].name, name) == 0) {
			return &_midiTable[i];
		}
	}
	return 0;
}

void Sound::setVolume(int volume) {
	_sfxVolume = volume;
}

void Sound::setPan(int pan) {
	_sfxPan = pan;
}

static uint32_t makeId(uint16_t a, uint16_t b = 0xFFFF) {
	return (a << 16) | b;
}

void Sound::playSfx(int16_t objKey, int16_t sndKey) {
	if (sndKey != -1) {
		const uint32_t id = makeId(objKey, sndKey);
		if (_mix.isWavPlaying(id)) {
			// already playing
			return;
		}
		const uint8_t *p_sndtype = _res->getData(kResType_SND, sndKey, "SNDTYPE");
		assert(p_sndtype && READ_LE_UINT32(p_sndtype) == 16);
		const uint8_t *p_sndinfo = _res->getData(kResType_SND, sndKey, "SNDINFO");
		const DigiSnd *dc = findDigiSndByName((const char *)p_sndinfo);
		if (dc) {
			debug(kDebug_SOUND, "Sound::playSfx() '%s' offset 0x%X", (const char *)p_sndinfo, dc->offset);
			fileSetPos(_fpSnd, dc->offset, kFilePosition_SET);
			_mix.playWav(_fpSnd, dc->size, _sfxVolume, _sfxPan, id, false, _digiCompressed);
		}
	}
	_sfxVolume = kDefaultVolume;
	_sfxPan = kDefaultPan;
}

void Sound::stopSfx(int16_t objKey, int16_t sndKey) {
	_mix.stopWav(makeId(objKey, sndKey));
}

void Sound::playVoice(int16_t objKey, uint32_t crc) {
	char name[16];
	snprintf(name, sizeof(name), "%08X.WAV", crc);
	int dataSize;
	File *fp = fileOpen(name, &dataSize, kFileType_VOICE, false);
	debug(kDebug_SOUND, "Sound::playVoice() '%s' dataSize %d", name, dataSize);
	if (fp) {
		_mix.playWav(fp, dataSize, kDefaultVolume, kDefaultPan, makeId(objKey), true);
		fileClose(fp);
	}
}

void Sound::stopVoice(int16_t objKey) {
	_mix.stopWav(makeId(objKey));
}

bool Sound::isVoicePlaying(int16_t objKey) const {
	return _mix.isWavPlaying(makeId(objKey));
}

void Sound::playMidi(int16_t objKey, int16_t sndKey) {
	const uint8_t *p_sndinfo = _res->getData(kResType_SND, sndKey, "SNDINFO");
	if (p_sndinfo && READ_LE_UINT32(p_sndinfo + 32) == 2) {
		debug(kDebug_SOUND, "Sound::playMidi() key %d '%s'", sndKey, (const char *)p_sndinfo);
		playMidi((const char *)p_sndinfo);
	}
}

void Sound::stopMidi(int16_t objKey, int16_t sndKey) {
	debug(kDebug_SOUND, "Sound::stopMidi() key 0x%x", objKey);
	_mix.stopXmi();
}

void Sound::playMidi(const char *name) {
	const MidiSng *ms = findMidiSngByName(name);
	if (ms) {
		debug(kDebug_SOUND, "Sound::playMidi() offset 0x%x size %d", ms->offset, ms->size);
		fileSetPos(_fpSng, ms->offset, kFilePosition_SET);
		_mix.playXmi(_fpSng, ms->size);
	} else {
		warning("MIDI file '%s' not found", name);
	}
}
