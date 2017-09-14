
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
	kIconHalfTurn,
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
	{ kIconConradStatic, 0, 0, kIconActionStatic },
	{ kIconConradRun, 18, 0, kIconActionRun },
	{ kIconConradWalk, 36, 0, kIconActionWalk },
	{ kIconConradJump, 54, 0, kIconActionJump },
	{ kIconConradDuck, 72, 0, kIconActionDuck },
	{ kIconConradLoad, 90, 0, kIconActionLoad },
	{ kIconConradGun, 108, 0, kIconActionGun },
	{ kIconConradHand, 126, 10, kIconActionHand },
	{ kIconConradHandUse, 126, -10, kIconActionHandUse },
};

static const IconsInitTable _iconsGroupStepTable[] = {
	{ kIconHalfTurn, 0, 0, kIconActionDirHalfTurn },
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
	{ kIconInventoryHand, 0, 0, -1 },
	{ kIconInventoryRight, 12, 0, -1 },
	{ kIconInventoryLeft, -12, 0, -1 },
};

void Game::loadIcon(int16_t key, int num, int x, int y, int action) {
	assert(_iconsCount < ARRAYSIZE(_iconsTable));
	Icon *icon = &_iconsTable[_iconsCount];

	const uint8_t *p_anifram = _res.getData(kResType_ANI, key, "ANIFRAM");
	assert(p_anifram);
	int16_t sprKey = READ_LE_UINT16(p_anifram);
	initSprite(kResType_SPR, sprKey, &icon->spr);
	icon->spr.data = _spriteCache.getData(sprKey, icon->spr.data);
	_res.unload(kResType_SPR, sprKey);

	icon->x = x;
	icon->y = y;
	icon->action = action;
	++_iconsCount;
}

static void loadIconGroup(Game *g, int16_t *aniKeys, const IconsInitTable *icons, int iconsCount, int xPos, int yPos) {
	for (int i = 0; i < iconsCount; ++i) {
		const int16_t key = aniKeys[icons[i].num];
		if (key != 0) {
			g->loadIcon(key, icons[i].num, icons[i].x + xPos, icons[i].y + yPos, icons[i].action);
		}
	}
}

void Game::initIcons(bool inBox) {
	_iconsCount = 0;
	GameObject *o = _objectsPtrTable[kObjPtrIconesSouris];
	if (o) {
		int16_t key = _res.getChild(kResType_ANI, o->anim.currentAnimKey);
		int16_t aniKeys[kIconTableSize];
		for (int i = 0; key != 0 && i < ARRAYSIZE(aniKeys); ++i) {
			aniKeys[i] = key;
			key = _res.getNext(kResType_ANI, key);
		}
		if (inBox) {
			loadIconGroup(this, aniKeys, _iconsGroupCabTable, ARRAYSIZE(_iconsGroupCabTable), _res._userConfig.iconLrCabX, _res._userConfig.iconLrCabY);
		} else {
			if (_level == 12) {
				return;
			}
			loadIconGroup(this, aniKeys, _iconsGroupMoveTable, ARRAYSIZE(_iconsGroupMoveTable), _res._userConfig.iconLrMoveX, _res._userConfig.iconLrMoveY);
			loadIconGroup(this, aniKeys, _iconsGroupStepTable, ARRAYSIZE(_iconsGroupStepTable), _res._userConfig.iconLrStepX, _res._userConfig.iconLrStepY);
			loadIconGroup(this, aniKeys, _iconsGroupToolsTable, ARRAYSIZE(_iconsGroupToolsTable), _res._userConfig.iconLrToolsX, _res._userConfig.iconLrToolsY);
		}
	}
}

void Game::drawIcons() {
	for (int i = 0; i < _iconsCount; ++i) {
		const Icon *icon = &_iconsTable[i];
		_render->drawSprite(icon->x, icon->y, icon->spr.data, icon->spr.w, icon->spr.h, 0, icon->spr.key);
	}
}
