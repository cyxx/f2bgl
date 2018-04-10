/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include "game.h"

void Game::initCabinet() {
	assert(_cabinetItemObj);
	setPalette(_palKeysTable[_cabinetItemObj->pal]);
	loadInventoryObjectMesh(_cabinetItemObj);
	initIcons(kIconModeCabinet);
}

void Game::finiCabinet() {
	// restore game mode icons
	if (_params.mouseMode || _params.touchMode) {
		initIcons(kIconModeGame);
	}
}

void Game::setCabinetItem(GameObject *o) {
	assert(o);
	_cabinetItemObj = o;
	setPalette(_palKeysTable[_cabinetItemObj->pal]);
	loadInventoryObjectMesh(_cabinetItemObj);
}

void Game::doCabinet() {
	_render->clearScreen();
	if (_cabinetItemCount != 0) {

		SceneObject *so = &_sceneObjectsTable[0];
		if (so->verticesCount != 0) {
			_render->setupProjection(kProjMenu);
			_render->setIgnoreDepth(false);
			_render->beginObjectDraw(so->x, so->y, so->z, so->pitch);
			drawSceneObjectMesh(so);
			_render->endObjectDraw();
			so->pitch += 6;
			so->pitch &= 1023;
		}

		_render->setupProjection(kProj2D);
		const int num = (_cabinetItemCount == 1) ? 28 : 29;
		if (getMessage(_objectsPtrTable[kObjPtrWorld]->objKey, num, &_tmpMsg)) {
			char buf[128];
			snprintf(buf, sizeof(buf), "%d %s", _cabinetItemCount, (const char *)_tmpMsg.data);
			memset(&_drawCharBuf, 0, sizeof(_drawCharBuf));
			int w, h;
			getStringRect(buf, kFontNameInvent, &w, &h);
			drawString((kScreenWidth - w) / 2, kScreenHeight - kScreenHeight / 10, buf, kFontNameInvent, 0);
		}

		if (getMessage(_cabinetItemObj->objKey, 1, &_tmpMsg)) {
			memset(&_drawCharBuf, 0, sizeof(_drawCharBuf));
			int w, h;
			getStringRect((const char *)_tmpMsg.data, _tmpMsg.font, &w, &h);
			drawString((kScreenWidth - w) / 2, 3, (const char *)_tmpMsg.data, _tmpMsg.font, 0);
		}
		drawIcons();
	}
	if (_cabinetItemCount > 1) {
		if (inp.dirMask & kInputDirLeft) {
			inp.dirMask &= ~kInputDirLeft;
			setCabinetItem(getPreviousObject(_cabinetItemObj));
		}
		if (inp.dirMask & kInputDirRight) {
			inp.dirMask &= ~kInputDirRight;
			setCabinetItem(getNextObject(_cabinetItemObj));
		}
		if (!inp.pointers[0][0].down && inp.pointers[0][1].down) {
			for (int j = 0; j < _iconsCount; ++j) {
				if (!_iconsTable[j].isCursorOver(inp.pointers[0][0].x, inp.pointers[0][0].y)) {
					continue;
				}

				switch (_iconsTable[j].action) {
				case kIconActionHand:
					inp.spaceKey = true;
					break;
				case kIconActionDirLeft:
					setCabinetItem(getPreviousObject(_cabinetItemObj));
					break;
				case kIconActionDirRight:
					setCabinetItem(getNextObject(_cabinetItemObj));
					break;
				}
			}
		}
	}
	if (inp.spaceKey) {
		inp.spaceKey = false;
		if (_cabinetItemCount > 0) {
			--_cabinetItemCount;
			GameObject *nextItemObj = getNextObject(_cabinetItemObj);
			GameObject *tmpObj = getObjectByKey(_cabinetItemObj->customData[0]);
			const int num = tmpObj->specialData[1][22];
			if (num == 2 || num == 8 || num == 16) {
				setObjectParent(_cabinetItemObj, tmpObj);
				if (!tmpObj->o_child) {
					switch (num) {
					case 2:
						_objectsPtrTable[kObjPtrUtil] = _objectsPtrTable[kObjPtrInventaire]->o_child->o_next->o_child;
						_varsTable[23] = _objectsPtrTable[kObjPtrUtil]->objKey;
						if (getMessage(_objectsPtrTable[kObjPtrUtil]->objKey, 0, &_tmpMsg) && _tmpMsg.data) {
							_objectsPtrTable[kObjPtrUtil]->text = (const char *)_tmpMsg.data;
						}
						break;
					case 8:
						_objectsPtrTable[kObjPtrShield] = _objectsPtrTable[kObjPtrInventaire]->o_child->o_next->o_next->o_next->o_child;
						_varsTable[25] = _objectsPtrTable[kObjPtrShield]->objKey;
						if (getMessage(_objectsPtrTable[kObjPtrShield]->objKey, 0, &_tmpMsg) && _tmpMsg.data) {
							_objectsPtrTable[kObjPtrShield]->text = (const char *)_tmpMsg.data;
						}
						break;
					case 16:
						_objectsPtrTable[kObjPtrScan] = _objectsPtrTable[kObjPtrInventaire]->o_child->o_next->o_next->o_next->o_next->o_child;
						_varsTable[24] = _objectsPtrTable[kObjPtrScan]->objKey;
						if (getMessage(_objectsPtrTable[kObjPtrScan]->objKey, 0, &_tmpMsg) && _tmpMsg.data) {
							_objectsPtrTable[kObjPtrScan]->text = (const char *)_tmpMsg.data;
						}
						break;
					}
				}
			} else if (num == 32) {
				setObjectParent(_cabinetItemObj, tmpObj);
			} else {
				if (tmpObj->o_parent->o_parent->specialData[1][21] != 128) {
					if (num == 8192 || num == 16384 || num == 32768 || num == 65536) {
						setObjectParent(tmpObj, getObjectByKey(tmpObj->customData[1]));
						_objectsPtrTable[kObjPtrProj] = _objectsPtrTable[kObjPtrInventaire]->o_child->o_next->o_next->o_child;
						_objectsPtrTable[kObjPtrProj] = tmpObj;
						if (_objectsPtrTable[kObjPtrProj]) {
							if (getMessage(_objectsPtrTable[kObjPtrProj]->objKey, 0, &_tmpMsg) && _tmpMsg.data) {
								_objectsPtrTable[kObjPtrProj]->text = (const char *)_tmpMsg.data;
							}
						}
					} else {
						setObjectParent(tmpObj, getObjectByKey(tmpObj->customData[3]));
						_objectsPtrTable[kObjPtrGun] = _objectsPtrTable[kObjPtrInventaire]->o_child->o_child;
						if (_objectsPtrTable[kObjPtrGun]) {
							_varsTable[15] = _objectsPtrTable[kObjPtrGun]->objKey;
							if (getMessage(_objectsPtrTable[kObjPtrGun]->objKey, 0, &_tmpMsg) && _tmpMsg.data) {
								_objectsPtrTable[kObjPtrGun]->text = (const char *)_tmpMsg.data;
							}
						}
					}
				}
				if (tmpObj->o_parent->specialData[1][22] == 1) {
					tmpObj->customData[1] += _cabinetItemObj->specialData[1][18];
				} else {
					tmpObj->customData[0] += _cabinetItemObj->specialData[1][18];
				}
				setObjectParent(_cabinetItemObj, _objectsPtrTable[kObjPtrCimetiere]);
			}
			setCabinetItem(nextItemObj);
			_snd.playSfx(_objectsPtrTable[kObjPtrWorld]->objKey, _res._sndKeysTable[8]);
		}
	}
	if (inp.escapeKey) {
		inp.escapeKey = false;
		_cabinetItemCount = 0;
	}
}
