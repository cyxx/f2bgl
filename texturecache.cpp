/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#ifdef USE_GLES
#include <GLES/gl.h>
#else
#include <SDL_opengl.h>
#endif
#include "scaler.h"
#include "texturecache.h"

static const int kLutTextureBufferSize = 320 * 200;

static uint16_t convert_RGBA_5551(int r, int g, int b) {
	return ((r >> 3) << 11) | ((g >> 3) << 6) | ((b >> 3) << 1) | 1;
}

static const struct {
	int internal;
	int format;
	int type;
	uint16_t (*convertColor)(int, int, int);
} _formats[] = {
#ifdef USE_GLES
	{ GL_RGBA, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, &convert_RGBA_5551 },
#else
	{ GL_RGB5_A1, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, &convert_RGBA_5551 },
#endif
	{ -1, -1, -1, 0 }
};

static const struct {
	const char *name;
	int filter;
} _filters[] = {
	{ "nearest", GL_NEAREST },
	{ "linear", GL_LINEAR },
	{ 0, 0 },
};

static const struct {
	void (*proc)(uint16_t *dst, int dstPitch, const uint16_t *src, int srcPitch, int w, int h);
	const char *name;
	int factor;
} _scalers[] = {
	{ point1x, "point1x", 1 },
	{ point2x, "point2x", 2 },
	{ scale2x, "scale2x", 2 },
	{ point3x, "point3x", 3 },
	{ scale3x, "scale3x", 3 },
	{ 0, 0, 0 },
};

static const int _fmt = 0;

TextureCache::TextureCache()
	: _texturesListHead(0), _texturesListTail(0) {
	memset(_clut, 0, sizeof(_clut));
	_texBuf = 0;
	_npotTex = false;
	_filter = GL_LINEAR;
	_scaler = 2;
}

TextureCache::~TextureCache() {
	free(_texBuf);
	flush();
}

static bool hasExt(const char *exts, const char *name) {
	const char *p = strstr(exts, name);
	if (p) {
		p += strlen(name);
		return *p == ' ' || *p == 0;
	}
	return false;
}

void TextureCache::init(const char *filter, const char *scaler) {
	const char *exts = (const char *)glGetString(GL_EXTENSIONS);
	if (exts && hasExt(exts, "GL_ARB_texture_non_power_of_two")) {
		_npotTex = true;
	}
	if (filter) {
		for (int i = 0; _filters[i].name; ++i) {
			if (strcasecmp(_filters[i].name, filter) == 0) {
				_filter = _filters[i].filter;
				break;
			}
		}
	}
	if (scaler) {
		for (int i = 0; _scalers[i].name; ++i) {
			if (strcasecmp(_scalers[i].name, scaler) == 0) {
				_scaler = i;
				break;
			}
		}
	}
	if (_scalers[_scaler].factor != 1) {
		_texBuf = (uint16_t *)malloc(kLutTextureBufferSize * sizeof(uint16_t));
	}
}

void TextureCache::flush() {
	Texture *t = _texturesListHead;
	while (t) {
		Texture *next = t->next;
		glDeleteTextures(1, &t->id);
		free(t->bitmapData);
		delete t;
		t = next;
	}
	_texturesListHead = _texturesListTail = 0;
	memset(_clut, 0, sizeof(_clut));
}

bool TextureCache::hasTexture(int16_t key) const {
	for (Texture *t = _texturesListHead; t; t = t->next) {
		if (t->key == key) {
			return true;
		}
	}
	return false;
}

void TextureCache::releaseTexture(int16_t key) {
	for (Texture *t = _texturesListHead; t; t = t->next) {
		if (t->key == key) {
			destroyTexture(t);
			break;
		}
	}
}

Texture *TextureCache::getCachedTexture(int16_t key, const uint8_t *data, int w, int h, bool rgb, const uint8_t *pal) {
	Texture *prev = 0;
	for (Texture *t = _texturesListHead; t; t = t->next) {
		if (t->key == key) {
			if (prev) { // move to head
				prev->next = t->next;
				t->next = _texturesListHead;
				_texturesListHead = t;
				if (t == _texturesListTail) {
					_texturesListTail = prev;
				}
			}
			return t;
		}
		prev = t;
	}
	assert(data && w > 0 && h > 0);
	Texture *t = createTexture(data, w, h, rgb, pal);
	if (t) {
		t->key = key;
	}
	return t;
}

static int roundPow2(int sz) {
	if (sz != 0 && (sz & (sz - 1)) == 0) {
		return sz;
	}
	int textureSize = 1;
	while (textureSize < sz) {
		textureSize <<= 1;
	}
	return textureSize;
}

void TextureCache::convertTexture(const uint8_t *src, int w, int h, const uint16_t *clut, uint16_t *dst, int dstPitch) {
	if (_scalers[_scaler].factor == 1) {
		for (int y = 0; y < h; ++y) {
			for (int x = 0; x < w; ++x) {
				dst[x] = clut[*src++];
			}
			dst += dstPitch;
		}
	} else {
		assert(w * h <= kLutTextureBufferSize);
		int offset = 0;
		for (int y = 0; y < h; ++y) {
			for (int x = 0; x < w; ++x) {
				_texBuf[offset + x] = clut[*src++];
			}
			offset += w;
		}
		_scalers[_scaler].proc(dst, dstPitch, _texBuf, w, w, h);
	}
}

