/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#ifndef SPRITECACHE_H__
#define SPRITECACHE_H__

struct SpriteCache {
	struct {
		const uint8_t *src;
		uint8_t *data;
	} _entries[3072];

	SpriteCache();
	~SpriteCache();

	void flush();

	uint8_t *getData(int16_t key, const uint8_t *src);
};

#endif // SPRITECACHE_H__
