/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include "decoder.h"
#include "spritecache.h"

SpriteCache::SpriteCache() {
	memset(_entries, 0, sizeof(_entries));
}

SpriteCache::~SpriteCache() {
	flush();
}

void SpriteCache::flush() {
	for (int i = 0; i < ARRAYSIZE(_entries); ++i) {
		free(_entries[i].data);
	}
	memset(_entries, 0, sizeof(_entries));
}

uint8_t *SpriteCache::getData(int16_t key, const uint8_t *src) {
	assert(key >= 0 && key < ARRAYSIZE(_entries));
	if (_entries[key].data) {
		if (src == _entries[key].src) {
			return _entries[key].data;
		}
		warning("Invalid cache entry for key %d", key);
		free(_entries[key].data);
		_entries[key].data = 0;
	}
	const int size = READ_LE_UINT16(src); src += 2;
	const int packedSize = READ_LE_UINT16(src); src += 2;
	uint8_t *dst = (uint8_t *)malloc(size);
	if (dst) {
		if (size > packedSize) {
			decodeLZSS(src, dst, size);
		} else {
			memcpy(dst, src, size);
		}
		_entries[key].src = src - 4;
		_entries[key].data = dst;
	}
	return dst;
}
