/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include "game.h"
#include "render.h"

enum {
	kOptionSave,
	kOptionLoad,
};

static const int kMsgMenuSave = 51;
static const int kMsgMenuCancel = 55;
static const int kMsgMenuLoad = 56;

static const char *kMenuFnTga_s = "f2bgl-freesav%d.tga";

static int _currentOption = kOptionSave;

static int _rotateDirection;
static int _rotateTargetPitch;

static bool _resumeMusic;

static struct SaveLoadTexture {
	int16_t texKey;
	int w, h;
	const uint8_t *data;
} _saveLoadTexture;

static struct {
	int num;
	bool used;
	char description[256];
	SaveLoadTexture texture;
} _saveLoadSlots[kSaveLoadSlots];

void Game::resetObjectAnim(GameObject *o) {
	GameObjectAnimation *anim = &o->anim;
	anim->aniheadData = _res.getData(kResType_ANI, anim->animKey, "ANIHEAD");
	anim->currentAnimKey = _res.getChild(kResType_ANI, anim->animKey);
	anim->anikeyfData = _res.getData(kResType_ANI, anim->currentAnimKey, "ANIKEYF");
	anim->framesCount = 0;
	anim->ticksCount = 0;
}

void Game::initMenu() {
	setPalette(_roomsTable[_objectsPtrTable[kObjPtrConrad]->room].palKey);
	GameObject *o = _objectsPtrTable[kObjPtrSaveloadOption];
	resetObjectAnim(o);
	o->specialData[1][20] = (_currentOption == kOptionLoad) ? 1 : 0;
	loadMenuObjectMesh(o, o->anim.currentAnimKey);
	o->specialData[1][9] = 16;
	SceneObject *so = &_sceneObjectsTable[0];
	so->o = o;
	so->x =    0;
	so->y = -(38 << kPosShift);
	so->z =   56 << kPosShift;
	so->pitch = 512;

	static const uint8_t texIndexLut[] = { 7, 6, 5, 4, 3, 2, 1, 0 };

	// enumerate the saves
	for (int i = 1; i < kSaveLoadSlots; ++i) {
		_saveLoadSlots[i].num = -i;
		_saveLoadSlots[i].used = hasSavedGameState(_saveLoadSlots[i].num);
		snprintf(_saveLoadSlots[i].description, sizeof(_saveLoadSlots[i].description), "freesav%d", i);
		if (_saveLoadSlots[i].used) {
			char filename[32];
			snprintf(filename, sizeof(filename), kMenuFnTga_s, i);
			_saveLoadSlots[i].texture.data = loadTGA(filename, &_saveLoadSlots[i].texture.w, &_saveLoadSlots[i].texture.h);
			if (_saveLoadSlots[i].texture.data && _saveLoadSlots[i].texture.w > 0 && _saveLoadSlots[i].texture.h > 0) {
				_render->prepareTextureRgb(_saveLoadSlots[i].texture.data, _saveLoadSlots[i].texture.w, _saveLoadSlots[i].texture.h, kSaveLoadTexKey + texIndexLut[i]);				}
		} else {
			memset(&_saveLoadSlots[i].texture, 0, sizeof(_saveLoadSlots[i].texture));
		}
		_saveLoadSlots[i].texture.texKey = kSaveLoadTexKey + texIndexLut[i];
	}
	// current game state screenshot
	_saveLoadTexture.data = _render->captureScreen(&_saveLoadTexture.w, &_saveLoadTexture.h);
	_render->prepareTextureRgb(_saveLoadTexture.data, _saveLoadTexture.w, _saveLoadTexture.h, kSaveLoadTexKey + texIndexLut[0]);

	_snd.playMidi("savemap.xmi");
	_resumeMusic = true;
}

void Game::finiMenu() {
	for (int i = 0; i < kSaveLoadSlots; ++i) {
		_render->releaseTexture(kSaveLoadTexKey + i);
		uint8_t *texData = (uint8_t *)_saveLoadSlots[i].texture.data;
		free(texData);
		memset(&_saveLoadSlots[i].texture, 0, sizeof(_saveLoadSlots[i].texture));
	}
	if (_resumeMusic) {
		_snd.playMidi(_objectsPtrTable[kObjPtrWorld]->objKey, _snd._musicKey);
	}
}

void Game::loadMenuObjectMesh(GameObject *o, int16_t key) {
	SceneObject *so = &_sceneObjectsTable[0];
	key = _res.getChild(kResType_ANI, key);
	const uint8_t *p_anifram = _res.getData(kResType_ANI, key, "ANIFRAM");
	if (p_anifram[2] == 9) {
		uint8_t *p_poly3d;
		uint8_t *p_form3d = initMesh(kResType_F3D, READ_LE_UINT16(p_anifram), &so->verticesData, &so->polygonsData, o, &p_poly3d, &o->specialData[1][20]);
		so->verticesCount = READ_LE_UINT16(p_form3d + 18);
		// find texture ids for save states screenshots
		int textureIdCount = 0;
		const uint8_t *polygonsData = so->polygonsData;
		int count = *polygonsData++;
		assert((count & 0x80) == 0);
		while (count != 0) {
			const int color = READ_LE_UINT16(polygonsData); polygonsData += 2;
			const int indexSize = (count & 0x40) != 0 ? 1 : 2;
			polygonsData += ((count & 15) + 1) * indexSize;
			if (((color >> 8) & 31) == 12) {
				_saveLoadTextureIdTable[textureIdCount] = (color & 255);
				++textureIdCount;
				if (textureIdCount >= kSaveLoadSlots) {
					break;
				}
			}
			count = *polygonsData++;
		}
	}
}

