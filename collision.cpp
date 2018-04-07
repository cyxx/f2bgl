/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include "game.h"

CollisionSlot *Game::createCollisionSlot(CollisionSlot *prev, CollisionSlot *next, GameObject *o, CellMap *cell) {
	CollisionSlot *colSlot = (CollisionSlot *)malloc(sizeof(CollisionSlot));
	if (colSlot) {
		colSlot->o = o;
		colSlot->prev = prev;
		colSlot->next = next;
		colSlot->cell = cell;
		colSlot->list = o->colSlot;
		o->colSlot = colSlot;
	}
	return colSlot;
}

void Game::destroyCollisionSlot(CellMap *cell) {
	CollisionSlot *colSlot = cell->colSlot;
	while (colSlot) {
		if (colSlot->o == _currentObject) {
			if (!colSlot->prev && !colSlot->next) {
				cell->colSlot = 0;
			} else {
				if (colSlot == cell->colSlot) {
					cell->colSlot = colSlot->next;
				}
				if (colSlot->prev) {
					colSlot->prev->next = colSlot->next;
				}
				if (colSlot->next) {
					colSlot->next->prev = colSlot->prev;
				}
			}
			CollisionSlot *next = colSlot->next;
			free(colSlot);
			_currentObject->colSlot = 0;
			colSlot = next;
		} else {
			colSlot = colSlot->next;
		}
	}
}

bool Game::collisionSlotCb1(CellMap *cell) {
	if (!cell->colSlot) {
		cell->colSlot = createCollisionSlot(0, 0, _currentObject, cell);
	} else {
		CollisionSlot *colSlotPrev = 0, *colSlot = cell->colSlot;
		while (colSlot) {
			colSlotPrev = colSlot;
			if (colSlot->o == _currentObject) {
				break;
			}
			colSlot = colSlot->next;
		}
		if (!colSlot) {
			colSlotPrev->next = createCollisionSlot(colSlotPrev, 0, _currentObject, cell);
		}
	}
	return true;
}

bool Game::collisionSlotCb2(CellMap *cell) {
	if (!cell->colSlot) {
		cell->colSlot = createCollisionSlot(0, 0, _currentObject, cell);
	} else {
		CollisionSlot *colSlotPrev = 0, *colSlot = cell->colSlot;
		while (colSlot) {
			colSlotPrev = colSlot;
			if (colSlot->o == _currentObject) {
				destroyCollisionSlot(cell);
				break;
			}
			colSlot = colSlot->next;
		}
		if (!colSlot) {
			colSlotPrev->next = createCollisionSlot(colSlotPrev, 0, _currentObject, cell);
		}
	}
	if (_currentObject->specialData[1][23] != 57) {
		for (CollisionSlot *slot = cell->colSlot; slot; slot = slot->next) {
			GameObject *o = slot->o;
			if (_currentObject == o) {
				continue;
			}
			if ((o->flags[1] & 0x20) != 0 && (o->specialData[1][8] & _currentObject->specialData[1][8]) != 0) {
				if (testCollisionSlotRect(_currentObject, o)) {
					if (o->specialData[1][8] != 1 && o->specialData[1][23] != 57) {
						sendMessage(57, _currentObject->objKey);
					}
					if ((o->flags[1] & 0x100) == 0 && (_currentObject->flags[1] & 0x80) == 0) {
						sendMessage(57, o->objKey);
					}
					if (o->state != 1) {
						if ((_currentObject->flags[1] & 0x80) == 0 || (o->flags[1] & 0x80) == 0) {
							o->state = 3;
							addToChangedObjects(o);
						}
					}
					if (o->specialData[1][8] == 1 && (o->flags[1] & 8) != 0 && o->specialData[1][23] != 57) {
						sendMessage(57, _currentObject->objKey);
					}
				}
			}
		}
	} else {
		for (CollisionSlot *slot = cell->colSlot; slot; slot = slot->next) {
			GameObject *o = slot->o;
			if (_currentObject == o) {
				continue;
			}
			if (o->specialData[1][21] != 8 && (o->specialData[1][8] & _currentObject->specialData[1][8]) != 0) {
				if (testCollisionSlotRect(_currentObject, o)) {
					GameObject *o_tmp = _currentObject;
					_currentObject = o;
					sendMessage(57, o_tmp->objKey);
					_currentObject = o_tmp;
				}
			}
		}
	}
	return true;
}

