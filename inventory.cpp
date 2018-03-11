/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include "game.h"

static const int kInventoryCategoryWidth  = 32;
static const int kInventoryCategoryHeight = 23;

static const int kInventoryObjectWidth  = 46;
static const int kInventoryObjectHeight = 32;

static GameObject *_inventoryCategoryObj;
static GameObject *_inventoryCurrentObj;
static int _inventoryCurrentNum;
static int _inventoryCategoryNum;
static int _inventoryFirstObjNum;
static bool _inventoryVoicePlayed;

static bool _3dObj;

enum {
	kInventoryActionCategory1, // Gun
	kInventoryActionCategory2, // Use
	kInventoryActionCategory3, // Proj
	kInventoryActionCategory4, // Scan
	kInventoryActionCategory5, // Shield
	kInventoryActionCategory6, // Message
	kInventoryActionUp,
	kInventoryActionDown,
	kInventoryActionObj1,
	kInventoryActionObj2,
	kInventoryActionObj3,
	kInventoryActionObj4,
	kInventoryActionObj5,
	kInventoryActionToggle3dObj,
};

static const struct {
	int x, y, w, h;
	int action;
} _mouseHotspots[] = {
	{ 49, 5, 32, 23, kInventoryActionCategory1 },
	{ 87, 5, 32, 23, kInventoryActionCategory2 },
	{ 125, 5, 32, 23, kInventoryActionCategory3 },
	{ 163, 5, 32, 23, kInventoryActionCategory4 },
	{ 201, 5, 32, 23, kInventoryActionCategory5 },
	{ 239, 5, 32, 23, kInventoryActionCategory6 },
	{ 137, 33, 40, 14, kInventoryActionUp },
	{ 137, 99, 40, 14, kInventoryActionUp },
	{ 39, 52, 46, 32, kInventoryActionObj1 },
	{ 88, 52, 46, 32, kInventoryActionObj2 },
	{ 137, 52, 46, 32, kInventoryActionObj3 },
	{ 186, 52, 46, 32, kInventoryActionObj4 },
	{ 235, 52, 46, 32, kInventoryActionObj5 },
	{ 137, 129, 45, 20, kInventoryActionToggle3dObj },
};

bool Game::initInventory() {
	if (_res._levelDescriptionsTable[_level].inventory) {
		const int num = _objectsPtrTable[kObjPtrConrad]->specialData[1][20] & 15;
		if (num != 8 && num != 9) {
			const int pal = _objectsPtrTable[kObjPtrInventaire]->pal;
			setPalette(_palKeysTable[pal]);
			_inventoryCategoryObj = _objectsPtrTable[kObjPtrInventaire]->o_child->o_next->o_next->o_next->o_next->o_next;
			_inventoryCurrentObj = _inventoryCategoryObj->o_child;
			_inventoryCurrentNum = 0;
			_inventoryCategoryNum = 5;
                        _3dObj = false;
			_snd.playSfx(_objectsPtrTable[kObjPtrWorld]->objKey, _res._sndKeysTable[3]);
			return true;
		}
	}
	return false;
}

void Game::loadInventoryObjectMesh(GameObject *o) {
	SceneObject *so = &_sceneObjectsTable[0];
	memset(so, 0, sizeof(SceneObject));
	int16_t key = _res.getNext(kResType_ANI, o->anim.currentAnimKey);
	key = _res.getNext(kResType_ANI, key);
	key = _res.getChild(kResType_ANI, key);
	const uint8_t *p_anifram = _res.getData(kResType_ANI, key, "ANIFRAM");
	if (p_anifram[2] == 9) {
		uint8_t *p_poly3d;
		uint8_t *p_form3d = initMesh(kResType_F3D, READ_LE_UINT16(p_anifram), &so->verticesData, &so->polygonsData, o, &p_poly3d, 0);
		so->verticesCount = READ_LE_UINT16(p_form3d + 18);
	}
}

void Game::closeInventory() {
	if (_inventoryCurrentObj) {
		_snd.stopVoice(_inventoryCurrentObj->objKey);
	}
	_snd.playSfx(_objectsPtrTable[kObjPtrWorld]->objKey, _res._sndKeysTable[3]);
	const int num = (_objectsPtrTable[kObjPtrShield]->specialData[1][22] == 1) ? 5 : 0;
	setObjectData(_objectsPtrTable[kObjPtrConrad], 20, num);
}

