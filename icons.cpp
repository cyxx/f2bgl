
#include "game.h"
#include "render.h"

enum {
	kIconJump = 0,
	kIconRun,
	kIconGun,
	kIconLookFront,
	kIconLookRight,
	kIconAction,
	kIconWalk,
	kIconLoad,
	kIconLookBack,
	kIconLookLeft,
	kIconTurn180,
	kIconCameraFix,
	kIconDuck,
	kIconStepLeft,
	kIconStepBack,
	kIconStepRight,
	kIconCameraMotion,
	kIconCameraRotation,
	kIconInventory,
	kIconOptions,
	kIconMap,
	kIconMode,
	kIconStatic,
	kIconMapZoom,
	kIconMapUnzoom,
	kIconMapLeft,
	kIconMapUp,
	kIconMapRight,
	kIconMapDown,
	kIconConradGun,
	kIconConradRun,
	kIconConradJump,
	kIconConradWalk,
	kIconConradStatic,
	kIconConradDuck,
	kIconConradLoad,
	kIconInventoryLeft = 45,
	kIconInventoryUp,
	kIconInventoryRight,
	kIconInventoryDown,
	kIconInventoryHand,
	kIconConradHand = 60,
	kIconConradHandUse,

	kIconTableSize
};

struct IconsInitTable {
	int num;
	int x, y;
	int action;
};

static const IconsInitTable _iconsGroupMoveTable[] = {
	{ kIconConradRun, 0, 0, kIconActionRun },
	{ kIconConradJump, 18, 0, kIconActionJump },
	{ kIconConradDuck, 36, 0, kIconActionDuck },
	{ kIconConradLoad, 54, 0, kIconActionLoad },
	{ kIconConradGun, 72, 0, kIconActionGun },
	{ kIconConradHand, 90, 10, kIconActionHand },
	{ kIconConradHandUse, 90, -10, kIconActionHandUse },
};

static const IconsInitTable _iconsGroupStepTable[] = {
	{ kIconTurn180, 0, 0, kIconActionDirTurn180 },
	{ kIconMapUp, 0, -12, kIconActionDirUp },
	{ kIconMapDown, 0, 12, kIconActionDirDown },
	{ kIconMapLeft, -12, 0, kIconActionDirLeft },
	{ kIconMapRight, 12, 0, kIconActionDirRight },
};

static const IconsInitTable _iconsGroupToolsTable[] = {
	{ kIconInventory, 0, -12, kIconActionInventory },
	{ kIconOptions, 0, 0, kIconActionOptions },
};

static const IconsInitTable _iconsGroupCabTable[] = {
	{ kIconInventoryHand, 0, 0, kIconActionHand },
	{ kIconInventoryRight, 12, 0, kIconActionDirRight },
	{ kIconInventoryLeft, -12, 0, kIconActionDirLeft },
};

// return false if the icon should not be presented
static bool canUseIcon(int level, int action) {
	if (level == 12) {
		switch (action) {
		case kIconActionRun:
		case kIconActionJump:
		case kIconActionDuck:
		case kIconActionLoad:
		case kIconActionGun:
		case kIconActionHand:
		case kIconActionHandUse:
		case kIconActionDirTurn180:
			return false;
		}
	}
	return true;
}

void Game::loadIcon(int16_t key, int num, int x, int y, int action) {
	assert(_iconsCount < ARRAYSIZE(_iconsTable));
	Icon *icon = &_iconsTable[_iconsCount];

	const uint8_t *p_anifram = _res.getData(kResType_ANI, key, "ANIFRAM");
	assert(p_anifram);
	int16_t sprKey = READ_LE_UINT16(p_anifram);
	initSprite(kResType_SPR, sprKey, &icon->spr);
	icon->spr.data = _spriteCache.getData(sprKey, icon->spr.data);

	icon->x = x;
	icon->y = y;
	icon->xMargin = 2;
	icon->yMargin = 2;
	icon->action = action;
	++_iconsCount;
}

static void loadIconGroup(Game *g, int16_t *aniKeys, const IconsInitTable *icons, int iconsCount, int xPos, int yPos) {
	for (int i = 0; i < iconsCount; ++i) {
		const int16_t key = aniKeys[icons[i].num];
		if (key != 0 && canUseIcon(g->_level, icons[i].action)) {
			g->loadIcon(key, icons[i].num, icons[i].x + xPos, icons[i].y + yPos, icons[i].action);
		}
	}
}

void Game::initIcons(int iconMode) {
	_iconsCount = 0;
	GameObject *o = _objectsPtrTable[kObjPtrIconesSouris];
	if (o) {
		int16_t key = _res.getChild(kResType_ANI, o->anim.currentAnimKey);
		int16_t aniKeys[kIconTableSize];
		for (int i = 0; key != 0 && i < ARRAYSIZE(aniKeys); ++i) {
			aniKeys[i] = key;
			key = _res.getNext(kResType_ANI, key);
		}
		switch (iconMode) {
		case kIconModeCabinet:
			loadIconGroup(this, aniKeys, _iconsGroupCabTable, ARRAYSIZE(_iconsGroupCabTable), _res._userConfig.iconLrCabX, _res._userConfig.iconLrCabY);
			break;
		case kIconModeGame:
			loadIconGroup(this, aniKeys, _iconsGroupMoveTable, ARRAYSIZE(_iconsGroupMoveTable), _res._userConfig.iconLrMoveX, _res._userConfig.iconLrMoveY);
			loadIconGroup(this, aniKeys, _iconsGroupStepTable, ARRAYSIZE(_iconsGroupStepTable), _res._userConfig.iconLrStepX, _res._userConfig.iconLrStepY);
			loadIconGroup(this, aniKeys, _iconsGroupToolsTable, ARRAYSIZE(_iconsGroupToolsTable), _res._userConfig.iconLrToolsX, _res._userConfig.iconLrToolsY);
			break;
		}
	}
}

void Game::drawIcons() {
	for (int i = 0; i < _iconsCount; ++i) {
		const Icon *icon = &_iconsTable[i];
		_render->drawSprite(icon->x, icon->y, icon->spr.data, icon->spr.w, icon->spr.h, 0, icon->spr.key);
	}
}
