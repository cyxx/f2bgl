/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include "scaler.h"

void point1x(uint16_t *dst, int dstPitch, const uint16_t *src, int srcPitch, int w, int h) {
	while (h--) {
		memcpy(dst, src, w * sizeof(uint16_t));
		dst += dstPitch;
		src += srcPitch;
	}
}

void point2x(uint16_t *dst, int dstPitch, const uint16_t *src, int srcPitch, int w, int h) {
	while (h--) {
		uint16_t *p = dst;
		for (int i = 0; i < w; ++i, p += 2) {
			const uint16_t color = *(src + i);
			p[0] = p[1] = color;
			p[dstPitch] = p[dstPitch + 1] = color;
		}
		dst += dstPitch * 2;
		src += srcPitch;
	}
}

void point3x(uint16_t *dst, int dstPitch, const uint16_t *src, int srcPitch, int w, int h) {
	while (h--) {
		uint16_t *p = dst;
		for (int i = 0; i < w; ++i, p += 3) {
			const uint16_t color = *(src + i);
			p[0] = p[1] = p[2] = color;
			p[dstPitch] = p[dstPitch + 1] = p[dstPitch + 2] = color;
			p[dstPitch * 2] = p[dstPitch * 2 + 1] = p[dstPitch * 2 + 2] = color;
		}
		dst += dstPitch * 3;
		src += srcPitch;
	}
}

void scale2x(uint16_t *dst, int dstPitch, const uint16_t *src, int srcPitch, int w, int h) {
	for (int y = 0; y < h; ++y) {
		uint16_t *p = dst;
		for (int x = 0; x < w; ++x, p += 2) {
			const uint16_t E = *(src + x);
			const uint16_t B = (y == 0) ? E : *(src + x - srcPitch);
			const uint16_t D = (x == 0) ? E : *(src + x - 1);
			const uint16_t F = (x == w - 1) ? E : *(src + x + 1);
			const uint16_t H = (y == h - 1) ? E : *(src + x + srcPitch);
			if (B != H && D != F) {
				*(p) = D == B ? D : E;
				*(p + 1) = B == F ? F : E;
				*(p + dstPitch) = D == H ? D : E;
				*(p + dstPitch + 1) = H == F ? F : E;
			} else {
				*(p) = E;
				*(p + 1) = E;
				*(p + dstPitch) = E;
				*(p + dstPitch + 1) = E;
			}
		}
		dst += dstPitch * 2;
		src += srcPitch;
	}
}

void scale3x(uint16_t *dst, int dstPitch, const uint16_t *src, int srcPitch, int w, int h) {
	for (int y = 0; y < h; ++y) {
		uint16_t *p = dst;
		for (int x = 0; x < w; ++x, p += 3) {
			const uint16_t E = *(src + x);
			uint16_t B = (y == 0) ? E : *(src + x - srcPitch);
			uint16_t D = (x == 0) ? E : *(src + x - 1);
			uint16_t F = (x == w - 1) ? E : *(src + x + 1);
			uint16_t H = (y == h - 1) ? E : *(src + x + srcPitch);
			uint16_t A, C;
			if (y == 0) {
				A = D;
				C = F;
			} else {
				A = (x == 0) ? B : *(src + x - srcPitch - 1);
				C = (x == w - 1) ? B : *(src + x - srcPitch + 1);
			}
			uint16_t G, I;
			if (y == h - 1) {
				G = D;
				I = F;
			} else {
				G = (x == 0) ? H : *(src + x + srcPitch - 1);
				I = (x == w - 1) ? H : *(src + x + srcPitch + 1);
			}
			if (B != H && D != F) {
				*(p) = D == B ? D : E;
				*(p + 1) = (D == B && E != C) || (B == F && E != A) ? B : E;
				*(p + 2) = B == F ? F : E;
				*(p + dstPitch) = (D == B && E != G) || (D == B && E != A) ? D : E;
				*(p + dstPitch + 1) = E;
				*(p + dstPitch + 2) = (B == F && E != I) || (H == F && E != C) ? F : E;
				*(p + 2 * dstPitch) = D == H ? D : E;
				*(p + 2 * dstPitch + 1) = (D == H && E != I) || (H == F && E != G) ? H : E;
				*(p + 2 * dstPitch + 2) = H == F ? F : E;
			} else {
				*(p) = E;
				*(p + 1) = E;
				*(p + 2) = E;
				*(p + dstPitch) = E;
				*(p + dstPitch + 1) = E;
				*(p + dstPitch + 2) = E;
				*(p + 2 * dstPitch) = E;
				*(p + 2 * dstPitch + 1) = E;
				*(p + 2 * dstPitch + 2) = E;
			}
		}
		dst += dstPitch * 3;
		src += srcPitch;
	}
}

void eagle2x(uint16_t *dst, int dstPitch, const uint16_t *src, int srcPitch, int w, int h) {
	for (int y = 0; y < h; ++y) {
		uint16_t *p = dst;
		for (int x = 0; x < w; ++x, p += 2) {
			const uint16_t C = *(src + x);
			const uint16_t S = (y == 0 || x == 0) ? C : *(src + x - 1 - srcPitch);
			const uint16_t T = (y == 0) ? C : *(src + x - srcPitch);
			const uint16_t U = (y == 0 || x == w - 1) ? C : *(src + x + 1 - srcPitch);
			const uint16_t V = (x == 0) ? C : *(src + x - 1);
			const uint16_t X = (x == 0 || y == h - 1) ? C : *(src + x - 1 + srcPitch);
			const uint16_t W = (x == w - 1) ? C : *(src + x + 1);
			const uint16_t Y = (y == h - 1) ? C : *(src + x + srcPitch);
			const uint16_t Z = (x == w - 1 || y == h - 1) ? C : *(src + x + 1 + srcPitch);
			*(p) = (V == S && S == T) ? S : C;
			*(p + 1) = (T == U && U == W) ? U : C;
			*(p + dstPitch) = (V == X && X == Y) ? X : C;
			*(p + dstPitch + 1) = (W == Z && Z == Y) ? Z : C;
		}
		dst += dstPitch * 2;
		src += srcPitch;
	}
}
