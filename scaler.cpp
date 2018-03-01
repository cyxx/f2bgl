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
			uint16_t c = *(src + i);
			*(p) = c;
			*(p + 1) = c;
			*(p + dstPitch) = c;
			*(p + dstPitch + 1) = c;
		}
		dst += dstPitch * 2;
		src += srcPitch;
	}
}

void point3x(uint16_t *dst, int dstPitch, const uint16_t *src, int srcPitch, int w, int h) {
	while (h--) {
		uint16_t *p = dst;
		for (int i = 0; i < w; ++i, p += 3) {
			uint16_t c = *(src + i);
			*(p) = c;
			*(p + 1) = c;
			*(p + 2) = c;
			*(p + dstPitch) = c;
			*(p + dstPitch + 1) = c;
			*(p + dstPitch + 2) = c;
			*(p + 2 * dstPitch) = c;
			*(p + 2 * dstPitch + 1) = c;
			*(p + 2 * dstPitch + 2) = c;
		}
		dst += dstPitch * 3;
		src += srcPitch;
	}
}

static void scanline2x(uint16_t *dst0, uint16_t *dst1, const uint16_t *src0, const uint16_t *src1, const uint16_t *src2, int w) {
	uint16_t B, D, E, F, H;

	// ABC
	// DEF
	// GHI

	int x = 0;

	// first pixel (D == E)
	B = *(src0 + x);
	E = *(src1 + x);
	D = E;
	F = *(src1 + x + 1);
	H = *(src2 + x);
	if (B != H && D != F) {
		dst0[0] = D == B ? D : E;
		dst0[1] = B == F ? F : E;
		dst1[0] = D == H ? D : E;
		dst1[1] = H == F ? F : E;
	} else {
		dst0[0] = E;
		dst0[1] = E;
		dst1[0] = E;
		dst1[1] = E;
	}
	dst0 += 2;
	dst1 += 2;

	// center pixels
	E = F;
	for (x = 1; x < w - 1; ++x) {
		B = *(src0 + x);
		F = *(src1 + x + 1);
		H = *(src2 + x);
		if (B != H && D != F) {
			dst0[0] = D == B ? D : E;
			dst0[1] = B == F ? F : E;
			dst1[0] = D == H ? D : E;
			dst1[1] = H == F ? F : E;
		} else {
			dst0[0] = E;
			dst0[1] = E;
			dst1[0] = E;
			dst1[1] = E;
		}
		D = E; E = F;
		dst0 += 2;
		dst1 += 2;
	}

	// last pixel (F == E)
	B = *(src0 + x);
	H = *(src2 + x);
	if (B != H && D != F) {
		dst0[0] = D == B ? D : E;
		dst0[1] = B == F ? F : E;
		dst1[0] = D == H ? D : E;
		dst1[1] = H == F ? F : E;
	} else {
		dst0[0] = E;
		dst0[1] = E;
		dst1[0] = E;
		dst1[1] = E;
	}
}

void scale2x(uint16_t *dst, int dstPitch, const uint16_t *src, int srcPitch, int w, int h) {
	if (w == 1 || h == 1) {
		point2x(dst, dstPitch, src, srcPitch, w, h);
		return;
	}
	assert(w > 1 && h > 1);
	const int dstPitch2 = dstPitch * 2;

	const uint16_t *src0, *src1, *src2;

	// y == 0
	src0 = src;
	src1 = src;
	src2 = src + srcPitch;
	scanline2x(dst, dst + dstPitch, src0, src1, src2, w);
	dst += dstPitch2;

	// center
	src0 = src;
	src1 = src + srcPitch;
	src2 = src + srcPitch * 2;
	for (int y = 1; y < h - 1; ++y) {
		scanline2x(dst, dst + dstPitch, src0, src1, src2, w);
		dst += dstPitch2;

		src0 += srcPitch;
		src1 += srcPitch;
		src2 += srcPitch;
	}

	// y == h-1
	src2 = src1;
	scanline2x(dst, dst + dstPitch, src0, src1, src2, w);
}

