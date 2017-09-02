
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

#define kTgaImageTypeUncompressedTrueColor 2
#define kTgaImageTypeRunLengthEncodedTrueColor 10

static const int TGA_HEADER_SIZE = 18;

void saveTGA(const char *filepath, const uint8_t *rgb, int w, int h) {
	static const uint8_t kImageType = kTgaImageTypeRunLengthEncodedTrueColor;
	uint8_t buffer[TGA_HEADER_SIZE];
	buffer[0]            = 0; // ID Length
	buffer[1]            = 0; // ColorMap Type
	buffer[2]            = kImageType;
	TO_LE16(buffer +  3,   0); // ColorMap Start
	TO_LE16(buffer +  5,   0); // ColorMap Length
	buffer[7]            = 0;  // ColorMap Bits
	TO_LE16(buffer +  8,   0); // X-origin
	TO_LE16(buffer + 10,   0); // Y-origin
	TO_LE16(buffer + 12,   w); // Image Width
	TO_LE16(buffer + 14,   h); // Image Height
	buffer[16]           = 24; // Pixel Depth
	buffer[17]           = 0;  // Descriptor

	File *f = fileOpen(filepath, 0, kFileType_SCREENSHOT);
	if (f) {
		fileWrite(f, buffer, sizeof(buffer));
		if (kImageType == kTgaImageTypeUncompressedTrueColor) {
			for (int i = 0; i < w * h; ++i) {
				fileWriteByte(f, rgb[2]);
				fileWriteByte(f, rgb[1]);
				fileWriteByte(f, rgb[0]);
				rgb += 3;
			}
		} else {
			assert(kImageType == kTgaImageTypeRunLengthEncodedTrueColor);
			int prevColor = rgb[0] + (rgb[1] << 8) + (rgb[2] << 16); rgb += 3;
			int count = 0;
			for (int i = 1; i < w * h; ++i) {
				int color = rgb[0] + (rgb[1] << 8) + (rgb[2] << 16); rgb += 3;
				if (prevColor == color && count < 127) {
					++count;
					continue;
				}
				fileWriteByte(f, count | 0x80);
				fileWriteByte(f, (prevColor >> 16) & 255);
				fileWriteByte(f, (prevColor >>  8) & 255);
				fileWriteByte(f,  prevColor        & 255);
				count = 0;
				prevColor = color;
			}
			if (count != 0) {
				fileWriteByte(f, count | 0x80);
				fileWriteByte(f, (prevColor >> 16) & 255);
				fileWriteByte(f, (prevColor >>  8) & 255);
				fileWriteByte(f,  prevColor        & 255);
			}
		}
		fileClose(f);
	}
}

uint8_t *loadTGA(const char *filepath, int *w, int *h) {
	*w = *h = 0;
	uint8_t *buffer = 0;
	File *f = fileOpen(filepath, 0, kFileType_SCREENSHOT);
	if (f) {
		uint8_t header[TGA_HEADER_SIZE];
		fileRead(f, header, sizeof(header));
		const int hdrW = READ_LE_UINT16(header + 12);
		const int hdrH = READ_LE_UINT16(header + 14);
		const int bufferSize = hdrW * hdrH * 3;
		buffer = (uint8_t *)calloc(bufferSize, 1);
		if (buffer) {
			// This is not a generic TGA loader and only support decoding the output of saveTGA
			if (header[2] == kTgaImageTypeRunLengthEncodedTrueColor) {
				int offset = 0;
				while (offset < bufferSize) {
					const int count = (fileReadByte(f) & 0x7F) + 1;
					const int b = fileReadByte(f);
					const int g = fileReadByte(f);
					const int r = fileReadByte(f);
					for (int i = 0; i < count && offset < bufferSize; ++i) {
						buffer[offset++] = r;
						buffer[offset++] = g;
						buffer[offset++] = b;
					}
				}
				*w = hdrW;
				*h = hdrH;
			}
		}
		fileClose(f);
	}
	return buffer;
}
