
#include "util.h"
#include "file.h"

static void TO_LE16(uint8_t *dst, uint16_t value) {
	for (int i = 0; i < 2; ++i) {
		dst[i] = value & 255;
		value >>= 8;
	}
}

static void TO_LE32(uint8_t *dst, uint32_t value) {
	for (int i = 0; i < 4; ++i) {
		dst[i] = value & 255;
		value >>= 8;
	}
}

static uint32_t TO_ARGB(const uint8_t *p) {
	const int r = p[0];
	const int g = p[1];
	const int b = p[2];
        return 0xFF000000 | (r << 16) | (g << 8) | b;
}

void saveBMP(const char *filepath, const uint8_t *rgb, int w, int h) {
	static const int FILE_HEADER_SIZE = 14;
	static const int INFO_HEADER_SIZE = 40;
	const int alignedWidth = ((w + 3) & ~3) * 4;
	const int imageSize = alignedWidth * h;
	const int bufferSize = FILE_HEADER_SIZE + INFO_HEADER_SIZE + imageSize;
	uint8_t *buffer = (uint8_t *)calloc(bufferSize, 1);
	if (buffer) {
		int offset = 0;

		// file header
		TO_LE16(buffer + offset, 0x4D42); offset += 2;
		TO_LE32(buffer + offset, bufferSize); offset += 4;
		TO_LE16(buffer + offset, 0); offset += 2; // reserved1
		TO_LE16(buffer + offset, 0); offset += 2; // reserved2
		TO_LE32(buffer + offset, FILE_HEADER_SIZE + INFO_HEADER_SIZE); offset += 4;

		// info header
		TO_LE32(buffer + offset, INFO_HEADER_SIZE); offset += 4;
		TO_LE32(buffer + offset, w);  offset += 4;
		TO_LE32(buffer + offset, h);  offset += 4;
		TO_LE16(buffer + offset, 1);  offset += 2; // planes
		TO_LE16(buffer + offset, 32); offset += 2; // bit_count
		TO_LE32(buffer + offset, 0);  offset += 4; // compression
		TO_LE32(buffer + offset, imageSize); offset += 4; // size_image
		TO_LE32(buffer + offset, 0);  offset += 4; // x_pels_per_meter
		TO_LE32(buffer + offset, 0);  offset += 4; // y_pels_per_meter
		TO_LE32(buffer + offset, 0);  offset += 4; // num_colors_used
		TO_LE32(buffer + offset, 0);  offset += 4; // num_colors_important

		// bitmap data
		for (int y = 0; y < h; ++y) {
			const uint8_t *p = rgb + y * w * 3;
			for (int x = 0; x < w; ++x) {
				TO_LE32(buffer + offset + x * 4, TO_ARGB(&p[x * 3]));
			}
			offset += alignedWidth;
		}

		File *f = fileOpen(filepath, 0, kFileType_SCREENSHOT);
		if (f) {
			fileWrite(f, buffer, bufferSize);
			fileClose(f);
		}
		free(buffer);
	}
}
