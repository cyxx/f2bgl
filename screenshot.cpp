
#include "util.h"
#include "file.h"

static void TO_LE16(uint8_t *dst, uint16_t value) {
	for (int i = 0; i < 2; ++i) {
		dst[i] = value & 255;
		value >>= 8;
	}
}

#define kTgaImageTypeUncompressedTrueColor 2
#define kTgaImageTypeRunLengthEncodedTrueColor 10

static const int TGA_HEADER_SIZE = 18;

void saveTGA(const char *filepath, const uint8_t *rgba, int w, int h) {
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

	File *f = fileOpen(filepath, 0, kFileType_SCREENSHOT_SAVE);
	if (f) {
		fileWrite(f, buffer, sizeof(buffer));
		if (kImageType == kTgaImageTypeUncompressedTrueColor) {
			for (int i = 0; i < w * h; ++i) {
				fileWriteByte(f, rgba[2]);
				fileWriteByte(f, rgba[1]);
				fileWriteByte(f, rgba[0]);
				rgba += 4;
			}
		} else {
			assert(kImageType == kTgaImageTypeRunLengthEncodedTrueColor);
			int prevColor = rgba[0] + (rgba[1] << 8) + (rgba[2] << 16); rgba += 4;
			int count = 0;
			for (int i = 1; i < w * h; ++i) {
				int color = rgba[0] + (rgba[1] << 8) + (rgba[2] << 16); rgba += 4;
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
	File *f = fileOpen(filepath, 0, kFileType_SCREENSHOT_LOAD);
	if (f) {
		uint8_t header[TGA_HEADER_SIZE];
		fileRead(f, header, sizeof(header));
		const int hdrW = READ_LE_UINT16(header + 12);
		const int hdrH = READ_LE_UINT16(header + 14);
		const int bufferSize = hdrW * hdrH * 4;
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
						buffer[offset++] = 255;
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
