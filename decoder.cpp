/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include "decoder.h"

void decodeLZSS(const uint8_t *src, uint8_t *dst, int decodedSize) {
	while (decodedSize > 0) {
		const int code = *src++;
		for (int bit = 0; bit < 8 && decodedSize > 0; ++bit) {
			if (code & (1 << bit)) {
				*dst++ = *src++;
				--decodedSize;
			} else {
				const int offset = (src[1] << 4) | (src[0] >> 4);
				int size = (src[0] & 15) + 2;
				assert(decodedSize >= size);
				src += 2;
				decodedSize -= size;
				while (size-- != 0) {
					*dst = *(dst - offset - 1);
					++dst;
				}
			}
		}
	}
}