void Game::updateInventoryInput() {
	if (!inp.pointers[0][0].down && inp.pointers[0][1].down) {
		for (int j = 0; j < ARRAYSIZE(_mouseHotspots); ++j) {
			if (inp.pointers[0][0].x < _mouseHotspots[j].x || inp.pointers[0][0].x > _mouseHotspots[j].x + _mouseHotspots[j].w - 1) {
				continue;
			}
			if (inp.pointers[0][0].y < _mouseHotspots[j].y || inp.pointers[0][0].y > _mouseHotspots[j].y + _mouseHotspots[j].h - 1) {
				continue;
			}

			switch (_mouseHotspots[j].action) {
			case kInventoryActionCategory1:
			case kInventoryActionCategory2:
			case kInventoryActionCategory3:
			case kInventoryActionCategory4:
			case kInventoryActionCategory5:
			case kInventoryActionCategory6:
				_inventoryCategoryObj = _objectsPtrTable[kObjPtrInventaire]->o_child;
				_inventoryCategoryNum = 0;
				for (int i = 0; i < _mouseHotspots[j].action - kInventoryActionCategory1; ++i) {
					_inventoryCategoryObj = _inventoryCategoryObj->o_next;
					++_inventoryCategoryNum;
				}
				_inventoryCurrentObj = _inventoryCategoryObj->o_child;
				_inventoryCurrentNum = 0;
				_snd.playSfx(_objectsPtrTable[kObjPtrWorld]->objKey, _res._sndKeysTable[4]);
				break;
			case kInventoryActionUp:
				inp.dirMask |= kInputDirUp;
				break;
			case kInventoryActionDown:
				inp.dirMask |= kInputDirDown;
				break;
			case kInventoryActionObj1:
			case kInventoryActionObj2:
			case kInventoryActionObj3:
			case kInventoryActionObj4:
			case kInventoryActionObj5:
				_inventoryCurrentObj = _inventoryCategoryObj->o_child;
				_inventoryCurrentNum = 0;
				for (int i = 0; i < _mouseHotspots[j].action - kInventoryActionObj1; ++i) {
					if (!_inventoryCurrentObj->o_next) {
						break;
					}
					_inventoryCurrentObj = _inventoryCurrentObj->o_next;
					++_inventoryCurrentNum;
				}
				_snd.playSfx(_objectsPtrTable[kObjPtrWorld]->objKey, _res._sndKeysTable[5]);
				break;
			case kInventoryActionToggle3dObj:
				inp.shiftKey = true;
				break;
			}
		}
	}

	if (inp.tabKey && !_3dObj) {
		inp.tabKey = false;
		if (inp.shiftKey) {
			if (_inventoryCategoryObj != _objectsPtrTable[kObjPtrInventaire]->o_child) {
				_inventoryCategoryNum--;
			} else {
				_inventoryCategoryNum = 5;
			}
			_inventoryCategoryObj = getPreviousObject(_inventoryCategoryObj);
			_inventoryCurrentObj = _inventoryCategoryObj->o_child;
			_inventoryCurrentNum = 0;
			_snd.playSfx(_objectsPtrTable[kObjPtrWorld]->objKey, _res._sndKeysTable[4]);
		} else {
			_inventoryCategoryObj = getNextObject(_inventoryCategoryObj);
			if (_inventoryCategoryObj != _objectsPtrTable[kObjPtrInventaire]->o_child) {
				_inventoryCategoryNum++;
			} else {
				_inventoryCategoryNum = 0;
			}
			_inventoryCurrentObj = _inventoryCategoryObj->o_child;
			_inventoryCurrentNum = 0;
			_snd.playSfx(_objectsPtrTable[kObjPtrWorld]->objKey, _res._sndKeysTable[4]);
		}
	}
	if ((inp.dirMask & kInputDirUp) != 0 && !_3dObj) {
		inp.dirMask &= ~kInputDirUp;
		if (_inventoryCurrentNum >= 5) {
			_inventoryCurrentNum -= 5;
			for (int i = 0; i < 5; ++i) {
				_inventoryCurrentObj = getPreviousObject(_inventoryCurrentObj);
			}
			_snd.playSfx(_objectsPtrTable[kObjPtrWorld]->objKey, _res._sndKeysTable[5]);
		}
	}
	if ((inp.dirMask & kInputDirDown) != 0 && !_3dObj) {
		inp.dirMask &= ~kInputDirDown;
		const int lastObj = _inventoryCurrentNum + 5;
		const int firstObj = lastObj - lastObj % 5;
		GameObject *tmpObj = _inventoryCurrentObj;
		for (int i = _inventoryCurrentNum; tmpObj && i < firstObj; ++i) {
			tmpObj = tmpObj->o_next;
		}
		if (tmpObj) {
			int i;
			for (i = firstObj; tmpObj->o_next && i < lastObj; ++i) {
				tmpObj = tmpObj->o_next;
			}
			_inventoryCurrentNum = i;
			_inventoryCurrentObj = tmpObj;
			_snd.playSfx(_objectsPtrTable[kObjPtrWorld]->objKey, _res._sndKeysTable[5]);
		}
	}
	if (_inventoryCurrentObj && (inp.dirMask & kInputDirLeft) != 0 && !_3dObj) {
		inp.dirMask &= ~kInputDirLeft;
		if (_inventoryCurrentNum > 0) {
			--_inventoryCurrentNum;
			_inventoryCurrentObj = getPreviousObject(_inventoryCurrentObj);
			_snd.playSfx(_objectsPtrTable[kObjPtrWorld]->objKey, _res._sndKeysTable[5]);
		}
	}
	if (_inventoryCurrentObj && (inp.dirMask & kInputDirRight) != 0 && !_3dObj) {
		inp.dirMask &= ~kInputDirRight;
		if (_inventoryCurrentObj->o_next) {
			++_inventoryCurrentNum;
			_inventoryCurrentObj = _inventoryCurrentObj->o_next;
			_snd.playSfx(_objectsPtrTable[kObjPtrWorld]->objKey, _res._sndKeysTable[5]);
		}
	}
	if (inp.shiftKey) {
		inp.shiftKey = false;
		if (_inventoryCurrentObj) {
			_3dObj = !_3dObj;
			if (_3dObj) {
				if (_inventoryCurrentObj->specialData[1][22] != 32) {
					loadInventoryObjectMesh(_inventoryCurrentObj);
					setPalette(_palKeysTable[_inventoryCurrentObj->pal]);
				}
			} else {
				_snd.stopVoice(_inventoryCurrentObj->objKey);
				_inventoryVoicePlayed = false;
			}
			_snd.playSfx(_objectsPtrTable[kObjPtrWorld]->objKey, _res._sndKeysTable[6]);
		}
	}
	bool nextObject = false;
	if (inp.ctrlKey) {
		inp.ctrlKey = false;
		nextObject = true;
	}
	if (inp.enterKey) {
		inp.enterKey = false;
		nextObject = true;
	}
	if (nextObject && !_3dObj) {
		if (_inventoryCurrentNum > 0 && _inventoryCategoryNum != 5) {
			GameObject *tmpObj = getPreviousObject(_inventoryCurrentObj);
			tmpObj->o_next = _inventoryCurrentObj->o_next;
			_inventoryCurrentObj->o_next = _inventoryCategoryObj->o_child;
			_inventoryCategoryObj->o_child = _inventoryCurrentObj;
			_inventoryCurrentNum = 0;
			switch (_inventoryCategoryNum) {
			case 0:
				_objectsPtrTable[kObjPtrGun] = _inventoryCurrentObj;
				_varsTable[15] = _objectsPtrTable[kObjPtrGun]->objKey;
				_objectsPtrTable[kObjPtrTargetCommand]->specialData[1][16] = 1;
				if (getMessage(_objectsPtrTable[kObjPtrGun]->objKey, 0, &_tmpMsg)) {
					_objectsPtrTable[kObjPtrGun]->text = (const char *)_tmpMsg.data;
				}
				break;
			case 1:
				_objectsPtrTable[kObjPtrUtil] = _inventoryCurrentObj;
				_varsTable[23] = _objectsPtrTable[kObjPtrUtil]->objKey;
				if (getMessage(_objectsPtrTable[kObjPtrUtil]->objKey, 0, &_tmpMsg)) {
					_objectsPtrTable[kObjPtrUtil]->text = (const char *)_tmpMsg.data;
				}
				break;
			case 2:
				_objectsPtrTable[kObjPtrProj] = _inventoryCurrentObj;
				if (getMessage(_objectsPtrTable[kObjPtrProj]->objKey, 0, &_tmpMsg)) {
					_objectsPtrTable[kObjPtrProj]->text = (const char *)_tmpMsg.data;
				}
				break;
			case 3:
				_objectsPtrTable[kObjPtrShield] = _inventoryCurrentObj;
				_varsTable[25] = _objectsPtrTable[kObjPtrShield]->objKey;
				if (getMessage(_objectsPtrTable[kObjPtrShield]->objKey, 0, &_tmpMsg)) {
					_objectsPtrTable[kObjPtrShield]->text = (const char *)_tmpMsg.data;
				}
				break;
			case 4:
				_objectsPtrTable[kObjPtrScan] = _inventoryCurrentObj;
				_varsTable[24] = _objectsPtrTable[kObjPtrScan]->objKey;
				if (getMessage(_objectsPtrTable[kObjPtrScan]->objKey, 0, &_tmpMsg)) {
					_objectsPtrTable[kObjPtrScan]->text = (const char *)_tmpMsg.data;
				}
				break;
			}
			_snd.playSfx(_objectsPtrTable[kObjPtrWorld]->objKey, _res._sndKeysTable[7]);
		}
	}
}

