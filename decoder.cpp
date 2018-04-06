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
				// LE16 - offset,count - bits == 4
				const int offset = (src[1] << 4) | (src[0] >> 4);
				int count = (src[0] & 15) + 2;
				src += 2;
				if (count > decodedSize) {
					warning("Invalid end of stream for compressed LZSS data, size %d count %d", decodedSize, count);
					count = decodedSize;
				}
				decodedSize -= count;
				while (count-- != 0) {
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
				// LE16 - count,offset - bits == 10
				int offset = READ_LE_UINT16(src); src += 2;
				int count = (offset >> bits) + 2;
				offset &= (1 << bits) - 1;
				if (offset == 0) { // end of data marker
					return;
				}
				if (count > decodedSize) {
					warning("Invalid end of stream for compressed RAC data, size %d count %d", decodedSize, count);
					count = decodedSize;
				}
				decodedSize -= count;
				while (count-- != 0) {
					*dst = *(dst - offset);
					++dst;
				}
			}
		}
	}
}
