
#include "util.h"
#include "file.h"

static const bool kLinearResize = true; // bilinear resampling of screenshot bitmap

static uint32_t qlerp(uint32_t a, uint32_t b, uint32_t c, uint32_t d, int u, int v) {
	return (a * (256 - u) * (256 - v) + b * u * (256 - v) + c * (256 - u) * v + c * u * v) >> 16;
}

static uint8_t *resizeThumbnail(const uint8_t *rgba, int w, int h, int *rw, int *rh) {
	const int thumbW = 320;
	const int thumbH = h * thumbW / w;
	uint8_t *buffer = (uint8_t *)malloc(thumbW * thumbH * 4);
	if (buffer) {
		if (!kLinearResize) {
			for (int y = 0; y < thumbH; ++y) {
				const int ty = y * h / thumbH;
				for (int x = 0; x < thumbW; ++x) {
					const int tx = x * w / thumbW;
					memcpy(buffer + (y * thumbW + x) * 4, rgba + (ty * w + tx) * 4, 4);
				}
			}
		} else {
			for (int y = 0; y < thumbH; ++y) {
				const int ty = y * (h - 1) / thumbH;
				const int v = ((y * (h - 1)) % thumbH) * 256 / (thumbH - 1);
				for (int x = 0; x < thumbW; ++x) {
					const int tx = x * (w - 1) / thumbW;
					const int u = ((x * (w - 1)) % thumbW) * 256 / (thumbW - 1);
					// A B
					// C D
					const uint32_t *p = (uint32_t *)(rgba + (ty * w + tx) * 4);
					const uint32_t A = p[0];
					const uint32_t B = p[1];
					const uint32_t C = p[w];
					const uint32_t D = p[w + 1];
					// interpolate ARGB colors
					const uint32_t a = qlerp( A >> 24,         B >> 24,         C >> 24,         D >> 24,        u, v);
					const uint32_t b = qlerp((A >> 16) & 255, (B >> 16) & 255, (C >> 16) & 255, (D >> 16) & 255, u, v);
					const uint32_t g = qlerp((A >>  8) & 255, (B >>  8) & 255, (C >>  8) & 255, (D >>  8) & 255, u, v);
					const uint32_t r = qlerp( A & 255,         B & 255,         C & 255,         D & 255,        u, v);
					*(uint32_t *)(buffer + (y * thumbW + x) * 4) = (a << 24) | (b << 16) | (g << 8) | r;
				}
			}
		}
		*rw = thumbW;
		*rh = thumbH;
	}
	return buffer;
}

static void TO_LE16(uint8_t *dst, uint16_t value) {
	for (int i = 0; i < 2; ++i) {
		dst[i] = value & 255;
		value >>= 8;
	}
}

#define kTgaImageTypeUncompressedTrueColor 2
#define kTgaImageTypeRunLengthEncodedTrueColor 10

static const int TGA_HEADER_SIZE = 18;

void saveTGA(const char *filepath, const uint8_t *rgba, int w, int h, bool thumbnail) {

	uint8_t *thumbBuffer = 0;

	if (thumbnail) {
		int thumbW, thumbH;
		thumbBuffer = resizeThumbnail(rgba, w, h, &thumbW, &thumbH);
		if (thumbBuffer) {
			rgba = thumbBuffer;
			w = thumbW;
			h = thumbH;
		}
	}

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
			fileWriteByte(f, count | 0x80);
			fileWriteByte(f, (prevColor >> 16) & 255);
			fileWriteByte(f, (prevColor >>  8) & 255);
			fileWriteByte(f,  prevColor        & 255);
		}
		fileClose(f);
	}

	free(thumbBuffer);
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
					if (fileEof(f)) {
						warning("Incomplete TGA file '%s' offset %d size %d", filepath, offset, bufferSize);
						break;
					}
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
