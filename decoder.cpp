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

void decodeRAC(const uint8_t *src, uint8_t *dst, int decodedSize) {
	static const int bits = 10;
	while (decodedSize > 0) {
		const uint8_t code = *src++;
		for (int bit = 0; bit < 8 && decodedSize > 0; ++bit) {
			if (code & (1 << bit)) {
				*dst++ = *src++;
				--decodedSize;
			} else {
				int offset = READ_LE_UINT16(src); src += 2;
				const int count = (offset >> bits) + 2;
				offset &= (1 << bits) - 1;
				if (offset == 0) {
					return;
				}
				assert(decodedSize >= count);
				for (int j = 0; j < count; ++j) {
					dst[j] = dst[j - offset];
				}
				dst += count;
				decodedSize -= count;
			}
		}
	}
}