Texture *TextureCache::createTexture(const uint8_t *data, int w, int h, bool rgb, const uint8_t *pal) {
	Texture *t = new Texture;
	t->bitmapW = w;
	t->bitmapH = h;
	if (rgb) {
		// bitmap is true color, we don't need to keep a copy for palette changes
		t->bitmapData = 0;
	} else {
		t->bitmapData = (uint8_t *)malloc(w * h);
		if (!t->bitmapData) {
			delete t;
			return 0;
		}
		memcpy(t->bitmapData, data, w * h);
		w *= _scalers[_scaler].factor;
		h *= _scalers[_scaler].factor;
	}
	t->texW = _npotTex ? w : roundPow2(w);
	t->texH = _npotTex ? h : roundPow2(h);
	t->u = w / (float)t->texW;
	t->v = h / (float)t->texH;
	glGenTextures(1, &t->id);
	uint16_t *texData = (uint16_t *)calloc(t->texW * t->texH, sizeof(uint16_t));
	if (texData) {
		if (rgb) {
			uint16_t *p = texData;
			for (int y = 0; y < h; ++y) {
				for (int x = 0; x < w; ++x) {
					p[x] = _formats[_fmt].convertColor(data[0], data[1], data[2]); data += 4;
				}
				p += t->texW;
			}
		} else if (pal) {
			uint16_t clut[256];
			convertPalette(pal, clut);
			convertTexture(t->bitmapData, t->bitmapW, t->bitmapH, clut, texData, t->texW);
		} else {
			convertTexture(t->bitmapData, t->bitmapW, t->bitmapH, _clut, texData, t->texW);
		}
		glBindTexture(GL_TEXTURE_2D, t->id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _filter);
		if (_filter == GL_LINEAR) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, _formats[_fmt].internal, t->texW, t->texH, 0, _formats[_fmt].format, _formats[_fmt].type, texData);
		free(texData);
	}
	if (!_texturesListHead) {
		_texturesListHead = _texturesListTail = t;
	} else {
		_texturesListTail->next = t;
		_texturesListTail = t;
	}
	t->next = 0;
	t->key = -1;
	return t;
}

void TextureCache::destroyTexture(Texture *texture) {
	glDeleteTextures(1, &texture->id);
	free(texture->bitmapData);
	if (texture == _texturesListHead) {
		_texturesListHead = texture->next;
		if (texture == _texturesListTail) {
			_texturesListTail = _texturesListHead;
		}
	} else {
		for (Texture *t = _texturesListHead; t; t = t->next) {
			if (t->next == texture) {
				t->next = texture->next;
				if (texture == _texturesListTail) {
					_texturesListTail = t;
				}
				break;
			}
		}
	}
	delete texture;
}

void TextureCache::updateTexture(Texture *t, const uint8_t *data, int w, int h, bool rgb, const uint8_t *pal) {
	assert(t->bitmapW == w && t->bitmapH == h);
	if (rgb) {
		assert(t->bitmapData == 0);
	} else {
		memcpy(t->bitmapData, data, w * h);
	}
	uint16_t *texData = (uint16_t *)calloc(t->texW * t->texH, sizeof(uint16_t));
	if (texData) {
		if (rgb) {
			uint16_t *p = texData;
			for (int y = 0; y < h; ++y) {
				for (int x = 0; x < w; ++x) {
					p[x] = _formats[_fmt].convertColor(data[0], data[1], data[2]); data += 4;
				}
				p += t->texW;
			}
		} else {
			assert(pal);
			uint16_t clut[256];
			convertPalette(pal, clut);
			convertTexture(t->bitmapData, t->bitmapW, t->bitmapH, clut, texData, t->texW);
		}
		glBindTexture(GL_TEXTURE_2D, t->id);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, t->texW, t->texH, _formats[_fmt].format, _formats[_fmt].type, texData);
		free(texData);
	}
}

void TextureCache::convertPalette(const uint8_t *src, uint16_t *dst) {
	const int r = *src++;
	const int g = *src++;
	const int b = *src++;
	if (r != 0 || g != 0 || b != 0) {
		warning("Unexpected palette color for index 0, r %d g %d b %d", r, g, b);
		dst[0] = _formats[_fmt].convertColor(r, g, b);
	} else {
		// first palette entry is used for transparency
		dst[0] = 0;
	}
	for (int i = 1; i < 256; ++i, src += 3) {
		dst[i] = _formats[_fmt].convertColor(src[0], src[1], src[2]);
	}
}

void TextureCache::setPalette(const uint8_t *pal, bool updateTextures) {
	convertPalette(pal, _clut);
	if (updateTextures) {
		for (Texture *t = _texturesListHead; t; t = t->next) {
			if (!t->bitmapData) {
				// skip rgb textures
				continue;
			}
			uint16_t *texData = (uint16_t *)calloc(t->texW * t->texH, sizeof(uint16_t));
			if (texData) {
				convertTexture(t->bitmapData, t->bitmapW, t->bitmapH, _clut, texData, t->texW);
				glBindTexture(GL_TEXTURE_2D, t->id);
				glTexImage2D(GL_TEXTURE_2D, 0, _formats[_fmt].internal, t->texW, t->texH, 0, _formats[_fmt].format, _formats[_fmt].type, texData);
				free(texData);
			}
		}
	}
}