void Game::initCollisionSlot(GameObject *o) {
	setCollisionSlotsUsingCallback1(o->xPosParent + o->xPos, o->zPosParent + o->zPos, &Game::collisionSlotCb1);
}

void Game::resetCollisionSlot(GameObject *o) {
	CollisionSlot *colSlot = o->colSlot;
	while (colSlot) {
		if (!colSlot->prev && !colSlot->next) {
			colSlot->cell->colSlot = 0;
		} else {
			if (colSlot == colSlot->cell->colSlot) {
				colSlot->cell->colSlot = colSlot->next;
			}
			if (colSlot->prev) {
				colSlot->prev->next = colSlot->next;
			}
			if (colSlot->next) {
				colSlot->next->prev = colSlot->prev;
			}
		}
		CollisionSlot *next = colSlot->list;
		free(colSlot);
		colSlot = next;
	}
	o->colSlot = 0;
}

bool Game::setCollisionSlotsUsingCallback1(int x, int z, CollisionSlotCallbackType1 callback) {
	int box[65];
	if ((_currentObject->flags[1] & 0x100) == 0) {
		const int x1 = (x + _currentObject->xFrm1) >> 19;
		const int z1 = (z + _currentObject->zFrm1) >> 19;
		const int x2 = (x + _currentObject->xFrm2) >> 19;
		const int z2 = (z + _currentObject->zFrm2) >> 19;
		box[0] = -1;
		for (int zCell = z1; zCell <= z2; ++zCell) {
			for (int xCell = x1; xCell <= x2; ++xCell) {
				int mask = xCell | (zCell << 8);
				for (int i = 0; i < 64; ++i) {
					if (box[i] == -1) {
						box[i] = mask;
						box[i + 1] = -1;
						break;
					}
					if (box[i] == mask) {
						mask = -1;
						break;
					}
				}
				if (mask == -1) {
					continue;
				}
				if (xCell < 0 || xCell >= 64 || zCell < 0 || zCell >= 64 || !(this->*callback)(getCellMap(xCell, zCell))) {
					return false;
				}
			}
		}
	}
	return true;
}

bool Game::setCollisionSlotsUsingCallback2(GameObject *o, int x, int z, CollisionSlotCallbackType2 callback, uint32_t a, CollisionSlot2 *slot2) {
	static int hitsCount;
	if ((o->flags[1] & 0x100) == 0) {
		_varsTable[32] = 0;
		if (o->xFrm1 == 0 && o->zFrm1 == 0 && o->xFrm2 == 0 && o->zFrm2 == 0) {
			return true;
		}
		const int x1 = (x + o->xFrm1) >> 19;
		const int z1 = (z + o->zFrm1) >> 19;
		const int x2 = (x + o->xFrm2) >> 19;
		const int z2 = (z + o->zFrm2) >> 19;
		if (slot2[0].box == -1) {
			hitsCount = 0;
		} else {
			++hitsCount;
		}
		CellMap *cell;
		for (int zCell = z1; zCell <= z2; ++zCell) {
			for (int xCell = x1; xCell <= x2; ++xCell) {
				int i, mask = xCell | (zCell << 8);
				for (i = 0; i < 64; ++i) {
					if (slot2[i].box == -1) {
						if (xCell < 0 || xCell >= kMapSizeX || zCell < 0 || zCell >= kMapSizeZ) {
							return false;
						}
						slot2[i].box = mask;
						slot2[i].cell = cell = getCellMap(xCell, zCell);
						slot2[i].validObj = cell->colSlot && (cell->colSlot->o != o || cell->colSlot->next);
						slot2[i + 1].box = -1;
						break;
					}
					if (slot2[i].box == mask) {
						if (!slot2[i].validObj || slot2[i].hitsCount == hitsCount) {
							mask = -1;
						}
						break;
					}
				}
				if (mask != -1) {
					slot2[i].hitsCount = hitsCount;
					cell = slot2[i].cell;
					if ((o->flags[1] & 0x800) != 0 && cell->isDoor) {
						_updateGlobalPosRefObject = 0;
						_varsTable[32] = -1;
						return false;
					}
					if ((this->*callback)(o, cell, x, z, a)) {
						return false;
					}
				}
			}
		}
	}
	return true;
}

