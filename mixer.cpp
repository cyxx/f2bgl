/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include "file.h"
#include "mixer.h"
#include "render.h"
#include "xmiplayer.h"

static const int16_t _delta16Table[128] = {
	    0,      1,      2,      3,      4,      5,      6,      7,
	    8,      9,     10,     11,     12,     13,     14,     15,
	   17,     18,     20,     21,     23,     25,     27,     30,
	   32,     35,     38,     41,     45,     49,     53,     58,
	   62,     68,     74,     80,     87,     94,    102,    111,
	  120,    130,    141,    153,    166,    181,    196,    212,
	  230,    250,    271,    294,    319,    346,    376,    407,
	  442,    479,    520,    564,    612,    663,    720,    781,
	  847,    918,    996,   1080,   1172,   1271,   1379,   1495,
	 1622,   1759,   1908,   2070,   2245,   2435,   2641,   2865,
	 3107,   3370,   3655,   3964,   4300,   4664,   5059,   5487,
	 5951,   6455,   7001,   7593,   8236,   8933,   9689,  10508,
	11398,  12362,  13408,  14543,  15774,  17108,  18556,  20126,
	21829,  23677,  25680,  27853,  30210,      0,      0,      0,
            0,      0,      0,      0,      0,      0,      0,      0,
            0,      0,      0,      0,      0,      0,      0,      0
};

static int clipS16(int sample) {
	return CLIP(sample, -32768, 32767);
}

struct Delta16Decoder {
	int _firstSample;
	int _delta;

	Delta16Decoder()
		: _firstSample(0), _delta(0) {
	}

	int decode(uint8_t data) {
		switch (_firstSample) {
		case 0:
			_delta = data;
			_firstSample = 1;
			break;
		case 1:
			_delta = (int16_t)((data << 8) | _delta);
			_firstSample = 2;
			break;
		default:
			if (data & (1 << 7)) {
				_delta += _delta16Table[data - 128];
			} else {
				_delta -= _delta16Table[127 - data];
			}
			break;
		}
		return _firstSample == 1 ? 0 : clipS16(_delta);
	}
};

struct SoundDataWav {
	int _bufSize;
	uint8_t *_buf;
	int _bitsPerSample;
	bool _stereo;

	SoundDataWav()
		: _bufSize(0), _buf(0) {
	}

	bool load(File *fp, int dataSize, int mixerSampleRate) {
		const int pos = fileGetPos(fp);
		char buf[8];
		fileRead(fp, buf, 8);
		if (memcmp(buf, "RIFF", 4) != 0) {
			return false;
		}
		fileRead(fp, buf, 8);
		if (memcmp(buf, "WAVEfmt ", 8) != 0) {
			return false;
		}
		fileReadUint32LE(fp); // fmtLength
		const int compression = fileReadUint16LE(fp);
		assert(compression == 1);
		const int channels = fileReadUint16LE(fp);
		assert(channels == 1);
		const int sampleRate = fileReadUint32LE(fp);
		assert(sampleRate == mixerSampleRate);
		fileReadUint32LE(fp); // averageBytesPerSec
		fileReadUint16LE(fp); // blockAlign
		_bitsPerSample = fileReadUint16LE(fp);
		assert(_bitsPerSample == 16);
		debug(kDebug_SOUND, "wav/pcm format compression %d channels %d rate %d bits %d", compression, channels, sampleRate, _bitsPerSample);
		_stereo = (channels == 2);
//		_bufReadStep = (sampleRate << _fracStepBits) / mixerSampleRate;
		fileRead(fp, buf, 4);
		if (memcmp(buf, "data", 4) != 0) {
			return false;
		}
		const int chunkSize = fileReadUint32LE(fp);
		const int headerSize = fileGetPos(fp) - pos;
		assert(dataSize > headerSize);
		_bufSize = dataSize - headerSize;
		debug(kDebug_SOUND, "chunk size %d header size %d buf size %d", chunkSize, headerSize, _bufSize);
		_buf = (uint8_t *)malloc(_bufSize);
		if (_buf) {
			fileRead(fp, _buf, _bufSize);
		}
                return true;
	}
};

