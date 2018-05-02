
#include "textureconvert.h"

uint32_t yuv420_to_rgba(int Y, int U, int V) {
	float R = Y + 1.402 * (V - 128);
	if (R < 0) {
		R = 0;
	} else if (R > 255) {
		R = 255;
	}
	float G = Y - 0.344 * (U - 128) - 0.714 * (V - 128);
	if (G < 0) {
		G = 0;
	} else if (G > 255) {
		G = 255;
	}
	float B = Y + 1.772 * (U - 128);
	if (B < 0) {
		B = 0;
	} else if (B > 255) {
		B = 255;
	}
	return 0xFF000000 | (((uint8_t)B) << 16) | (((uint8_t)G) << 8) | ((uint8_t)R);
}
