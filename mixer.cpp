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

static int16_t lerpS16(int16_t s1, int16_t s2, uint32_t frac) {
	static const int kFracOne = (1 << kFracBits);
	static const int kFracHalf = kFracOne >> 1;
	static const int kFracBitsLow = kFracOne - 1;
	return s1 + (((s2 - s1) * (frac & kFracBitsLow) + kFracHalf) >> kFracBits);
}

struct Delta16Decoder {
	int _firstSample;
	int _delta;

	Delta16Decoder()
		: _firstSample(0), _delta(0) {
	}

	void reset() {
		_firstSample = 0;
		_delta = 0;
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

static const int16_t K0_1024[] = { 0, 960, 1840, 1568, 1952 };
static const int16_t K1_1024[] = { 0,   0, -832, -880, -960 };

static int8_t sext8(uint8_t x, int bits) {
	const int shift = 8 - bits;
	return ((int8_t)(x << shift)) >> shift;
}

struct XaDecoder {
	int _pcmL0, _pcmL1;
	int _pcmR0, _pcmR1;

	uint8_t _data[128];
	int _dataSize;
	int16_t _samples[224];
	int _samplesSize;

	bool _stereo;
	int _inputDecodeSize;

	XaDecoder() {
		memset(_data, 0, sizeof(_data));
		_dataSize = 0;
		memset(_samples, 0, sizeof(_samples));
		_samplesSize = 0;
	}

	void reset(bool stereo) {
		_pcmL0 = _pcmL1 = 0;
		_pcmR0 = _pcmR1 = 0;

		_stereo = stereo;
		_inputDecodeSize = stereo ? 128 : 16;
	}

	int decode(const uint8_t *src, int size) {
		const int count = _dataSize + size;
		if (count >= _inputDecodeSize) {
			size = _inputDecodeSize - _dataSize;
		}
		memcpy(_data + _dataSize, src, size);
		_dataSize += size;
		if (_dataSize == _inputDecodeSize) {
			if (_stereo) {
				_samplesSize = decodeGroupXa(_data, _samples);
			} else {
				_samplesSize = decodeGroupSpu(_data, _samples);
			}
			_dataSize = 0;
		} else {
			_samplesSize = 0;
		}
		assert(_dataSize < _inputDecodeSize);
		return size;
	}

	// src points to a 128 bytes buffer, dst to a 224 bytes buffer
	int decodeGroupXa(const uint8_t *src, int16_t *dst) {
		for (int i = 0; i < 4; ++i) {
			const int shiftL = 12 - (src[4 + i * 2] & 15);
			assert(shiftL >= 0);
			const int filterL = src[4 + i * 2] >> 4;
			assert(filterL < 5);
			const int shiftR = 12 - (src[5 + i * 2] & 15);
			assert(shiftR >= 0);
			const int filterR = src[5 + i * 2] >> 4;
			assert(filterR < 5);
			for (int j = 0; j < 28; ++j) {
				const uint8_t data = src[16 + i + j * 4];
				const int tL = sext8(data & 15, 4);
				const int sL = (tL << shiftL) + ((_pcmL0 * K0_1024[filterL] + _pcmL1 * K1_1024[filterL] + 512) >> 10);
				_pcmL1 = _pcmL0;
				_pcmL0 = sL;
				*dst++ = clipS16(_pcmL0);
				const int tR = sext8(data >> 4, 4);
				const int sR = (tR << shiftR) + ((_pcmR0 * K0_1024[filterR] + _pcmR1 * K1_1024[filterR] + 512) >> 10);
				_pcmR1 = _pcmR0;
				_pcmR0 = sR;
				*dst++ = clipS16(_pcmR0);
			}
		}
		return 224;
        }

	// src points to a 16 bytes buffer, dst to a 56 bytes buffer
	int decodeGroupSpu(const uint8_t *src, int16_t *dst) {
		const int shift = 12 - (*src & 15);
		assert(shift >= 0);
		const int filter = *src >> 4;
		assert(filter < 5);
		++src;
		const int flag = *src++;
		if (flag < 7) {
			for (int i = 0; i < 14; ++i) {
				const uint8_t b = *src++;
				const int t1 = sext8(b & 15, 4);
				const int s1 = (t1 << shift) + ((_pcmL0 * K0_1024[filter] + _pcmL1 * K1_1024[filter] + 512) >> 10);
				_pcmL1 = _pcmL0;
				_pcmL0 = s1;
				*dst++ = clipS16(_pcmL0);
				const int t2 = sext8(b >> 4, 4);
				const int s2 = (t2 << shift) + ((_pcmL0 * K0_1024[filter] + _pcmL1 * K1_1024[filter] + 512) >> 10);
				_pcmL1 = _pcmL0;
				_pcmL0 = s2;
				*dst++ = clipS16(_pcmL0);
			}
		} else {
			memset(dst, 0, 2 * 14 * sizeof(int16_t));
			_pcmL1 = _pcmL0 = 0;
		}
		return 56;
	}
};

struct SoundDataWav {
	int _bufSize;
	uint8_t *_buf;
	int _bitsPerSample;

	SoundDataWav()
		: _bufSize(0), _buf(0) {
	}
	~SoundDataWav() {
		free(_buf);
	}
	bool load(File *fp, int dataSize, int mixerSampleRate) {
		const int pos = fileGetPos(fp);
		char buf[8];
		fileRead(fp, buf, 8);
		if (memcmp(buf, "RIFF", 4) != 0) {
			warning("Missing WAV 'RIFF' tag");
			return false;
		}
		fileRead(fp, buf, 8);
		if (memcmp(buf, "WAVEfmt ", 8) != 0) {
			warning("Missing WAV 'WAVEfmt ' tag");
			return false;
		}
		fileReadUint32LE(fp); // fmtLength
		const int compression = fileReadUint16LE(fp);
		const int channels = fileReadUint16LE(fp);
		const int sampleRate = fileReadUint32LE(fp);
		fileReadUint32LE(fp); // averageBytesPerSec
		fileReadUint16LE(fp); // blockAlign
		_bitsPerSample = fileReadUint16LE(fp);
		if (compression != 1 || channels != 1 || sampleRate != mixerSampleRate || _bitsPerSample != 16) {
			warning("Unhandled WAV format compression %d channels rate %d bits %d", compression, channels, sampleRate, _bitsPerSample);
			return false;
		}
		fileRead(fp, buf, 4);
		if (memcmp(buf, "data", 4) != 0) {
			warning("Missing WAV 'data' tag");
			return false;
		}
		const int chunkSize = fileReadUint32LE(fp);
		const int headerSize = fileGetPos(fp) - pos;
		assert(dataSize > headerSize);
		_bufSize = dataSize - headerSize;
		debug(kDebug_SOUND, "chunk size %d header size %d buf size %d", chunkSize, headerSize, _bufSize);
		_buf = (uint8_t *)malloc(_bufSize);
		if (!_buf) {
			warning("Unable to allocate %d bytes", _bufSize);
			return false;
		}
		fileRead(fp, _buf, _bufSize);
		return true;
	}
};

struct SoundDataXa {
	int _bufSize;
	uint8_t *_buf;

	SoundDataXa()
		: _bufSize(0), _buf(0) {
	}
	~SoundDataXa() {
		free(_buf);
	}
	bool load(File *fp, int dataSize, int mixerSampleRate) {
		if (mixerSampleRate != 22050) { // SPU samples frequency
			warning("Unhandled mixer sample rate %d for XA SPU samples", mixerSampleRate);
			return false;
		}
		_bufSize = dataSize;
		_buf = (uint8_t *)malloc(_bufSize);
		if (!_buf) {
			warning("Unable to allocate %d bytes", _bufSize);
			return false;
		}
		fileRead(fp, _buf, _bufSize);
		return true;
	}
};

static void mix(int16_t *dst, int pcm, int volume) {
	pcm = *dst + pcm * volume / 127;
	*dst = clipS16(pcm);
}

struct MixerSoundWav : MixerSound {
	SoundDataWav data;
	int readOffset;
	bool compressed;
	Delta16Decoder d16Decoder;

	MixerSoundWav(bool c)
		: compressed(c) {
	}

	bool load(File *f, int dataSize, int mixerSampleRate) {
		d16Decoder.reset();
		readOffset = 0;
		return data.load(f, dataSize, mixerSampleRate);
	}

	bool readSamples(int16_t *dst, int len) {
		int sample;
		assert((len & 1) == 0);
		for (int i = 0; i < len; i += 2) {
			if (compressed) {
				sample = d16Decoder.decode(data._buf[readOffset]);
				++readOffset;
				if (readOffset == 1) {
					sample = d16Decoder.decode(data._buf[readOffset]);
					++readOffset;
				}
			} else {
				sample = (int16_t)READ_LE_UINT16(data._buf + readOffset);
				readOffset += 2;
			}
			// mono to stereo
			mix(&dst[i + 0], sample, volumeL);
			mix(&dst[i + 1], sample, volumeR);
			if (readOffset >= data._bufSize) {
				--loopsCount;
				if (loopsCount <= 0) {
					return false;
				}
				readOffset = 0;
				if (compressed) {
					d16Decoder.reset();
				}
			}
		}
		return true;
	}
};

struct MixerSoundSpu: MixerSound {
	SoundDataXa data;
	int readOffset;
	XaDecoder xaDecoder;
	int samplesOffset;

	bool load(File *f, int dataSize, int mixerSampleRate) {
		xaDecoder.reset(false); // mono
		readOffset = samplesOffset = 0;
		return data.load(f, dataSize, mixerSampleRate);
	}

	bool readSamples(int16_t *dst, int len) {
		for (int i = 0; i < len; i += 2) {
			if (samplesOffset >= xaDecoder._samplesSize) {
				const int count = xaDecoder.decode(data._buf + readOffset, data._bufSize - readOffset);
				readOffset += count;
				if (readOffset >= data._bufSize) {
					return false;
				}
				samplesOffset = 0;
			}
			// mono to stereo
			const int16_t sample = xaDecoder._samples[samplesOffset++];
			mix(&dst[i + 0], sample, kDefaultVolume);
			mix(&dst[i + 1], sample, kDefaultVolume);
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
	int type;
	Delta16Decoder d16Decoder;
	XaDecoder xaDecoder;
	int size;
	int preloadSize;
	MixerQueueList *head;
	uint32_t xaOffset;
	uint32_t xaStep;
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
	MixerSound *snd = new MixerSoundWav(compressed);
	if (!snd->load(fp, dataSize, _rate)) {
		delete snd;
		return;
	}
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
	snd->loopsCount = 1;
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

void Mixer::loopWav(uint32_t id, int count) {
	MixerLock ml(_lock);
	const int i = findIndexById(id);
	if (i != -1 && _soundsTable[i]) {
		_soundsTable[i]->loopsCount = count;
	}
}

void Mixer::playQueue(int preloadSize, int type) {
	stopQueue();
	MixerLock ml(_lock);
	_queue = new MixerQueue;
	_queue->preloadSize = preloadSize;
	_queue->type = type;
	_queue->size = 0;
	_queue->head = 0;
	if (type == kMixerQueueType_XA) {
		_queue->xaOffset = 0;
		_queue->xaStep = 2 * (37800 << kFracBits) / _rate; // stereo
		_queue->xaDecoder.reset(true); // stereo
	}
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
	_xmiPlayer->unload();
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

void Mixer::playXa(File *fp, int dataSize, uint32_t id) {
	MixerSound *snd = new MixerSoundSpu();
	if (!snd->load(fp, dataSize, _rate)) {
		delete snd;
		return;
	}
	snd->volumeL = _soundVolume;
	snd->volumeR = _soundVolume;
	snd->loopsCount = 0;
	MixerLock ml(_lock);
	for (int i = 0; i < kMaxSoundsCount; ++i) {
		if (!_soundsTable[i]) {
			_soundsTable[i] = snd;
			_idsMap[i] = id;
			break;
		}
	}
}

void Mixer::stopXa(uint32_t id) {
	stopWav(id);
}

void Mixer::mixBuf(int16_t *buf, int len) {
	assert((len & 1) == 0);
	memset(buf, 0, len * sizeof(int16_t));
	if (!_queue && _xmiPlayer) {
		_xmiPlayer->readSamples(buf, len);
	} else if (_queue->size >= _queue->preloadSize) {
		MixerQueueList *mql = _queue->head;
		switch (_queue->type) {
		case kMixerQueueType_D16:
			for (int i = 0; mql && i < len; i += 2) {
				int sample = _queue->d16Decoder.decode(mql->buffer[mql->read]);
				++mql->read;
				if (mql->read == 1) {
					sample = _queue->d16Decoder.decode(mql->buffer[mql->read]);
					++mql->read;
				}
				if (mql->read >= mql->size) {
					MixerQueueList *next = mql->next;
					free(mql->buffer);
					delete mql;
					mql = next;
					_queue->head = mql;
				}
				mix(&buf[i + 0], sample, _musicVolume);
				mix(&buf[i + 1], sample, _musicVolume);
			}
			break;
		case kMixerQueueType_XA:
			for (int i = 0; mql && i < len; i += 2) {
				int pos = _queue->xaOffset >> kFracBits;
				if (pos >= _queue->xaDecoder._samplesSize) {
					const int count = _queue->xaDecoder.decode(mql->buffer + mql->read, mql->size - mql->read);
					mql->read += count;
					if (mql->read >= mql->size) {
						MixerQueueList *next = mql->next;
						free(mql->buffer);
						delete mql;
						mql = next;
						_queue->head = mql;
					}
					_queue->xaOffset = pos = 0;
				}
				assert(pos >= 0 && pos + 1 < _queue->xaDecoder._samplesSize);
				const int j = pos + 2;
				if (j == 0 || j >= _queue->xaDecoder._samplesSize) {
					mix(&buf[i + 0], _queue->xaDecoder._samples[pos + 0], _musicVolume);
					mix(&buf[i + 1], _queue->xaDecoder._samples[pos + 1], _musicVolume);
				} else {
					mix(&buf[i + 0], lerpS16(_queue->xaDecoder._samples[pos + 0], _queue->xaDecoder._samples[j + 0], _queue->xaOffset), _musicVolume);
					mix(&buf[i + 1], lerpS16(_queue->xaDecoder._samples[pos + 1], _queue->xaDecoder._samples[j + 1], _queue->xaOffset), _musicVolume);
				}
				_queue->xaOffset += _queue->xaStep;
			}
			break;
		}
	}
	for (int i = 0; i < kMaxSoundsCount; ++i) {
		if (_soundsTable[i]) {
			if (!_soundsTable[i]->readSamples(buf, len)) {
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