bool Game::setCollisionSlotsUsingCallback3(GameObject *o, int x, int z, CollisionSlotCallbackType3 callback, uint32_t a, int b, CollisionSlot2 *slot2) {
	static int hitsCount;
	if ((o->flags[1] & 0x100) == 0) {
		_varsTable[32] = 0;
		if (o->xFrm1 == 0 && o->zFrm1 == 0 && o->xFrm2 == 0 && o->zFrm2 == 0) {
			return true;
		}
		const int x1 = (x + o->xFrm1) >> 19;
		const int z1 = (z + o->zFrm1) >> 19;
		const int x2 = (x + o->xFrm2) >> 19;
		const int z2 = (z + o->zFrm2) >> 19;
		if (slot2[0].box == -1) {
			hitsCount = 0;
		} else {
			++hitsCount;
		}
		CellMap *cell;
		for (int zCell = z1; zCell <= z2; ++zCell) {
			for (int xCell = x1; xCell <= x2; ++xCell) {
				int i, mask = xCell | (zCell << 8);
				for (i = 0; i < 64; ++i) {
					if (slot2[i].box == -1) {
						if (xCell < 0 || xCell >= kMapSizeX || zCell < 0 || zCell >= kMapSizeZ) {
							return false;
						}
						slot2[i].box = mask;
						slot2[i].cell = cell = getCellMap(xCell, zCell);
						slot2[i].validObj = cell->colSlot && (cell->colSlot->o != _currentObject || cell->colSlot->next);
						slot2[i + 1].box = -1;
						break;
					}
					if (slot2[i].box == mask) {
						if (!slot2[i].validObj || slot2[i].hitsCount == hitsCount) {
							mask = -1;
						}
						break;
					}
				}
				if (mask != -1) {
					slot2[i].hitsCount = hitsCount;
					cell = slot2[i].cell;
					if ((o->flags[1] & 0x800) != 0 && cell->isDoor) {
						_updateGlobalPosRefObject = 0;
						return false;
					}
					if ((this->*callback)(o, cell, x, z, a, b)) {
						return false;
					}
				}
			}
		}
	}
	return true;
}

void Game::addObjectToDrawList(CellMap *cell) {
	for (CollisionSlot *colSlot = cell->colSlot; colSlot; colSlot = colSlot->next) {
		GameObject *o = colSlot->o;
		if (o->rayCastCounter == _rayCastCounter) {
			continue;
		}
		o->inSceneList = 1;
		if (o->flags[1] & 8) {
			GameObject *o_tmp = o;
			if (o->flags[1] & 0x100) {
				o_tmp = o->o_parent;
			} else if (o->flags[1] & 0x200) {
				o->state = 1;
			}
			if ((o->flags[1] & 0x80) == 0 && o->o_parent->specialData[1][21] != 32) {
				while (o_tmp->state == 1 && o_tmp != _objectsPtrTable[kObjPtrWorld]) {
					o_tmp = o_tmp->o_parent;
				}
				if (o_tmp == _objectsPtrTable[kObjPtrWorld]) {
					assert(_objectsDrawCount < kObjectsDrawListSize);
					_objectsDrawList[_objectsDrawCount] = o;
					++_objectsDrawCount;
				}
			}
		}
		o->rayCastCounter = _rayCastCounter;
	}
}

bool Game::testCollisionSlotRect(GameObject *o1, GameObject *o2) const {
	const int x1_1 = o1->xPosParent + o1->xPos + o1->xFrm1;
	const int x2_1 = o1->xPosParent + o1->xPos + o1->xFrm2;
	const int y1_1 = o1->zPosParent + o1->zPos + o1->zFrm1;
	const int y2_1 = o1->zPosParent + o1->zPos + o1->zFrm2;

	const int x1_2 = o2->xPosParent + o2->xPos + o2->xFrm1;
	const int x2_2 = o2->xPosParent + o2->xPos + o2->xFrm2;
	const int y1_2 = o2->zPosParent + o2->zPos + o2->zFrm1;
	const int y2_2 = o2->zPosParent + o2->zPos + o2->zFrm2;

	const int x1 = MAX(x1_1, x1_2);
	const int y1 = MAX(y1_1, y1_2);
	const int x2 = MIN(x2_1, x2_2);
	const int y2 = MIN(y2_1, y2_2);

	return (x2 > x1 && y2 > y1);
}

