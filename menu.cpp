/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include "game.h"
#include "render.h"

static const int _y0 = 36;
static const int _y1 = _y0 + 8;

void Game::resetObjectAnim(GameObject *o) {
	GameObjectAnimation *anim = &o->anim;
	anim->aniheadData = _res.getData(kResType_ANI, anim->animKey, "ANIHEAD");
	anim->currentAnimKey = _res.getChild(kResType_ANI, anim->animKey);
	anim->anikeyfData = _res.getData(kResType_ANI, anim->currentAnimKey, "ANIKEYF");
	anim->framesCount = 0;
	anim->ticksCount = 0;
}

int Game::setNextObjectAnim(GameObject *o, int direction) {
	int16_t key;
	GameObjectAnimation *anim = &o->anim;
	switch (direction) {
	case 1:
		key = _res.getNext(kResType_ANI, anim->currentAnimKey);
		if (key == 0) {
			return 0;
		}
		anim->currentAnimKey = key;
		++anim->framesCount;
		break;
	case -1:
		key = _res.getPrevious(kResType_ANI, anim->currentAnimKey);
		if (key == 0) {
			return 0;
		}
		anim->currentAnimKey = key;
		--anim->framesCount;
		break;
	}
	anim->anikeyfData = _res.getData(kResType_ANI, anim->currentAnimKey, "ANIKEYF");
	return 1;
}

void Game::initMenu() {
	setPalette(_roomsTable[_objectsPtrTable[kObjPtrConrad]->room].palKey);
	resetObjectAnim(_objectsPtrTable[kObjPtrSaveloadOption]);
	_objectsPtrTable[kObjPtrSaveloadOption]->specialData[1][9] = 16;
	SceneObject *so = &_sceneObjectsTable[0];
	so->x =     0;
	so->y = -(_y0 << kPosShift);
	so->z =    56 << kPosShift;
}

void Game::loadMenuObjectMesh(GameObject *o, int16_t key) {
	SceneObject *so = &_sceneObjectsTable[0];
	key = _res.getChild(kResType_ANI, key);
	const uint8_t *p_anifram = _res.getData(kResType_ANI, key, "ANIFRAM");
	if (p_anifram[2] == 9) {
		uint8_t *p_poly3d;
		uint8_t *p_form3d = initMesh(kResType_F3D, READ_LE_UINT16(p_anifram), &so->verticesData, &so->polygonsData, o, &p_poly3d, 0);
		so->verticesCount = READ_LE_UINT16(p_form3d + 18);
		so->pitch = (o->pitch + (p_anifram[3] << 4)) & 1023;
	}
}

void Game::doMenu() {
	GameObject *o = _objectsPtrTable[kObjPtrSaveloadOption];
	loadMenuObjectMesh(o, o->anim.currentAnimKey);

	_render->clearScreen();
	_render->setupProjection(kProjMenu);

	SceneObject *so = &_sceneObjectsTable[0];
	_render->beginObjectDraw(so->x, so->y, so->z, so->pitch, kPosShift);
	drawSceneObjectMesh(so->polygonsData, so->verticesData, so->verticesCount);
	_render->endObjectDraw();

	_render->setupProjection2d();
	const int option = (((so->pitch + 512) & 1023) + 64) / 128;
	if (getMessage(o->objKey, option, &_tmpMsg)) {
		memset(&_drawCharBuf, 0, sizeof(_drawCharBuf));
		int w, h;
		getStringRect((const char *)_tmpMsg.data, _tmpMsg.font, &w, &h);
		drawString((kScreenWidth - w) / 2, 0, (const char *)_tmpMsg.data, _tmpMsg.font, 0);
	}

	if (inp.dirMask & kInputDirLeft) {
		o->pitch += o->specialData[1][9];
		o->pitch &= 1023;
	}
	if (inp.dirMask & kInputDirRight) {
		o->pitch -= o->specialData[1][9];
		o->pitch &= 1023;
	}
	if (inp.dirMask & kInputDirUp) {
		setNextObjectAnim(o, 1);
	}
	if (inp.dirMask & kInputDirDown) {
		setNextObjectAnim(o, -1);
	}
	so->y = -(_y0 << kPosShift) - (o->anim.framesCount * (_y1 - _y0) << kPosShift) / (o->anim.aniheadData[1] - 1);
}
