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

	void init(const char *filter, const char *scaler);
	void flush();

	bool hasTexture(int16_t key) const;
	void releaseTexture(int16_t key);
	Texture *getCachedTexture(int16_t key, const uint8_t *data, int w, int h, bool rgb = false, const uint8_t *pal = 0);
	void convertTexture(const uint8_t *src, int w, int h, const uint16_t *clut, uint16_t *dst, int dstPitch);
	Texture *createTexture(const uint8_t *data, int w, int h, bool rgb = false, const uint8_t *pal = 0);
	void destroyTexture(Texture *);
	void updateTexture(Texture *, const uint8_t *data, int w, int h, bool rgb = false, const uint8_t *pal = 0);

	void convertPalette(const uint8_t *src, uint16_t *dst);
	void setPalette(const uint8_t *pal, bool updateTextures = true);

	int _filter;
	int _scaler;
	Texture *_texturesListHead, *_texturesListTail;
	uint16_t _clut[256];
	uint16_t *_texBuf;
	bool _npotTex;
};

#endif // TEXTURECACHE_H__
