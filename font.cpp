/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include "game.h"
#include "render.h"

void Game::loadFont(int num, int h, int w, int spacing, int16_t fontKey) {
	assert(num < kFontTableSize);
	Font *ft = &_fontsTable[num];
	memset(ft, 0, sizeof(Font));
	ft->h = h;
	ft->w = w;
	ft->spacing = spacing;
	int16_t key = fontKey;
	for (int i = 0; key != 0 && i < kFontGlyphsCount; ++i) {
		initSprite(kResType_SPR, key, &ft->glyphs[i]);
		ft->glyphs[i].data = _spriteCache.getData(key, ft->glyphs[i].data);
		ft->glyphs[i].key = key;
		_res.unload(kResType_SPR, key);
		key = _res.getNext(kResType_SPR, key);
	}
}

static const struct {
	int num;
	int h, w, spacing;
	const char *keyPath;
} _fontsInitTable[] = {
	{ kFontNormale, 8, 8, 0, "SPR_TREE:/fb2/fonts/normale/Spr_0" },
	{ kFontMenuCart, 6, 8, -1, "SPR_TREE:/fb2/fonts/menu_cart/Spr_0" },
	{ kFontNameCart, 8, 8, 1, "SPR_TREE:/fb2/fonts/name_cart/Spr_0" },
	{ kFontNbCart, 14, 8, 1, "SPR_TREE:/fb2/fonts/nb_cart/Spr_0" },
	{ kFontNameInvent, 9, 8, -1, "SPR_TREE:/fb2/fonts/name_invent/Spr_0" },
	{ kFontNameCineTypo, 14, 8, 1, "SPR_TREE:/fb2/fonts/cine_typo/Spr_0" }
};

void Game::initFonts() {
	for (int i = 0; i < ARRAYSIZE(_fontsInitTable); ++i) {
		int16_t key = _res.getKeyFromPath(_fontsInitTable[i].keyPath);
		assert(key != 0);
		loadFont(_fontsInitTable[i].num, _fontsInitTable[i].h, _fontsInitTable[i].w, _fontsInitTable[i].spacing, key);
	}
}

void Game::drawChar(int x, int y, SpriteImage *spr, int color) {
	if (_drawCharBuf.draw) {
		_drawCharBuf.draw(&_drawCharBuf, x, y, spr->w, spr->h, spr->data);
	} else {
		_render->drawSprite(x, y, spr->data, spr->w, spr->h, 0, spr->key);
	}
}

void Game::drawString(int x, int y, const char *str, int font, int color) {
	font &= 31;
	assert(font < kFontTableSize);
	Font *ft = &_fontsTable[font];
	SpriteImage *spr;
	int chrX = x;
	int chrY = y;
	int chr;
	while ((chr = (unsigned char)*str++) != 0) {
		switch (chr) {
		case '\t':
			chrX += 8 * ft->w;
			break;
		case '@':
		case '|':
			if (font == kFontNameCineTypo) {
				return;
			}
			chrY += ft->h;
			chrX = x;
			break;
		case ' ':
			chrX += ft->w;
			break;
		default:
			chr -= 33;
			assert(chr >= 0 && chr < kFontGlyphsCount);
			spr = &ft->glyphs[chr];
			drawChar(chrX, chrY + ft->h - spr->h, spr, color);
			chrX += spr->w + ft->spacing;
			break;
		}
	}
}

int Game::getStringRect(const char *str, int font, int *w, int *h) {
	font &= 31;
	assert(font < kFontTableSize);
	Font *ft = &_fontsTable[font];
	int chrHeight = 0;
	int chrWidth = 0;
	int chrMaxWidth = 0;
	int chr;
	int offset = 0;
	for (; (chr = (uint8_t)str[offset]) != 0; ++offset) {
		switch (chr) {
		case '\t':
			chrWidth += 8 * ft->w;
			break;
		case '@':
		case '|':
			if (font == kFontNameCineTypo) {
				goto end;
			}
			if (chrWidth > chrMaxWidth) {
				chrMaxWidth = chrWidth;
			}
			chrHeight += ft->h;
			chrWidth = 0;
			break;
		case ' ':
			chrWidth += ft->w;
			break;
		default:
			chr -= 33;
			assert(chr >= 0 && chr < kFontGlyphsCount);
			chrWidth += ft->glyphs[chr].w + ft->spacing;
			break;
		}
	}
end:
	chrHeight += ft->h;
	*w = MAX(chrMaxWidth, chrWidth);
	*h = chrHeight;
	return offset;
}

static int getScaledCoord(int x, int f, int w) {
	return x;
}

void Game::setStringPos(GamePlayerMessage *msg) {
	assert(msg);

	if (msg->xPos < 0) {
		if (msg->xPos == -2) {
			msg->desc.xPos = 3;
		} else if (msg->xPos == -3) {
			msg->desc.xPos = kScreenWidth - msg->w - 3;
		} else {
			msg->desc.xPos = (kScreenWidth - msg->w) / 2;
		}
	} else {
		msg->desc.xPos = getScaledCoord(msg->xPos, _viewportSize, kScreenWidth);
	}
	if (msg->desc.xPos < 0) {
		msg->desc.xPos = 0;
	}

	if (msg->yPos < 0) {
		if (msg->yPos == -2) {
			msg->desc.yPos = 3;
		} else if (msg->yPos == -3) {
			msg->desc.yPos = kScreenHeight - msg->h - 3;
		} else {
			msg->desc.yPos = (kScreenHeight - msg->h) / 2;
		}
	} else {
		msg->desc.yPos = getScaledCoord(msg->yPos, _viewportSize, kScreenHeight);
	}
	if (msg->desc.yPos < 0) {
		msg->desc.yPos = 0;
	}

	if (msg->w == 0 || msg->h == 0) {
		return;
	}
	msg->desc.yPos -= 10;
	bool loopHorizontalPos = true;
	while (loopHorizontalPos) {
		msg->desc.yPos += 10;
		if (msg->desc.yPos + msg->h > kScreenHeight) {
			msg->desc.yPos = kScreenHeight - msg->h - 1;
			break;
		}
		loopHorizontalPos = false;
		for (int i = 0; i < kPlayerMessagesTableSize; ++i) {
			GamePlayerMessage *testMsg = &_playerMessagesTable[i];
			if (testMsg == msg || testMsg->desc.duration == 0) {
				continue;
			}
			const int y1 = msg->desc.yPos;
			const int h1 = msg->h + 1;
			const int y2 = testMsg->desc.yPos;
			const int h2 = testMsg->h + 1;
			if (y1 >= y2 && y1 <= y2 + h2) {
				loopHorizontalPos = true;
				break;
			}
			if (y1 + h1 >= y2 && y1 + h1 <= y2 + h2) {
				loopHorizontalPos = true;
				break;
			}
			if (y1 <= y2 && y1 + h1 >= y2 + h2) {
				loopHorizontalPos = true;
				break;
			}
			if (y1 >= y2 && y1 + h1 <= y2 + h2) {
				loopHorizontalPos = true;
				break;
			}
		}
	}
}
