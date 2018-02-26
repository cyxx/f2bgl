/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#ifndef SCALER_H__
#define SCALER_H__

#include "util.h"

void point1x(uint16_t *dst, int dstPitch, const uint16_t *src, int srcPitch, int w, int h);
void point2x(uint16_t *dst, int dstPitch, const uint16_t *src, int srcPitch, int w, int h);
void point3x(uint16_t *dst, int dstPitch, const uint16_t *src, int srcPitch, int w, int h);
void scale2x(uint16_t *dst, int dstPitch, const uint16_t *src, int srcPitch, int w, int h);
void scale3x(uint16_t *dst, int dstPitch, const uint16_t *src, int srcPitch, int w, int h);

#endif // SCALER_H__
