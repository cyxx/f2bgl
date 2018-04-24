/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#ifndef RENDER_H__
#define RENDER_H__

#include "util.h"

enum {
	kFlatColorRed = 512,
	kFlatColorGreen,
	kFlatColorYellow,
	kFlatColorBlue,
	kFlatColorShadow,
	kFlatColorLight,
	kFlatColorLight9
};

enum {
	kProjGame = 0,
	kProjMenu,
	kProjInstall,
	kProj2D
};

struct Texture;

struct RenderParams {
	bool fog;
	bool gouraud; // enable lighting
	const char *textureFilter;
	const char *textureScaler;
};

struct Render {
	uint8_t _clut[256 * 3];
	int _w, _h;
	float _aspectRatio;
	uint8_t *_screenshotBuf;
	struct {
		bool rgbTex;
		Texture *tex;
		int x, y, w, h;
		int displayWidth, displayHeight;
		int r, g, b;
	} _overlay;
	struct {
		bool changed;
		int wScale, hScale;
		int x, y, w, h;
	} _viewport;
	bool _paletteGreyScale;
	int _paletteRgbScale;
	bool _fog;
	bool _lighting;
	bool _drawObjectIgnoreDepth;
	int _framesCount;
	int _framesPerSec;

	Render(const RenderParams *params);
	~Render();

	void flushCachedTextures();

	void toggleFog() { _fog = !_fog; }
	void toggleGouraudShading() { _lighting = !_lighting; }

	void setCameraPos(int x, int y, int z, int shift = 0);
	void setCameraPitch(int a);

	bool hasTexture(int16_t key);
	void prepareTextureLut(const uint8_t *data, int w, int h, const uint8_t *clut, int16_t texKey);
	void prepareTextureRgb(const uint8_t *data, int w, int h, int16_t texKey);
	void releaseTexture(int16_t texKey);

	void drawPolygonFlat(const Vertex *vertices, int verticesCount, int color);
	void drawPolygonTexture(const Vertex *vertices, int verticesCount, int primitive, const uint8_t *texData, int texW, int texH, int16_t texKey);
	void drawParticle(const Vertex *pos, int color);
	void drawSprite(int x, int y, const uint8_t *texData, int texW, int texH, int primitive, int16_t texKey, uint8_t transparentScale = 255);
	void drawRectangle(int x, int y, int w, int h, int color);

	void setIgnoreDepth(bool ignoreDepth);
	void beginObjectDraw(int x, int y, int z, int ry, int shift = 0);
	void endObjectDraw();

	void setOverlayBlendColor(int r, int g, int b);
	void resizeOverlay(int w, int h, bool rgb = false, int displayWidth = 0, int displayHeight = 0);
	void copyToOverlay(int x, int y, int w, int h, const uint8_t *data, bool rgb = false, const uint8_t *clut = 0);

	void setPaletteScale(bool greyScale, int rgbScale);
	void setPalette(const uint8_t *pal, int offset, int count);
	void clearScreen();
	void setupProjection(int mode);
	void drawOverlay();
	void resizeScreen(int w, int h, float *p);

	const uint8_t *captureScreen(int *w, int *h);
};

#endif // RENDER_H__
