/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#ifdef USE_GLES
#include <GLES/gl.h>
#else
#include <SDL_opengl.h>
#endif
#include <math.h>
#include <sys/time.h>
#include "render.h"
#include "texturecache.h"

static const int kOverlayWidth = 320;
static const int kOverlayHeight = 200;

static const GLfloat _fogColor[4] = { .1, .1, .1, 1. };

static const GLfloat _lightPosition[4] = { 0., .3, 0., 0. };
static const GLfloat _lightAmbient[4]  = { .4, .4, .4, 1. };
static const GLfloat _lightDiffuse[4]  = { 1., 1., 1., 1. };
static const GLfloat _lightSpecular[4] = { 1., 1., 1., 1. };

struct Vertex3f {
	GLfloat x, y, z;
};

#ifdef USE_GLES

#define glOrtho glOrthof
#define glFrustum glFrustumf
#define glFogi glFogf

static const int kVerticesBufferSize = 1024;
static GLfloat _verticesBuffer[kVerticesBufferSize * 3];

static GLfloat *bufferVertex(const Vertex *vertices, int count) {
	assert(count <= kVerticesBufferSize);
	GLfloat *buf = _verticesBuffer;
	for (int i = 0; i < count; ++i) {
		buf[0] = vertices[i].x;
		buf[1] = vertices[i].y;
		buf[2] = vertices[i].z;
		buf += 3;
	}
	return _verticesBuffer;
}
#endif

static void emitQuad2i(int x, int y, int w, int h) {
#ifdef USE_GLES
	GLfloat vertices[] = { x, y, x + w, y, x + w, y + h, x, y + h };
	glVertexPointer(2, GL_FLOAT, 0, vertices);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
#else
	glBegin(GL_QUADS);
		glVertex2i(x, y);
		glVertex2i(x + w, y);
		glVertex2i(x + w, y + h);
		glVertex2i(x, y + h);
	glEnd();
#endif
}

static void emitQuadTex2i(int x, int y, int w, int h, GLfloat *uv) {
#ifdef USE_GLES
	GLfloat vertices[] = { x, y, x + w, y, x + w, y + h, x, y + h };
	glVertexPointer(2, GL_FLOAT, 0, vertices);
	glTexCoordPointer(2, GL_FLOAT, 0, uv);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
#else
	glBegin(GL_QUADS);
		glTexCoord2f(uv[0], uv[1]);
		glVertex2i(x, y);
		glTexCoord2f(uv[2], uv[3]);
		glVertex2i(x + w, y);
		glTexCoord2f(uv[4], uv[5]);
		glVertex2i(x + w, y + h);
		glTexCoord2f(uv[6], uv[7]);
		glVertex2i(x, y + h);
	glEnd();
#endif
}

static void emitQuadTex3i(const Vertex *vertices, GLfloat *uv) {
#ifdef USE_GLES
	glVertexPointer(3, GL_FLOAT, 0, bufferVertex(vertices, 4));
	glTexCoordPointer(2, GL_FLOAT, 0, uv);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
#else
	glBegin(GL_QUADS);
		glTexCoord2f(uv[0], uv[1]);
		glVertex3i(vertices[0].x, vertices[0].y, vertices[0].z);
		glTexCoord2f(uv[2], uv[3]);
		glVertex3i(vertices[1].x, vertices[1].y, vertices[1].z);
		glTexCoord2f(uv[4], uv[5]);
		glVertex3i(vertices[2].x, vertices[2].y, vertices[2].z);
		glTexCoord2f(uv[6], uv[7]);
		glVertex3i(vertices[3].x, vertices[3].y, vertices[3].z);
	glEnd();
#endif
}

static void emitTriTex3i(const Vertex *vertices, const GLfloat *uv) {
#ifdef USE_GLES
	glVertexPointer(3, GL_FLOAT, 0, bufferVertex(vertices, 3));
	glTexCoordPointer(2, GL_FLOAT, 0, uv);
	glDrawArrays(GL_TRIANGLES, 0, 3);
#else
	glBegin(GL_TRIANGLES);
		glTexCoord2f(uv[0], uv[1]);
		glVertex3i(vertices[0].x, vertices[0].y, vertices[0].z);
		glTexCoord2f(uv[2], uv[3]);
		glVertex3i(vertices[1].x, vertices[1].y, vertices[1].z);
		glTexCoord2f(uv[4], uv[5]);
		glVertex3i(vertices[2].x, vertices[2].y, vertices[2].z);
	glEnd();
#endif
}