bool Game::testCollisionSlotRect2(GameObject *o1, GameObject *o2, int x, int z) const {
	const int x1_1 = x + o1->xFrm1;
	const int x2_1 = x + o1->xFrm2;
	const int y1_1 = z + o1->zFrm1;
	const int y2_1 = z + o1->zFrm2;

	const int x1_2 = o2->xPosParent + o2->xPos + o2->xFrm1;
	const int x2_2 = o2->xPosParent + o2->xPos + o2->xFrm2;
	const int y1_2 = o2->zPosParent + o2->zPos + o2->zFrm1;
	const int y2_2 = o2->zPosParent + o2->zPos + o2->zFrm2;

	const int x1 = MAX(x1_1, x1_2);
	const int y1 = MAX(y1_1, y1_2);
	const int x2 = MIN(x2_1, x2_2);
	const int y2 = MIN(y2_1, y2_2);

	return (x2 > x1 && y2 > y1);
}

static bool isObjectCollidable(GameObject *o, GameObject *o_tmp) {
	if ((o->flags[1] & 0x80) != 0 && (o_tmp->flags[1] & 0x80) != 0) {
		return false;
	}
	if (o->o_parent == o_tmp) {
		return false;
	}
	if ((o->flags[1] & 4) != 0 && (o_tmp->flags[1] & 0x80) != 0) {
		return false;
	}
	if ((o->flags[1] & 0x80) != 0 && (o_tmp->flags[1] & 4) != 0) {
		return false;
	}
	if (o->specialData[1][21] == 8 && o_tmp->specialData[1][21] == 8 && o_tmp->specialData[1][22] == 0x200) {
		return false;
	}
	if (o_tmp->specialData[1][21] == 8 && o->specialData[1][21] == 8 && o->specialData[1][22] == 0x200) {
		return false;
	}
	if (o_tmp->specialData[1][23] == 57) {
		return false;
	}
	return true;
}

bool Game::collisionSlotCb3(GameObject *o, CellMap *cell, int x, int z, uint32_t a) {
	_varsTable[21] = 0;
	_varsTable[32] = 0;
	if (cell->type != 0) {
		if ((cell->type == 32 && (o->flags[1] & 0x2000) == 0) || cell->type != 32) {
			if ((o->flags[1] & 0x84) == 0) {
				_updateGlobalPosRefObject = 0;
				_varsTable[21] = cell->type;
				_varsTable[32] = cell->isDoor ? 1 : 0;
				return true;
			}
		}
	}
	for (CollisionSlot *colSlot = cell->colSlot; colSlot; colSlot = colSlot->next) {
		GameObject *o_tmp = colSlot->o;
		if (!isObjectCollidable(o, o_tmp) || o_tmp == o) {
			continue;
		}
		if ((o_tmp->specialData[1][8] & (o->specialData[1][8] & a)) != 0) {
			if (testCollisionSlotRect2(o, o_tmp, x, z)) {
				_updateGlobalPosRefObject = o_tmp;
				return true;
			}
		}
	}
	return false;
}

bool Game::collisionSlotCb4(GameObject *o, CellMap *cell, int x, int z, uint32_t a, int b) {
	_varsTable[21] = 0;
	_varsTable[32] = 0;
	if (cell->type != 0 && cell->type != -3) {
		if ((cell->type == 32 && (o->flags[1] & 0x2000) == 0) || cell->type != 32) {
			if ((o->flags[1] & 0x80) == 0) {
				_updateGlobalPosRefObject = 0;
				_varsTable[21] = cell->type;
				return true;
			}
		}
	}
	for (CollisionSlot *colSlot = cell->colSlot; colSlot; colSlot = colSlot->next) {
		GameObject *o_tmp = colSlot->o;
		if (!isObjectCollidable(o, o_tmp) || o_tmp == o) {
			continue;
		}
		if ((o_tmp->specialData[1][21] & b) != 0 && (o_tmp->specialData[1][8] & (o->specialData[1][8] & a)) != 0) {
			if (testCollisionSlotRect2(o, o_tmp, x, z)) {
				_updateGlobalPosRefObject = o_tmp;
				return true;
			}
		}
	}
	return false;
}