static void mix(int16_t *dst, int pcm, int volume) {
	pcm = *dst + pcm * volume / 127;
	*dst = clipS16(pcm);
}

struct MixerSound {
	SoundDataWav data;
	int readOffset;
	bool compressed;
	Delta16Decoder decoder;
	int volumeL;
	int volumeR;

	bool read(int16_t *dst, int len) {
		int sample;
		assert((len & 1) == 0);
		for (int i = 0; i < len; i += 2) {
			if (compressed) {
				sample = decoder.decode(data._buf[readOffset]);
				++readOffset;
				if (readOffset == 1) {
					sample = decoder.decode(data._buf[readOffset]);
					++readOffset;
				}
			} else {
				sample = (int16_t)READ_LE_UINT16(data._buf + readOffset);
				readOffset += 2;
			}
			if (!data._stereo) {
				mix(&dst[i + 0], sample, volumeL);
				mix(&dst[i + 1], sample, volumeR);
			}
			if (readOffset >= data._bufSize) {
				return false;
			}
		}
		return true;
	}
};

struct MixerQueueList {
	uint8_t *buffer;
	int size;
	int read;
	struct MixerQueueList *next;
};

struct MixerQueue {
	Delta16Decoder decoder;
	int size;
	int preloadSize;
	MixerQueueList *head;
};

static void nullMixerLock(int lock) {
}

struct MixerLock {
	void (*_lock)(int);
	MixerLock(void (*lock)(int))
		: _lock(lock) {
		_lock(1);
	}
	~MixerLock() {
		_lock(0);
	}
};

Mixer::Mixer() {
	_rate = 0;
	memset(_soundsTable, 0, sizeof(_soundsTable));
	_queue = 0;
	_xmiPlayer = 0;
	memset(_idsMap, 0, sizeof(_idsMap));
	_lock = &nullMixerLock;
	_soundVolume = kDefaultVolume;
	_musicVolume = kDefaultVolume;
	_voiceVolume = kDefaultVolume;
}

Mixer::~Mixer() {
	for (int i = 0; i < kMaxSoundsCount; ++i) {
		stopWav(_idsMap[i]);
	}
	stopQueue();
}

void Mixer::setSoundVolume(int volume) {
	_soundVolume = volume;
}

void Mixer::setMusicVolume(int volume) {
	_musicVolume = volume;
	if (_xmiPlayer) {
		_xmiPlayer->setVolume(_musicVolume * 255 / kDefaultVolume);
	}
}

void Mixer::setVoiceVolume(int volume) {
	_voiceVolume = volume;
}

void Mixer::setFormat(int rate, int fmt) {
	_rate = rate;
	if (_xmiPlayer) {
		_xmiPlayer->setRate(rate);
	}
}

int Mixer::findIndexById(uint32_t id) const {
	for (int i = 0; i < kMaxSoundsCount; ++i) {
		if (_idsMap[i] == id) {
			return i;
		}
	}
	return -1;
}

void Mixer::playWav(File *fp, int dataSize, int volume, int pan, uint32_t id, bool isVoice, bool compressed) {
	MixerSound *snd = new MixerSound();
	snd->data.load(fp, dataSize, _rate);
	snd->readOffset = 0;
	snd->compressed = compressed;
	assert(volume >= 0 && volume < 128);
	if (isVoice) {
		volume = volume * _voiceVolume / kDefaultVolume;
	} else {
		volume = volume * _soundVolume / kDefaultVolume;
	}
	assert(pan >= 0 && pan < 128);
	pan -= 64; // to -64,64 range
	if (pan < 0) {
		snd->volumeL = -pan * volume / 64;
		snd->volumeR = (64 + pan) * volume / 64;
	} else if (pan > 0) {
		snd->volumeL = (64 - pan) * volume / 64;
		snd->volumeR =  pan * volume / 64;
	} else {
		snd->volumeL = volume;
		snd->volumeR = volume;
	}
	MixerLock ml(_lock);
	for (int i = 0; i < kMaxSoundsCount; ++i) {
		if (!_soundsTable[i]) {
			_soundsTable[i] = snd;
			_idsMap[i] = id;
			break;
		}
	}
}

