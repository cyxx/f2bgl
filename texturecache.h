/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#ifndef TEXTURECACHE_H__
#define TEXTURECACHE_H__

#include "util.h"

struct Texture {
	GLuint id;
	int bitmapW, bitmapH;
	uint8_t *bitmapData;
	int texW, texH;
	float u, v;
	Texture *next;
	int16_t key;
};

struct TextureCache {

	TextureCache();
	~TextureCache();

	void init();
	void flush();

	Texture *getCachedTexture(const uint8_t *data, int w, int h, int16_t key);
	void convertTexture(const uint8_t *src, int w, int h, const uint16_t *clut, uint16_t *dst, int dstPitch);
	Texture *createTexture(const uint8_t *data, int w, int h);
	void destroyTexture(Texture *);
	void updateTexture(Texture *, const uint8_t *data, int w, int h);

	void setPalette(const uint8_t *pal, bool updateTextures = true);

	int _fmt;
	Texture *_texturesListHead, *_texturesListTail;
	uint16_t _clut[256];
	uint16_t *_texBuf;
	bool _npotTex;
};

#endif // TEXTURECACHE_H__