static void scanline3x(uint16_t *dst0, uint16_t *dst1, uint16_t *dst2, const uint16_t *src0, const uint16_t *src1, const uint16_t *src2, int w) {
	uint16_t A, B, C, D, E, F, G, H, I;

	// ABC
	// DEF
	// GHI

	int x = 0;

	// first pixel (A == B, D == E and G == H)
	B = *(src0 + x);
	A = B;
	C = *(src0 + x + 1);
	E = *(src1 + x);
	D = E;
	F = *(src1 + x + 1);
	H = *(src2 + x);
	G = H;
	I = *(src2 + x + 1);
	if (B != H && D != F) {
		dst0[0] = D == B ? D : E;
		dst0[1] = (E == B && E != C) || (B == F && E != A) ? B : E;
		dst0[2] = B == F ? F : E;
		dst1[0] = (D == B && E != G) || (D == B && E != A) ? D : E;
		dst1[1] = E;
		dst1[2] = (B == F && E != I) || (H == F && E != C) ? F : E;
		dst2[0] = D == H ? D : E;
		dst2[1] = (D == H && E != I) || (H == F && E != G) ? H : E;
		dst2[2] = H == F ? F : E;
	} else {
		dst0[0] = E;
		dst0[1] = E;
		dst0[2] = E;
		dst1[0] = E;
		dst1[1] = E;
		dst1[2] = E;
		dst2[0] = E;
		dst2[1] = E;
		dst2[2] = E;
	}
	dst0 += 3;
	dst1 += 3;
	dst2 += 3;

	// center pixels
	B = C;
	E = F;
	H = I;
	for (x = 1; x < w - 1; ++x) {
		C = *(src0 + x + 1);
		F = *(src1 + x + 1);
		I = *(src2 + x + 1);
		if (B != H && D != F) {
			dst0[0] = D == B ? D : E;
			dst0[1] = (E == B && E != C) || (B == F && E != A) ? B : E;
			dst0[2] = B == F ? F : E;
			dst1[0] = (D == B && E != G) || (D == B && E != A) ? D : E;
			dst1[1] = E;
			dst1[2] = (B == F && E != I) || (H == F && E != C) ? F : E;
			dst2[0] = D == H ? D : E;
			dst2[1] = (D == H && E != I) || (H == F && E != G) ? H : E;
			dst2[2] = H == F ? F : E;
		} else {
			dst0[0] = E;
			dst0[1] = E;
			dst0[2] = E;
			dst1[0] = E;
			dst1[1] = E;
			dst1[2] = E;
			dst2[0] = E;
			dst2[1] = E;
			dst2[2] = E;
		}
		A = B; B = C;
		D = E; E = F;
		G = H; H = I;
		dst0 += 3;
		dst1 += 3;
		dst2 += 3;
	}

	// last pixel (B == C, E == F and H == I)
	if (B != H && D != F) {
		dst0[0] = D == B ? D : E;
		dst0[1] = (E == B && E != C) || (B == F && E != A) ? B : E;
		dst0[2] = B == F ? F : E;
		dst1[0] = (D == B && E != G) || (D == B && E != A) ? D : E;
		dst1[1] = E;
		dst1[2] = (B == F && E != I) || (H == F && E != C) ? F : E;
		dst2[0] = D == H ? D : E;
		dst2[1] = (D == H && E != I) || (H == F && E != G) ? H : E;
		dst2[2] = H == F ? F : E;
	} else {
		dst0[0] = E;
		dst0[1] = E;
		dst0[2] = E;
		dst1[0] = E;
		dst1[1] = E;
		dst1[2] = E;
		dst2[0] = E;
		dst2[1] = E;
		dst2[2] = E;
	}
}

void scale3x(uint16_t *dst, int dstPitch, const uint16_t *src, int srcPitch, int w, int h) {
	if (w == 1 || h == 1) {
		point3x(dst, dstPitch, src, srcPitch, w, h);
		return;
	}
	assert(w > 1 && h > 1);
	const int dstPitch2 = dstPitch * 2;
	const int dstPitch3 = dstPitch * 3;

	const uint16_t *src0, *src1, *src2;

	// y == 0
	src0 = src;
	src1 = src;
	src2 = src + srcPitch;
	scanline3x(dst, dst + dstPitch, dst + dstPitch2, src0, src1, src2, w);
	dst += dstPitch3;

	// center
	src0 = src;
	src1 = src + srcPitch;
	src2 = src + srcPitch * 2;
	for (int y = 1; y < h - 1; ++y) {
		scanline3x(dst, dst + dstPitch, dst + dstPitch2, src0, src1, src2, w);
		dst += dstPitch3;

		src0 += srcPitch;
		src1 += srcPitch;
		src2 += srcPitch;
	}

	// y == h-1
	src2 = src1;
	scanline3x(dst, dst + dstPitch, dst + dstPitch2, src0, src1, src2, w);
}
