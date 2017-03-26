/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#ifndef DECODER_H__
#define DECODER_H__

#include "util.h"

void decodeLZSS(const uint8_t *src, uint8_t *dst, int decodedSize);
void decodeRAC(const uint8_t *src, uint8_t *dst, int decodedSize);

#endif // DECODER_H__