void Game::updateCurrentObjectCollisions() {
	assert(_currentObject);
	GameObject *o_cur = _currentObject;
	GameObject *o = _currentObject;
	if (o->setColliding) {
		resetCollisionSlot(o);
		o->xPosParentPrev = o->xPosParent = o->o_parent->xPosParent + o->o_parent->xPos;
		o->zPosParentPrev = o->zPosParent = o->o_parent->zPosParent + o->o_parent->zPos;
		setCollisionSlotsUsingCallback1(o->xPosParent + o->xPos, o->zPosParent + o->zPos, &Game::collisionSlotCb2);
		const int x = o->xPosParent + o->xPos;
		const int z = o->zPosParent + o->zPos;
		int previousRoom = o->room;
		o->room = getCellMapShr19(x, z)->room;
		if (_cameraViewKey == o->objKey && previousRoom != o->room) {
			_roomsTable[o->room].fl = 1;
			playMusic(-1);
		}
		o->setColliding = false;
	}
	if ((o->o_child && o->state == 1) || (o->o_parent == o)) {
		_currentObject = o->o_child;
		updateCurrentObjectCollisions();
	}
	if (o->o_next) {
		_currentObject = o->o_next;
		updateCurrentObjectCollisions();
	}
	_currentObject = o_cur;
}

bool Game::collisionSlotCb5(GameObject *o, CellMap *map, int x, int z, uint32_t mask8) {
	_varsTable[21] = 0;
	_varsTable[32] = 0;
	if (map->type != 0) {
		if ((map->type == 32 && (o->flags[1] & 0x2000) == 0) || (map->type != 32)) {
			if (!(o->flags[1] & 0x80)) {
				_updateGlobalPosRefObject = 0;
				_varsTable[21] = map->type;
				return true;
			}
		}
	}
	CollisionSlot *slot = map->colSlot;
	while (slot) {
		GameObject *obj = slot->o;
		if (obj != o && ((obj->flags[1] & 0x100) != 0 || ((obj->flags[1] & 0x80) != 0 && obj->specialData[1][23] != 57) || (obj->specialData[1][21] == 32 && obj->specialData[1][22] == 1)) && (obj->specialData[1][8] & (o->specialData[1][8] & mask8))) {
			if (testCollisionSlotRect2(o, obj, x, z)) {
				_updateGlobalPosRefObject = obj;
				return true;
			}
		}
		slot = slot->next;
	}
	return false;
}

int Game::testObjectCollision2(GameObject *o, int dx1, int dz1, int dx2, int dz2) {
	CollisionSlot2 slots2[65];
	slots2[0].box = -1;
	dx1 <<= kPosShift;
	dx2 <<= kPosShift;
	dz1 <<= kPosShift;
	dz2 <<= kPosShift;
	o->xFrm1 += dx1;
	o->xFrm2 += dx2;
	o->zFrm1 += dz1;
	o->zFrm2 += dz2;
	const int x = o->xPosParent + o->xPos;
	const int z = o->zPosParent + o->zPos;
	const int ret = !setCollisionSlotsUsingCallback2(o, x, z, &Game::collisionSlotCb5, 0xFFFFFFFE, slots2) ? -1 : 0;
	o->xFrm1 -= dx1;
	o->xFrm2 -= dx2;
	o->zFrm1 -= dz1;
	o->zFrm2 -= dz2;
	return ret;
}

void Game::fixCoordinates(GameObject *o, int dx1, int dz1, int dx2, int dz2, int *fx, int *fz) {
        int t = 4;
	const int xCol = (o->xFrm2 + (dx2 << kPosShift)) - (o->xFrm1 + (dx1 << kPosShift)) + (t << kPosShift);
	const int zCol = (o->zFrm2 + (dz2 << kPosShift)) - (o->zFrm1 + (dz1 << kPosShift)) + (t << kPosShift);
	int x1Prev = o->xFrm1;
	int x2Prev = o->xFrm2;
	int z1Prev = o->zFrm1;
	int z2Prev = o->zFrm2;
	int wCol = 0;
	int eCol = 0;
	int sCol = 0;
	int nCol = 0;
	int loop;
	do {
		loop = 0;
		int dx = x2Prev - x1Prev;
		int dz = z2Prev - z1Prev;
		while (!loop && (dx < xCol || dz < zCol)) {
			int dxPrev = dx;
			int dzPrev = dz;
                        if (!wCol) {
                                o->xFrm1 -= (1 << kPosShift);
                                if (testObjectCollision2(o, 0, 0, 0, 0)) {
                                        o->xFrm1 += (1 << kPosShift);
                                        wCol = 1;
                                }
                        }
                        if (!eCol) {
                                o->xFrm2 += (1 << kPosShift);
                                if (testObjectCollision2(o, 0, 0, 0, 0)) {
                                        o->xFrm2 -= (1 << kPosShift);
                                        eCol = 1;
                                }
                        }
                        if (!sCol) {
                                o->zFrm1 -= (1 << kPosShift);
                                if (testObjectCollision2(o, 0, 0, 0, 0)) {
                                        o->zFrm1 += (1 << kPosShift);
                                        sCol = 1;
                                }
                        }
                        if (!nCol) {
                                o->zFrm2 += (1 << kPosShift);
                                if (testObjectCollision2(o, 0, 0, 0, 0)) {
                                        o->zFrm2 -= (1 << kPosShift);
                                        nCol = 1;
                                }
			}
                        dx = o->xFrm2 - o->xFrm1;
                        dz = o->zFrm2 - o->zFrm1;
			if ((dx == dxPrev && dx < xCol) || (dz == dzPrev && dz < zCol)) {
				--t;
				loop = 1;
			}
		}
	} while (loop && t > -1);
	*fx = ((*fx + o->xFrm1) + (*fx + o->xFrm2)) / 2;
	*fz = ((*fz + o->zFrm1) + (*fz + o->zFrm2)) / 2;
        o->xFrm1 = x1Prev;
        o->xFrm2 = x2Prev;
        o->zFrm1 = z1Prev;
        o->zFrm2 = z2Prev;
}