static void emitTriFan3i(const Vertex *vertices, int count) {
#ifdef USE_GLES
	glVertexPointer(3, GL_FLOAT, 0, bufferVertex(vertices, count));
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
#else
	glBegin(GL_TRIANGLE_FAN);
		for (int i = 0; i < count; ++i) {
			glNormal3i(vertices[i].nx, vertices[i].ny, vertices[i].nz);
			glVertex3i(vertices[i].x, vertices[i].y, vertices[i].z);
		}
        glEnd();
#endif
}

static void emitPoint3i(const Vertex *pos) {
#ifdef USE_GLES
	glVertexPointer(3, GL_FLOAT, 0, bufferVertex(pos, 1));
	glDrawArrays(GL_POINTS, 0, 1);
#else
	glBegin(GL_POINTS);
		glVertex3i(pos->x, pos->y, pos->z);
	glEnd();
#endif
}

static TextureCache _textureCache;
static Vertex3f _cameraPos;
static GLfloat _cameraPitch;
struct timeval _frameTimeStamp;

Render::Render(const RenderParams *params) {
	memset(_clut, 0, sizeof(_clut));
	_aspectRatio = 1.;
	_fov = 0;
	_screenshotBuf = 0;
	memset(&_overlay, 0, sizeof(_overlay));
	_overlay.r = _overlay.g = _overlay.b = 255;
	_viewport.changed = true;
	_viewport.wScale = 256;
	_viewport.hScale = 256;
	_textureCache.init(params->textureFilter, params->textureScaler);
	_paletteGreyScale = false;
	_paletteRgbScale = 256;
	_fog = params->fog;
	_lighting = params->gouraud;
	_drawObjectIgnoreDepth = false;
	gettimeofday(&_frameTimeStamp, 0);
	_framesCount = 0;
	_framesPerSec = 0;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_NOTEQUAL, 0.);
	glEnable(GL_NORMALIZE);
	if (_fog) {
		glEnable(GL_FOG);
		glFogi(GL_FOG_MODE, GL_LINEAR);
		glFogfv(GL_FOG_COLOR, _fogColor);
		glFogf(GL_FOG_DENSITY, .35);
		glFogf(GL_FOG_START, 16.);
		glFogf(GL_FOG_END, 256.);
	}
	if (_lighting) {
		glEnable(GL_LIGHT0);
		glLightfv(GL_LIGHT0, GL_POSITION, _lightPosition);
		glLightfv(GL_LIGHT0, GL_AMBIENT,  _lightAmbient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE,  _lightDiffuse);
		glLightfv(GL_LIGHT0, GL_SPECULAR, _lightSpecular);
	}
	glShadeModel(GL_SMOOTH);
}

Render::~Render() {
	free(_screenshotBuf);
}

void Render::flushCachedTextures() {
	_textureCache.flush();
	_overlay.tex = 0;
}

void Render::resizeScreen(int w, int h, float *p, int fov) {
	_w = w;
	_h = h;
	_aspectRatio = p[2] / p[3];
	_fov = fov / 360.;
	_viewport.x = 0;
	_viewport.y = 0;
	_viewport.w = w;
	_viewport.h = h;
	_viewport.changed = true;
	free(_screenshotBuf);
	_screenshotBuf = 0;
}

void Render::setCameraPos(int x, int y, int z, int shift) {
	const GLfloat div = 1 << shift;
	_cameraPos.x = x / div;
	_cameraPos.z = z / div;
	_cameraPos.y = y / div;
}

void Render::setCameraPitch(int ry) {
	_cameraPitch = ry * 360 / 1024.;
}

bool Render::hasTexture(int16_t key) {
	return _textureCache.hasTexture(key);
}

void Render::prepareTextureLut(const uint8_t *data, int w, int h, const uint8_t *clut, int16_t texKey) {
	_textureCache.getCachedTexture(texKey, data, w, h, false, clut);
}

void Render::prepareTextureRgb(const uint8_t *data, int w, int h, int16_t texKey) {
	_textureCache.getCachedTexture(texKey, data, w, h, true);
}

void Render::releaseTexture(int16_t texKey) {
	_textureCache.releaseTexture(texKey);
}