void Game::doInventory() {

	_render->clearScreen();
	_render->setupProjection(kProj2D);
	_render->setIgnoreDepth(true);
	drawSprite(1, 1, _inventoryBackgroundKey);

	if (!_3dObj) {
		int x = (kScreenWidth - (6 * kInventoryCategoryWidth + 5 * 6)) / 2;
		int y = 5;
		GameObject *tmpObj = _objectsPtrTable[kObjPtrInventaire]->o_child;
		for (int i = 0; i < 6; ++i) {
			if (i != _inventoryCategoryNum) {
				const int sprKey = READ_LE_UINT16(tmpObj->anim.aniframData);
				drawSprite(x, y, sprKey);
			} else {
				const int nextKey = _res.getNext(kResType_ANI, tmpObj->anim.currentAnimKey);
				const int childKey = _res.getChild(kResType_ANI, nextKey);
				const uint8_t *p_anifram = _res.getData(kResType_ANI, childKey, "ANIFRAM");
				const int sprKey = READ_LE_UINT16(p_anifram);
				drawSprite(x, y, sprKey);
			}
			x += kInventoryCategoryWidth + 6;
			tmpObj = tmpObj->o_next;
		}
		const int xStart = (kScreenWidth - (5 * kInventoryObjectWidth + 12)) / 2;
		x = xStart;
		y = 15 + kInventoryCategoryHeight + 14;
		if (_inventoryCurrentNum < _inventoryFirstObjNum) {
			_inventoryFirstObjNum = (_inventoryCurrentNum / 5) * 5;
		} else if (_inventoryCurrentNum >= _inventoryFirstObjNum + 5) {
			_inventoryFirstObjNum = (_inventoryCurrentNum / 5) * 5;
		}
		tmpObj = _inventoryCategoryObj->o_child;
		for (int i = 0; i < _inventoryFirstObjNum; ++i) {
			tmpObj = tmpObj->o_next;
		}
		int count = 0;
		for (int ind = _inventoryFirstObjNum; ind < (_inventoryFirstObjNum + 5) && tmpObj; ++ind) {
			if (ind != _inventoryCurrentNum) {
				const int sprKey = READ_LE_UINT16(tmpObj->anim.aniframData);
				drawSprite(x, y, sprKey);
			} else {
				const int nextKey = _res.getNext(kResType_ANI, tmpObj->anim.currentAnimKey);
				const int childKey = _res.getChild(kResType_ANI, nextKey);
				const uint8_t *p_anifram = _res.getData(kResType_ANI, childKey, "ANIFRAM");
				drawSprite(x, y, READ_LE_UINT16(p_anifram));
				if (getMessage(tmpObj->objKey, 1, &_tmpMsg)) {
					memset(&_drawCharBuf, 0, sizeof(_drawCharBuf));
					const int len = strlen((const char *)_tmpMsg.data);
					const int xTmp = (kScreenWidth - len * 8) / 2;
					const int yTmp = 4 * 5 + kInventoryCategoryHeight + 14 + kInventoryObjectHeight - 3;
					drawString(xTmp, yTmp, (const char *)_tmpMsg.data, _tmpMsg.font, 0);
				}
			}
			++count;
			if (count >= 5) {
				count = 0;
				x = xStart;
				y += kInventoryObjectHeight + 5;
			} else {
				x += kInventoryObjectWidth + 3;
			}
			tmpObj = tmpObj->o_next;
		}
		if (_inventoryFirstObjNum > 0) {
			drawSprite((kScreenWidth - kInventoryObjectWidth) / 2, 10 + kInventoryCategoryHeight, _inventoryCursor[1]);
		} else {
			drawSprite((kScreenWidth - kInventoryObjectWidth) / 2, 10 + kInventoryCategoryHeight, _inventoryCursor[0]);
		}
		if (tmpObj) {
			drawSprite((kScreenWidth - kInventoryObjectWidth) / 2, 25 + kInventoryCategoryHeight + 14 + kInventoryObjectHeight + 5, _inventoryCursor[3]);
		} else {
			drawSprite((kScreenWidth - kInventoryObjectWidth) / 2, 25 + kInventoryCategoryHeight + 14 + kInventoryObjectHeight + 5, _inventoryCursor[2]);
		}
		_inventoryVoicePlayed = false;
	} else {
		const int type = _inventoryCurrentObj->specialData[1][22];
		if (type != 32) {
			_render->setupProjection(kProjMenu);
			_render->setIgnoreDepth(false);
			SceneObject *so = &_sceneObjectsTable[0];
			_render->beginObjectDraw(so->x, so->y, so->z, so->pitch);
			drawSceneObjectMesh(so);
			_render->endObjectDraw();
			so->pitch += 16;
			so->pitch &= 1023;
			_render->setupProjection(kProj2D);
		}

		memset(&_drawCharBuf, 0, sizeof(_drawCharBuf));
		if (getMessage(_inventoryCurrentObj->objKey, 1, &_tmpMsg)) {
			const int len = strlen((const char *)_tmpMsg.data);
			const int x = (kScreenWidth - len * 8) / 2;
			drawString(x, 3, (const char *)_tmpMsg.data, _tmpMsg.font, 0);
		}
		if (type == 1) {
			if (getMessage(_inventoryCurrentObj->objKey, 3, &_tmpMsg) && _tmpMsg.data) {
				if (type != 64 && type != 512) {
					char buf[128];
					const int count = (_inventoryCurrentObj->customData[1] + 8) / 9;
					snprintf(buf, sizeof(buf), "%s %d", (const char *)_tmpMsg.data, count);
					drawString(1, 20, buf, _tmpMsg.font, 0);
				} else {
					drawString(1, 20, (const char *)_tmpMsg.data, _tmpMsg.font, 0);
				}
			}
			if (getMessage(_inventoryCurrentObj->objKey, 2, &_tmpMsg) && _tmpMsg.data) {
				char buf[128];
				snprintf(buf, sizeof(buf), "%s %d", (const char *)_tmpMsg.data, _inventoryCurrentObj->customData[0]);
				drawString(1, 30, buf, _tmpMsg.font, 0);
			}
		}
		if (type == 4) {
			if (getMessage(_inventoryCurrentObj->objKey, 2, &_tmpMsg) && _tmpMsg.data) {
				char buf[128];
				snprintf(buf, sizeof(buf), "%s %d", (const char *)_tmpMsg.data, _inventoryCurrentObj->customData[0]);
				drawString(1, 20, buf, _tmpMsg.font, 0);
			}
		}
		if (getMessage(_inventoryCurrentObj->objKey, 4, &_tmpMsg) && _tmpMsg.data) {
			if (type == 32) {
				if ((_tmpMsg.font & 0x80) != 0 && !_snd.isVoicePlaying(_inventoryCurrentObj->objKey) && !_inventoryVoicePlayed) {
					char buf[32];
					snprintf(buf, sizeof(buf), "%s_4", _inventoryCurrentObj->name);
					_snd.playVoice(_inventoryCurrentObj->objKey, getStringHash(buf));
					_inventoryVoicePlayed = true;
				}
				++_inventoryCurrentObj->specialData[1][9];
				int32_t argv[] = { _objectsPtrTable[kObjPtrWorld]->objKey, 6 };
				op_removeObjectMessage(2, argv);
				drawString(1, 20, (const char *)_tmpMsg.data, _tmpMsg.font, 0);
			} else if (type == 262144) {
				const int t = _ticks / (1000 / kTickDurationMs);
				char buf[128];
				snprintf(buf, sizeof(buf), "%s %02d:%02d:%02d", (const char *)_tmpMsg.data, t / 3600, (t % 3600) / 60, (t % 3600) % 60);
				drawString(1, 20, buf, _tmpMsg.font, 0);
			} else {
				drawString(1, 40, (const char *)_tmpMsg.data, _tmpMsg.font, 0);
			}
		}
	}
}