int Game::testObjectCollision1(GameObject *o, int xFrom, int zFrom, int xTo, int zTo, uint32_t mask8) {
	int xPrev = -1;
	int zPrev = -1;
	CollisionSlot2 slots2[65];
	slots2[0].box = -1;
	int ret = 0;
	static const int delta = kFollowingMargin << kPosShift;
	o->xFrm1 -= delta;
	o->xFrm2 += delta;
	o->zFrm1 -= delta;
	o->zFrm2 += delta;
	bool result = false;
	const int xDistance = xTo - xFrom;
	const int zDistance = zTo - zFrom;
	int xStep, zStep;
	int x, z, k, k2;
	if (xDistance != 0 || zDistance != 0) {
		k = (ABS(xDistance) >= ABS(zDistance)) ? (ABS(xDistance) >> kPosShift) + 1 : (ABS(zDistance) >> kPosShift) + 1;
		xStep = xDistance / k;
		zStep = zDistance / k;
		x = xFrom;
		z = zFrom;
		k2 = k >> 1;
	} else {
		k = 1;
		xStep = 0;
		zStep = 0;
		x = xFrom;
		z = zFrom;
		k2 = 0;
	}
	while (k--) {
		if (k == 0) {
			x = xTo;
			z = zTo;
		} else if (k == k2) {
			x = xFrom + (xDistance >> 1);
			z = zFrom + (zDistance >> 1);
		}
		if (x != xPrev || z != zPrev) {
			if (!(result = setCollisionSlotsUsingCallback2(o, x, z, &Game::collisionSlotCb5, mask8, slots2))) {
				ret = -1;
			}
		}
		xPrev = x;
		zPrev = z;
		x += xStep;
		z += zStep;
		if (ret == -1) {
			break;
		}
	}
	if (!result && (o->flags[1] & 0x800) != 0 && getCellMapShr19(x, z)->isDoor) {
		ret = -1;
	}
	o->xFrm1 += delta;
	o->xFrm2 -= delta;
	o->zFrm1 += delta;
	o->zFrm2 -= delta;
	return ret;
}

void Game::fixCoordinates2(GameObject *o_following, int x1, int z1, int *x2, int *z2, int *x3, int *z3) {
	const int dx = (*x2 - x1) + (*x2 - *x3);
	const int dz = (*z2 - z1) + (*z2 - *z3);
	const int xTo = (*x2 + dx) >> kPosShift;
	const int zTo = (*z2 + dz) >> kPosShift;
	int x = (*x2) >> kPosShift;
	int z = (*z2) >> kPosShift;
	const int xDistance = xTo - x;
	const int zDistance = zTo - z;
	int k = (ABS(xDistance) >= ABS(zDistance)) ? ABS(xDistance) + 1 : ABS(zDistance) + 1;
	const int xStep = xDistance / k;
	const int zStep = zDistance / k;
	while (k--) {
		static const uint32_t mask = 0xFFFFFFFE;
		if (!testObjectCollision1(o_following, x1, z1, x << kPosShift, z << kPosShift, mask) && !testObjectCollision1(o_following, x << kPosShift, z << kPosShift, *x3, *z3, mask)) {
			*x2 = x << kPosShift;
			*z2 = z << kPosShift;
			break;
		}
		x += xStep;
		z += zStep;
	}
}