void Render::drawPolygonFlat(const Vertex *vertices, int verticesCount, int color) {
	bool lightFlatColor = false;
	switch (color) {
	case kFlatColorRed:
		glColor4ub(255, 0, 0, 127);
		break;
	case kFlatColorGreen:
		glColor4ub(0, 255, 0, 127);
		break;
	case kFlatColorYellow:
		glColor4ub(255, 255, 0, 127);
		break;
	case kFlatColorBlue:
		glColor4ub(0, 0, 255, 127);
		break;
	case kFlatColorShadow:
		glColor4ub(0, 0, 0, 63);
		break;
	case kFlatColorLight:
		glColor4ub(255, 255, 255, 63);
		break;
	case kFlatColorLight9:
		glColor4ub(255, 255, 255, 127);
		break;
	default:
		if (color >= 0 && color < 256) {
			if (_lighting) {
				glEnable(GL_LIGHTING);
				glEnable(GL_COLOR_MATERIAL);
				glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
				lightFlatColor = true;
			}
			glColor4ub(_clut[color * 3], _clut[color * 3 + 1], _clut[color * 3 + 2], color == 0 ? 0 : 255);
		} else {
			warning("Render::drawPolygonFlat() unhandled color %d", color);
		}
		break;
	}
	emitTriFan3i(vertices, verticesCount);
	if (_lighting && lightFlatColor) {
		glDisable(GL_COLOR_MATERIAL);
		glDisable(GL_LIGHTING);
	}
}

void Render::drawPolygonTexture(const Vertex *vertices, int verticesCount, int primitive, const uint8_t *texData, int texW, int texH, int16_t texKey) {
	assert(vertices && verticesCount >= 4);
	glColor4ub(255, 255, 255, 255);
	glEnable(GL_TEXTURE_2D);
	Texture *t = _textureCache.getCachedTexture(texKey, texData, texW, texH);
	glBindTexture(GL_TEXTURE_2D, t->id);
	const GLfloat tx = t->u;
	const GLfloat ty = t->v;
	switch (primitive) {
	case 0:
	case 2:
		//
		// 1:::2
		// :   :
		// 4:::3
		//
		{
			GLfloat uv[] = { 0., 0., tx, 0., tx, ty, 0., ty };
			emitQuadTex3i(vertices, uv);
		}
		break;
	case 1:
		//
		//   1
		//  : :
		// 3:::2
		//
		{
			GLfloat uv[] = { tx / 2, 0., tx, ty, 0., ty };
			emitTriTex3i(vertices, uv);
		}
		break;
	case 3:
	case 5:
		//
		// 4:::1
		// :   :
		// 3:::2
		//
		{
			GLfloat uv[] = { tx, 0., tx, ty, 0., ty, 0., 0. };
			emitQuadTex3i(vertices, uv);
		}
		break;
	case 4:
		//
		//   3
		//  : :
		// 2:::1
		//
		{
			GLfloat uv[] = { tx, ty, 0., ty, tx / 2, 0. };
			emitTriTex3i(vertices, uv);
		}
		break;
	case 6:
	case 8:
		//
		// 3:::4
		// :   :
		// 2:::1
		//
		{
			GLfloat uv[] = { tx, ty, 0., ty, 0., 0., tx, 0. };
			emitQuadTex3i(vertices, uv);
		}
		break;
	case 7:
		//
		//   2
		//  : :
		// 1:::3
		//
		{
			GLfloat uv[] = { .0, ty, tx / 2, 0., tx, ty };
			emitTriTex3i(vertices, uv);
		}
		break;
	case 9:
	case 10:
		//
		// 2:::3
		// :   :
		// 1:::4
		//
		{
			GLfloat uv[] = { 0., 0., 0., ty, tx, ty, tx, 0. };
			emitQuadTex3i(vertices, uv);
		}
		break;
	default:
		warning("Render::drawPolygonTexture() unhandled primitive %d", primitive);
		break;
	}
	glDisable(GL_TEXTURE_2D);
}

void Render::drawParticle(const Vertex *pos, int color) {
	switch (color) {
	case kFlatColorRed:
		glColor4ub(255, 0, 0, 127);
		break;
	case kFlatColorGreen:
		glColor4ub(0, 255, 0, 127);
		break;
	case kFlatColorYellow:
		glColor4ub(255, 255, 0, 127);
		break;
	case kFlatColorBlue:
		glColor4ub(0, 0, 255, 127);
		break;
	case kFlatColorShadow:
		glColor4ub(0, 0, 0, 127);
		break;
	case kFlatColorLight:
		glColor4ub(255, 255, 255, 63);
		break;
	case kFlatColorLight9:
		glColor4ub(255, 255, 255, 127);
		break;
	default:
		if (color >= 0 && color < 256) {
			glColor4ub(_clut[color * 3], _clut[color * 3 + 1], _clut[color * 3 + 2], color == 0 ? 0 : 255);
		} else {
			warning("Render::drawParticle() unhandled color %d", color);
		}
	}
	glPointSize(4.);
	emitPoint3i(pos);
	glPointSize(1.);
}