static void toggleOption(GameObject *o) {
	switch (_currentOption) {
	case kOptionLoad:
		_currentOption = kOptionSave;
		break;
	case kOptionSave:
		_currentOption = kOptionLoad;
		break;
	}
	o->specialData[1][20] = (_currentOption == kOptionLoad) ? 1 : 0;
}

static int getSaveSlot(int pitch) {
	return ((pitch + 512) & 1023) / (1024 / kSaveLoadSlots);
}

static bool pointerTap(const PlayerInput &inp, int x, int y, int w, int h) {
	if (!inp.pointers[0][0].down && inp.pointers[0][1].down) {
		if (inp.pointers[0][0].x >= x && inp.pointers[0][0].x < x + w) {
			if (inp.pointers[0][0].y >= y && inp.pointers[0][0].y < y + h) {
				return true;
			}
		}
	}
	return false;
}

bool Game::doMenu() {
	_render->clearScreen();
	_render->setupProjection(kProjMenu);

	SceneObject *so = &_sceneObjectsTable[0];
	_render->beginObjectDraw(so->x, so->y, so->z, so->pitch, kPosShift);
	drawSceneObjectMesh(so);
	_render->endObjectDraw();

	_render->setupProjection(kProj2D);

	GameObject *o = _objectsPtrTable[kObjPtrSaveloadOption];

	if (getMessage(_objectsPtrTable[kObjPtrWorld]->objKey, _currentOption == kOptionLoad ? kMsgMenuLoad : kMsgMenuSave, &_tmpMsg)) {
		memset(&_drawCharBuf, 0, sizeof(_drawCharBuf));
		int w, h;
		getStringRect((const char *)_tmpMsg.data, _tmpMsg.font, &w, &h);
		const int x = (kScreenWidth - w) / 2;
		const int y = 8;
		drawString(x, y, (const char *)_tmpMsg.data, _tmpMsg.font, 0);
		if (pointerTap(inp, x, y, w, h)) {
			toggleOption(o);
			loadMenuObjectMesh(o, o->anim.currentAnimKey);
		}
	}

	if (_rotateDirection != 0) {
		so->pitch += o->specialData[1][9] * _rotateDirection;
		so->pitch &= 1023;
		if (so->pitch == _rotateTargetPitch) {
			_rotateDirection = 0;
		}
	} else {
		if ((inp.dirMask & kInputDirLeft) || pointerTap(inp, 0, 50, 64, 100)) {
			_rotateDirection = 1;
			_rotateTargetPitch = (so->pitch + 128) & 1023;
		}
		if ((inp.dirMask & kInputDirRight) || pointerTap(inp, 256, 50, 64, 100)) {
			_rotateDirection = -1;
			_rotateTargetPitch = (so->pitch - 128) & 1023;
		}
	}

	if (inp.dirMask & kInputDirUp) {
		inp.dirMask &= ~kInputDirUp;
		toggleOption(o);
		loadMenuObjectMesh(o, o->anim.currentAnimKey);
	}
	if (inp.dirMask & kInputDirDown) {
		inp.dirMask &= ~kInputDirDown;
		toggleOption(o);
		loadMenuObjectMesh(o, o->anim.currentAnimKey);
	}

	const int saveSlot = getSaveSlot(so->pitch);

	memset(&_drawCharBuf, 0, sizeof(_drawCharBuf));
	if (saveSlot == 0) {
		if (getMessage(_objectsPtrTable[kObjPtrWorld]->objKey, kMsgMenuCancel, &_tmpMsg)) {
			int w, h;
			getStringRect((const char *)_tmpMsg.data, _tmpMsg.font, &w, &h);
			const int x = (kScreenWidth - w) / 2;
			const int y = kScreenHeight / 2 + 80;
			drawString(x, y, (const char *)_tmpMsg.data, _tmpMsg.font, 0);
			if (pointerTap(inp, x, y, w, h)) {
				inp.enterKey = true;
			}
		}
	} else {
		int w, h;
		getStringRect(_saveLoadSlots[saveSlot].description, kFontNormale, &w, &h);
		const int x = (kScreenWidth - w) / 2;
		const int y = kScreenHeight / 2 + 80;
		drawString(x, y, _saveLoadSlots[saveSlot].description, kFontNormale, 0);
		if (pointerTap(inp, x, y, w, h)) {
			inp.enterKey = true;
		}
	}

	if (inp.enterKey) {
		inp.enterKey = false;
		switch (_currentOption) {
		case kOptionSave:
			if (saveSlot == 0) { // cancel
				return false;
			} else {
				_saveLoadSlots[saveSlot].used = false;
				if (!_saveLoadSlots[saveSlot].used) {
					saveGameState(_saveLoadSlots[saveSlot].num);
					char filename[32];
					snprintf(filename, sizeof(filename), kMenuFnTga_s, saveSlot);
					saveTGA(filename, _saveLoadTexture.data, _saveLoadTexture.w, _saveLoadTexture.h, true);
					// game state saved, return to the game
					setGameStateSave(saveSlot);
					return false;
				}
			}
			break;
		case kOptionLoad:
			if (saveSlot == 0) { // cancel
				return false;
			} else {
				if (_saveLoadSlots[saveSlot].used) {
					loadGameState(_saveLoadSlots[saveSlot].num);
					// game state loaded, return to the game
					setGameStateLoad(saveSlot);
					// and do not resume level music
					_resumeMusic = false;
					return false;
				}
			}
			break;
		}
	}

	return true;
}