void Mixer::stopWav(uint32_t id) {
	MixerLock ml(_lock);
	const int i = findIndexById(id);
	if (i != -1) {
		delete _soundsTable[i];
		_soundsTable[i] = 0;
		_idsMap[i] = 0;
	}
}

bool Mixer::isWavPlaying(uint32_t id) const {
	MixerLock ml(_lock);
	const int i = findIndexById(id);
	return i != -1;
}

void Mixer::playQueue(int preloadSize) {
	stopQueue();
	MixerLock ml(_lock);
	_queue = new MixerQueue;
	_queue->preloadSize = preloadSize;
	_queue->size = 0;
	_queue->head = 0;
}

void Mixer::appendToQueue(const uint8_t *buf, int size) {
	MixerLock ml(_lock);
	MixerQueue *mq = _queue;
	if (mq) {
		MixerQueueList *mql = new MixerQueueList;
		mql->buffer = (uint8_t *)malloc(size);
		memcpy(mql->buffer, buf, size);
		mql->read = 0;
		mql->size = size;
		mql->next = 0;
		if (!mq->head) {
			mq->head = mql;
		} else {
			MixerQueueList *current = mq->head;
			while (current->next) {
				current = current->next;
			}
			current->next = mql;
		}
		if (mq->size < mq->preloadSize) {
			++mq->size;
		}
	}
}

void Mixer::stopQueue() {
	MixerLock ml(_lock);
	if (_queue) {
		MixerQueueList *mql = _queue->head;
		while (mql) {
			MixerQueueList *next = mql->next;
			free(mql->buffer);
			delete mql;
			mql = next;
		}
		delete _queue;
		_queue = 0;
	}
}

void Mixer::playXmi(File *f, int size) {
	MixerLock ml(_lock);
	uint8_t *buf = (uint8_t *)malloc(size);
	if (buf) {
		fileRead(f, buf, size);
		_xmiPlayer->load(buf, size);
		free(buf);
	}
}

void Mixer::stopXmi() {
	MixerLock ml(_lock);
	_xmiPlayer->unload();
}

void Mixer::mixBuf(int16_t *buf, int len) {
	assert((len & 1) == 0);
	memset(buf, 0, len * sizeof(int16_t));
	if (!_queue && _xmiPlayer) {
		_xmiPlayer->readSamples(buf, len);
	} else if (_queue->size >= _queue->preloadSize) {
		MixerQueueList *mql = _queue->head;
		for (int i = 0; mql && i < len; i += 2) {
			int sample = _queue->decoder.decode(mql->buffer[mql->read]);
			++mql->read;
			if (mql->read == 1) {
				sample = _queue->decoder.decode(mql->buffer[mql->read]);
				++mql->read;
			}
			mix(&buf[i + 0], sample, _musicVolume);
			mix(&buf[i + 1], sample, _musicVolume);
			if (mql->read >= mql->size) {
				MixerQueueList *next = mql->next;
				free(mql->buffer);
				delete mql;
				mql = next;
				_queue->head = mql;
			}
		}
	}
	for (int i = 0; i < kMaxSoundsCount; ++i) {
		if (_soundsTable[i]) {
			if (!_soundsTable[i]->read(buf, len)) {
				delete _soundsTable[i];
				_soundsTable[i] = 0;
				_idsMap[i] = 0;
			}
		}
	}
}

void Mixer::mixCb(void *param, uint8_t *buf, int len) {
	((Mixer *)param)->mixBuf((int16_t *)buf, len / 2);
}