void Render::drawSprite(int x, int y, const uint8_t *texData, int texW, int texH, int primitive, int16_t texKey, uint8_t transparentScale) {
	glColor4ub(255, 255, 255, transparentScale);
	glEnable(GL_TEXTURE_2D);
	Texture *t = _textureCache.getCachedTexture(texKey, texData, texW, texH);
	glBindTexture(GL_TEXTURE_2D, t->id);
	switch (primitive) {
	case 0:
		// 1:::2
		// :   :
		// 4:::3
		{
			GLfloat uv[] = { 0., 0., t->u, 0., t->u, t->v, 0., t->v };
			emitQuadTex2i(x, y, texW, texH, uv);
		}
		break;
	case 9:
		//
		// 2:::3
		// :   :
		// 1:::4
		//
		{
			GLfloat uv[] = { 0., 0., 0., t->v, t->u, t->v, t->u, 0. };
			emitQuadTex2i(x, y, texW, texH, uv);
		}
		break;
	default:
		warning("Render::drawSprite() unhandled primitive %d", primitive);
		break;
	}
	glDisable(GL_TEXTURE_2D);
}

void Render::drawRectangle(int x, int y, int w, int h, int color) {
	assert(color >= 0 && color < 256);
	glColor4ub(_clut[color * 3], _clut[color * 3 + 1], _clut[color * 3 + 2], color == 0 ? 0 : 255);
	emitQuad2i(x, y, w, h);
}

void Render::copyToOverlay(int x, int y, int w, int h, const uint8_t *data, bool rgb, const uint8_t *pal) {
	_overlay.x = x;
	_overlay.y = y;
	_overlay.w = w;
	_overlay.h = h;
	if (!_overlay.tex) {
		_overlay.rgbTex = rgb;
		_overlay.tex = _textureCache.createTexture(data, w, h, rgb, pal);
	} else {
		_textureCache.updateTexture(_overlay.tex, data, w, h, rgb, pal);
	}
}

void Render::setIgnoreDepth(bool ignoreDepth) {
	if (_drawObjectIgnoreDepth != ignoreDepth) {
		if (ignoreDepth) {
			glDisable(GL_DEPTH_TEST);
		} else {
			glEnable(GL_DEPTH_TEST);
		}
		_drawObjectIgnoreDepth = ignoreDepth;
	}
}

void Render::beginObjectDraw(int x, int y, int z, int ry, int shift) {
	glPushMatrix();
	const GLfloat div = 1 << shift;
	glTranslatef(x / div, y / div, z / div);
	glRotatef(ry * 360 / 1024., 0., 1., 0.);
	glScalef(1 / 8., 1 / 2., 1 / 8.);
}

void Render::endObjectDraw() {
	glPopMatrix();
}

void Render::setOverlayBlendColor(int r, int g, int b) {
	_overlay.r = r;
	_overlay.g = g;
	_overlay.b = b;
}

void Render::resizeOverlay(int w, int h, bool rgb, int displayWidth, int displayHeight) {
	if (w != _overlay.w || h != _overlay.h || rgb != _overlay.rgbTex) {
		if (_overlay.tex) {
			_textureCache.destroyTexture(_overlay.tex);
			_overlay.tex = 0;
		}
	}
	_overlay.displayWidth  = displayWidth  == 0 ? w : displayWidth;
	_overlay.displayHeight = displayHeight == 0 ? h : displayHeight;
}

void Render::setPaletteScale(bool greyScale, int rgbScale) {
	_paletteGreyScale = greyScale;
	_paletteRgbScale = rgbScale;
}

void Render::setPalette(const uint8_t *pal, int offset, int count) {
	int color = 3 * offset;
	for (int i = 0; i < count; ++i) {
		int r = pal[0];
		int g = pal[1];
		int b = pal[2];
		pal += 3;
		if (_paletteGreyScale) {
			const int grey = (r * 30 + g * 59 + b * 11) / 100;
			r = g = b = grey;
		}
		if (_paletteRgbScale != 256) {
			r = CLIP((r * _paletteRgbScale) >> 8, 0, 255);
			g = CLIP((g * _paletteRgbScale) >> 8, 0, 255);
			b = CLIP((b * _paletteRgbScale) >> 8, 0, 255);
		}
		_clut[color + 0] = r;
		_clut[color + 1] = g;
		_clut[color + 2] = b;
		color += 3;
	}
	_textureCache.setPalette(_clut);
}

