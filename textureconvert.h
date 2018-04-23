
#ifndef TEXTURECONVERT_H__
#define TEXTURECONVERT_H__

#include "util.h"

uint32_t yuv420_to_rgba(int Y, int U, int V);
uint32_t bgr555_to_rgba(uint16_t color);

#endif // TEXTURECONVERT_H__