void Render::clearScreen() {
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#ifdef USE_GLES
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
#endif
	if (_viewport.changed) {
		_viewport.changed = false;
		const int vw = _viewport.w * _viewport.wScale >> 8;
		const int vh = _viewport.h * _viewport.hScale >> 8;
		const int vx = _viewport.x + (_viewport.w - vw) / 2;
		const int vy = _viewport.y + (_viewport.h - vh) / 2;
		glViewport(vx, vy, vw, vh);
	}
}

void Render::setupProjection(int mode) {
	const GLfloat aspect = 1.5 * _aspectRatio;

	switch (mode) {
	case kProj2D:
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-1. / _aspectRatio, 1. / _aspectRatio, kOverlayHeight, 0, 0, 1);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(-1., 0., 0.);
		glScalef(2. / kOverlayWidth, 1., 1.);

		if (_fog) {
			glDisable(GL_FOG);
		}
		break;

	case kProjMenu:
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glFrustum(-.5, .5, -aspect / 2, 0., 1., 512.);
		glTranslatef(0., 0., -20.);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		if (_lighting) {
			glEnable(GL_LIGHT0);
		} else {
			glDisable(GL_LIGHT0);
		}
		glScalef(1., -.5, 1.);
		glTranslatef(0., 14. * _aspectRatio, -72.);

		if (_fog) {
			glDisable(GL_FOG);
		}
		break;

	case kProjInstall:
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glFrustum(-.5, .5, -aspect / 2, 0., 1., 4096.);
		glTranslatef(0., 0., -64.);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		if (_lighting) {
			glEnable(GL_LIGHT0);
		} else {
			glDisable(GL_LIGHT0);
		}
		glTranslatef(0, -1024., -3092.);

		if (_fog) {
			glDisable(GL_FOG);
		}
		break;

	case kProjGame:
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		if (_fov != 0.) {
			const float h = -tan(_fov * .5) * 7.5;
			const float w = aspect * h / 2;
			glFrustum(w, -w, h, 0, 1., 1024);
		} else {
			glFrustum(-.5, .5, -aspect / 2, 0., 1., 1024);
		}
		glTranslatef(0., 0., -16.);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		if (_lighting) {
			glEnable(GL_LIGHT0);
		} else {
			glDisable(GL_LIGHT0);
		}
		glScalef(1., -.5, -1.);
		glRotatef(_cameraPitch, 0., 1., 0.);
		glTranslatef(-_cameraPos.x, _cameraPos.y, -_cameraPos.z);

		if (_fog) {
			glEnable(GL_FOG);
		} else {
			glDisable(GL_FOG);
		}
		break;
	}
}

void Render::drawOverlay() {

	const bool hasOverlayTexture = (_overlay.tex != 0);
	const bool hasOverlayColor = (_overlay.r != 255 || _overlay.g != 255 || _overlay.b != 255);

	if (!hasOverlayTexture && !hasOverlayColor) {
		return;
	}

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1. / _aspectRatio, 1. / _aspectRatio, 0, _overlay.displayHeight, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if (hasOverlayTexture) {
		glColor4ub(255, 255, 255, 255);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, _overlay.tex->id);
		const GLfloat tU = _overlay.tex->u;
		const GLfloat tV = _overlay.tex->v;
		assert(tU != 0. && tV != 0.);
		GLfloat uv[] = { 0., 0., tU, 0., tU, tV, 0., tV };
		emitQuadTex2i(-1, _overlay.y, 2, _overlay.h, uv);
		glDisable(GL_TEXTURE_2D);
	}
	if (hasOverlayColor) {
		glColor4ub(_overlay.r, _overlay.g, _overlay.b, 191);
		emitQuad2i(-1, 0, 2, _overlay.displayHeight);
		_overlay.r = _overlay.g = _overlay.b = 255;
	}

	++_framesCount;
	if ((_framesCount & 31) == 0) {
		struct timeval t1;
		gettimeofday(&t1, 0);
		const int msecs = (t1.tv_sec - _frameTimeStamp.tv_sec) * 1000 + (t1.tv_usec - _frameTimeStamp.tv_usec) / 1000;
		_frameTimeStamp = t1;
		if (msecs != 0) {
			_framesPerSec = (int)(1000. * 32 / msecs);
		}
	}
}

const uint8_t *Render::captureScreen(int *w, int *h) {
	if (!_screenshotBuf) {
		_screenshotBuf = (uint8_t *)calloc(_w * _h, 4);
	}
	if (_screenshotBuf) {
		glReadPixels(0, 0, _w, _h, GL_RGBA, GL_UNSIGNED_BYTE, _screenshotBuf);
		*w = _w;
		*h = _h;
	}
	return _screenshotBuf;
}
