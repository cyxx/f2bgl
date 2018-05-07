/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include "game.h"
#include "trigo.h"

static int compareHelper(int op, int32_t value1, int32_t value2) {
	switch (op) {
	case 0:
		if (value1 == value2) {
			return -1;
		}
		break;
	case 1:
		if (value1 != value2) {
			return -1;
		}
		break;
	case 2:
		if (value1 < value2) {
			return -1;
		}
		break;
	case 3:
		if (value1 > value2) {
			return -1;
		}
		break;
	case 4:
		if (value1 <= value2) {
			return -1;
		}
		break;
	case 5:
		if (value1 >= value2) {
			return -1;
		}
		break;
	case 6:
		if (value1 & value2) {
			return -1;
		}
		break;
	default:
		error("compareHelper() unhandled operator %d", op);
		break;
	}
	return 0;
}

static int evalHelper(int op, int32_t value1, int32_t value2) {
	int ret = 0;
	switch (op) {
	case 0:
		ret = value1 + value2;
		break;
	case 1:
		ret = value1 - value2;
		break;
	case 2:
		ret = value1 * value2;
		break;
	case 3:
		if (value2 != 0) {
			ret = value1 / value2;
		}
		break;
	case 4:
		ret = value1 >> value2;
		break;
	case 5:
		ret = value1 << value2;
		break;
	case 6:
		ret = value1 | value2;
		break;
	case 7:
		ret = fixedSqrt(value1 * value1 + value2 * value2);
		break;
	case 8:
		ret = value1 & value2;
		break;
	case 9:
		ret = value1;
		if (ret < 0) {
			ret -= value2;
		} else {
			ret += value2;
		}
		break;
	default:
		error("evalHelper() unhandled operator %d", op);
		break;
	}
	return ret;
}

int Game::op_true(int argc, int32_t *argv) {
	assert(argc == 0);
	debug(kDebug_OPCODES, "Game::op_true() []");
	return -1;
}

int Game::op_toggleInput(int argc, int32_t *argv) {
	assert(argc == 2);
	debug(kDebug_OPCODES, "Game::op_toggleInput() [%d, %d]", argv[0], argv[1]);
	const uint32_t mask = argv[0];
	const int32_t input = argv[1];
	assert(input < _inputsCount);
	if ((_inputsTable[input].keymask & mask) == mask) {
		if ((_inputsTable[input].keymaskToggle & mask) == 0) {
			_inputsTable[input].keymaskToggle |= mask;
			return -1;
		}
	}
	return 0;
}

int Game::op_compareCamera(int argc, int32_t *argv) {
	assert(argc == 3);
	debug(kDebug_OPCODES, "Game::op_compareCamera() [%d, %d, %d]", argv[0], argv[1], argv[2]);
	const int param = argv[0];
	int value = 0;
	switch (param) {
	case 0:
		value = _cameraState;
		break;
	case 1:
		value = _cameraDeltaRotY;
		break;
	case 2:
		value = _cameraDistValue >> 15;
		break;
	case 3:
		value = _yPosObserverValue >> 15;
		break;
	default:
		error("Game::op_compareCamera() unhandled param %d", param);
		break;
	}
	return compareHelper(argv[1], value, argv[2]);
}


int Game::op_sendMessage(int argc, int32_t *argv) {
	assert(argc == 2);
	debug(kDebug_OPCODES, "Game::op_sendMessage() [%d, %d]", argv[0], argv[1]);
	return sendMessage(argv[0], argv[1]) ? -1 : 0;
}

int Game::op_getObjectMessage(int argc, int32_t *argv) {
	assert(argc == 2);
	debug(kDebug_OPCODES, "Game::op_getObjectMessage() [%d, %d]", argv[0], argv[1]);
	int32_t num = argv[0];
	int32_t objKey = argv[1];
	GameMessage *msg = _currentObject->msg;
	if (objKey == 0) {
		objKey = _currentObject->objKey;
	}
	while (msg) {
		if (objKey != -1) {
			if (msg->objKey == objKey && msg->num == num) {
				_varsTable[kVarMessageObjectKey] = msg->objKey;
				return -1;
			}
		} else {
			if (msg->num == num) {
				_varsTable[kVarMessageObjectKey] = msg->objKey;
				return -1;
			}
		}
		msg = msg->next;
	}
	return 0;
}

int Game::op_setParticleParams(int argc, int32_t *argv) {
	assert(argc == 6);
	debug(kDebug_OPCODES, "Game::op_setParticleParams() [%d, %d, %d, %d, %d, %d]", argv[0], argv[1], argv[2], argv[3], argv[4], argv[5]);
	int32_t objKey = argv[0];
	GameObject *o = (objKey == 0) ? _currentObject : getObjectByKey(objKey);
	if (!o) {
		return 0;
	}
	Vec_xz vec(argv[1], argv[3]);
	vec.rotate(o->pitch, kPosShift);
	const int dx = vec.x;
	const int dz = vec.z;
	const int dy = argv[2];
	_particleDx = o->xPos + dx;
	_particleDy = o->yPos + dy;
	_particleDz = o->zPos + dz;
	_particleRnd = argv[4];
	_particleSpd = argv[5];
	return -1;
}

int Game::op_setVar(int argc, int32_t *argv) {
	assert(argc == 2);
	debug(kDebug_OPCODES, "Game::op_setVar() [%d, %d]", argv[0], argv[1]);
	int32_t var = argv[0];
	int32_t value = argv[1];
	if (var == 137) {
		if (value == 1 || value == 8 || value == 7 || value == 6 || value == 5) {
			if (!_conradUsingGun) {
				if ((_cameraUsingGunDist & 1) != (_cameraDefaultDist & 1)) {
					fixCamera();
				}
				_cameraState = 0;
				_conradUsingGun = true;
			}
		} else {
			if (_conradUsingGun) {
				if ((_cameraStandingDist & 1) != (_cameraDefaultDist & 1)) {
					fixCamera();
				}
				_conradUsingGun = false;
			}
		}
	}
	assert(var >= 128);
	_varsTable[var - 128] = value;
	return -1;
}

int Game::op_compareConst(int argc, int32_t *argv) {
	assert(argc == 3);
	debug(kDebug_OPCODES, "Game::op_compareConst() [%d, %d, %d]", argv[0], argv[1], argv[2]);
	return compareHelper(argv[1], argv[0], argv[2]);
}

int Game::op_evalVar(int argc, int32_t *argv) {
	assert(argc == 3);
	debug(kDebug_OPCODES, "Game::op_evalVar() [%d, %d, %d]", argv[0], argv[1], argv[2]);
	_varsTable[10] = evalHelper(argv[1], argv[0], argv[2]);
	return -1;
}

int Game::op_playSound(int argc, int32_t *argv) {
	assert(argc == 1 || argc == 2);
	debug(kDebug_OPCODES, "Game::op_playSound() [%d, %d]", argv[0], argv[1]);
	int32_t key = argv[0];
	uint32_t flags = (argc == 1) ? 0 : argv[1];
	GameObject *o = _currentObject;
	GameObject *o_conrad = _objectsPtrTable[kObjPtrConrad];
	const int dx = ((o->xPos + o->xPosParent) >> 15) - (_xPosObserver >> 15);
	const int dz = ((o->zPos + o->zPosParent) >> 15) - (_zPosObserver >> 15);
	int distance = fixedSqrt(dx * dx + dz * dz) >> 1;
	if (flags & 0x8000) {
		distance >>= 1;
	}
	if (distance > 127) {
		distance = 127;
	}
	int volume = 127 - distance;
	if (o->room != o_conrad->room) {
		volume >>= 1;
	}
	int pan = 64;
	if (flags & 0x8000) {

		// flags &= 0x7FFFF;

		// The flags are not correctly masked in the original engine.
		// eg. masking 0x7FFFF instead of 0x7FFF. The sound system uses
		// bits 1 and 2, so this was clearly an typo.
		//
		//  cseg01:00038F01   mov     cl, byte ptr [ebp+flags+1]
		//  cseg01:00038F04   test    cl, 80h
		//  cseg01:00038F07   jz      short loc_38F1F
		//  cseg01:00038F09   mov     edx, [ebp+flags+2]
		//  cseg01:00038F0C   xor     dh, dh
		//  cseg01:00038F0E   and     dl, 7
		//  cseg01:00038F16   mov     word ptr [ebp+flags+2], dx
		//

		flags &= 0x7FFF;

	} else {
		int diffangle = getAngleFromPos(o->xPosParent + o->xPos - _xPosObserver, o->zPosParent + o->zPos - _zPosObserver);
		diffangle = (diffangle - (-_yRotObserver & 1023)) & 1023;
		if (diffangle > 512) {
			diffangle -= 1024;
		}
		if (diffangle > 0) {
			if (diffangle > 256) {
				diffangle += 2 * (256 - diffangle);
			}
			diffangle--;
		} else {
			if (diffangle < -256) {
				diffangle += 2 * (-256 - diffangle);
			}
		}
		pan -= (diffangle >> 3);
		if (pan < 0) {
			pan = 0;
		} else if (pan > 127) {
			pan = 127;
		}
		pan = 127 - pan;
	}
	if (flags & 0x4000) {
		if (_snd._musicMode != 3) {
			_objectsPtrTable[kObjPtrMusic] = o;
			playMusic(3);
		}
	} else {
		_snd.setVolume(volume);
		_snd.setPan(pan);
		if (key <= 0) {
			if (_currentObject == _objectsPtrTable[kObjPtrTarget]) {
				_snd.setVolume(127);
			}
			key = READ_LE_UINT16(o->anim.aniheadData + 6);
			if (key <= 0) {
				return -1;
			}
		}
		_snd.playSfx(o->objKey, key, (flags & 2) != 0);
	}
	return -1;
}

int Game::op_getAngle(int argc, int32_t *argv) {
	assert(argc == 2);
	debug(kDebug_OPCODES, "Game::op_getAngle() [%d, %d]", argv[0], argv[1]);
	int32_t obj1Key = argv[0];
	int32_t obj2Key = argv[1];
	GameObject *o1 = (obj1Key == 0) ? _currentObject : getObjectByKey(obj1Key);
	if (!o1) {
		return 0;
	}
	GameObject *o2 = (obj2Key == 0) ? _currentObject : getObjectByKey(obj2Key);
	if (!o2) {
		return 0;
	}
	const int x1 = o1->xPosParent + o1->xPos;
	const int z1 = o1->zPosParent + o1->zPos;
	const int x2 = o2->xPosParent + o2->xPos;
	const int z2 = o2->zPosParent + o2->zPos;
	const int segmentAngle = getAngleFromPos(x2 - x1, z2 - z1);
	_varsTable[8] = getAngleDiff(segmentAngle, o1->pitch & 1023);
	return -1;
}

int Game::op_setObjectData(int argc, int32_t *argv) {
	assert(argc == 3);
	debug(kDebug_OPCODES, "Game::op_setObjectData() [%d, %d, %d]", argv[0], argv[1], argv[2]);
	int32_t objKey = argv[0];
	GameObject *o = (objKey == 0) ? _currentObject : getObjectByKey(objKey);
	if (!o) {
		return 0;
	}
	setObjectData(o, argv[1], argv[2]);
	return -1;
}

int Game::op_evalObjectData(int argc, int32_t *argv) {
	assert(argc == 4);
	debug(kDebug_OPCODES, "Game::op_evalObjectData() [%d, %d, %d, %d]", argv[0], argv[1], argv[2], argv[3]);
	int32_t objKey = argv[0];
	uint32_t param = argv[1];
	uint32_t op = argv[2];
	int32_t value = argv[3];
	GameObject *o = (objKey == 0) ? _currentObject : getObjectByKey(objKey);
	if (!o) {
		return 0;
	}
	int *operand = 0;
	if (param >= 256) {
		switch (param) {
		case 257:
			if (objKey != 0) {
				if (!o->updateColliding && !o->setColliding) {
					addToCollidingObjects(o);
				}
			}
			operand = &o->xPos;
			value <<= 15;
			break;
		case 258:
			if (objKey != 0) {
				if (!o->updateColliding && !o->setColliding) {
					addToCollidingObjects(o);
				}
			}
			operand = &o->yPos;
			value <<= 15;
			break;
		case 259:
			if (objKey != 0) {
				if (!o->updateColliding && !o->setColliding) {
					addToCollidingObjects(o);
				}
			}
			operand = &o->zPos;
			value <<= 15;
			break;
		case 274:
			if (objKey != 0) {
				if (!o->updateColliding && !o->setColliding) {
					addToCollidingObjects(o);
				}
			}
			operand = &o->xPos;
			break;
		case 277:
			if (objKey != 0) {
				if (!o->updateColliding && !o->setColliding) {
					addToCollidingObjects(o);
				}
			}
			operand = &o->yPos;
			break;
		case 275:
			if (objKey != 0) {
				if (!o->updateColliding && !o->setColliding) {
					addToCollidingObjects(o);
				}
			}
			operand = &o->zPos;
			break;
		case 268:
			operand = &o->pitch;
			break;
		default:
			error("Game::op_evalObjectData() unhandled param %d", param);
			break;
		}
		*operand = evalHelper(op, *operand, value);
		if (param == 268) {
			*operand &= 1023;
		}
	} else {
		const int result = evalHelper(op, o->getData(param), value);
		o->setData(param, result);
	}
	return -1;
}

int Game::op_compareObjectData(int argc, int32_t *argv) {
	assert(argc == 4);
	debug(kDebug_OPCODES, "Game::op_compareObjectData() [%d, %d, %d, %d]", argv[0], argv[1], argv[2], argv[3]);
	int32_t objKey = argv[0];
	int32_t op = argv[2];
	GameObject *o = (objKey == 0) ? _currentObject : getObjectByKey(objKey);
	if (!o) {
		return 0;
	}
	if (argv[1] == 260) {
		const int x = o->xPosParent + o->xPos;
		const int z = o->zPosParent + o->zPos;
		const CellMap *cell = getCellMapShr19(x, z);
		o = getObjectByKey(argv[3]);
		switch (op) {
		case 0:
			if (cell->room == o->room || cell->room2 == o->room) {
				return -1;
			}
			break;
		case 1:
			if (cell->room != o->room && cell->room2 != o->room) {
				return -1;
			}
			break;
		default:
			error("Game::op_compareObjectData() invalid compare op for param 260");
			break;
		}
	} else {
		int value;
		if (argv[1] == 278) {
			const int x = o->xPosParent + o->xPos;
			const int z = o->zPosParent + o->zPos;
			value = getCellMapShr19(x, z)->room2;
		} else {
			value = getObjectData(o, argv[1]);
		}
		int result = compareHelper(op, value, argv[3]);
		return result;
	}
	return 0;
}

int Game::op_jumpToNextLevel(int argc, int32_t *argv) {
	assert(argc == 0);
	debug(kDebug_OPCODES, "Game::op_jumpToNextLevel() []");
	++_level;
	_changeLevel = true;
	int32_t target = -1;
	op_clearTarget(1, &target);
	return -1;
}

int Game::op_rand(int argc, int32_t *argv) {
	assert(argc == 1);
	debug(kDebug_OPCODES, "Game::op_rand() [%d]", argv[0]);
	const int32_t div = argv[0];
	return (div <= 0 || _rnd.getRandomNumber() <= 8192 / div) ? -1 : 0;
}

int Game::op_isObjectColliding(int argc, int32_t *argv) {
	assert(argc == 4);
	debug(kDebug_OPCODES, "Game::op_isObjectColliding() [%d, %d, %d, %d]", argv[0], argv[1], argv[2], argv[3]);
	int32_t objKey = argv[0];
	int32_t distance = argv[1];
	uint32_t mask = argv[2];
	int32_t dy = argv[3];
	GameObject *o = (objKey == 0) ? _currentObject : getObjectByKey(objKey);
	if (!o) {
		return 0;
	}
	int pitchTable[3] = { 0, 0, 0 };
	int pitch = 2;
	bool isPlayerObject = false;
	if ((o->flags[1] & 0x10000) != 0 && _varsTable[kVarPlayerObject] == o->objKey) {
		isPlayerObject = true;
		if (distance != 0) {
			pitchTable[0] = 0;
			pitchTable[1] = 48;
			pitchTable[2] = -48;
			pitch = 0;
		}
	}
	int ret = 0;
	CollisionSlot2 slots2[65];
	bool collidingTest = false;
	int collidingTest2 = 0;
	int xPosStart, zPosStart, xPos, zPos;
	do {
		slots2[0].box = -1;
		int xPosPrev = -1;
		int zPosPrev = -1;
		ret = 0;
		const int a = (o->pitch + dy + pitchTable[pitch]) & 1023;
		int cosy = g_cos[a];
		int siny = g_sin[a];
		int xDistance = siny * distance;
		int zDistance = cosy * distance;
		int stepCount, stepCount2;
		int dxStep, dzStep;
		if (xDistance != 0 || zDistance != 0) {
			if (ABS(xDistance) >= ABS(zDistance)) {
				stepCount = (ABS(xDistance) >> kPosShift) + 1;
			} else {
				stepCount = (ABS(zDistance) >> kPosShift) + 1;
			}
			dxStep = xDistance / stepCount;
			dzStep = zDistance / stepCount;
			stepCount2 = stepCount >> 1;
		} else {
			stepCount = 1;
			dxStep = 0;
			dzStep = 0;
			stepCount2 = 0;
		}
		xPosStart = xPos = o->xPosParent + o->xPos;
		zPosStart = zPos = o->zPosParent + o->zPos;
		while (stepCount--) {
			if (stepCount == 0) {
				xPos = xPosStart + xDistance;
				zPos = zPosStart + zDistance;
			} else if (stepCount == stepCount2) {
				xPos = xPosStart + (xDistance >> 1);
				zPos = zPosStart + (zDistance >> 1);
			}
			if (xPos != xPosPrev || zPos != zPosPrev) {
				collidingTest = setCollisionSlotsUsingCallback2(o, xPos, zPos, &Game::collisionSlotCb3, mask, slots2);
				if (!collidingTest) {
					if (isPlayerObject) {
						collidingTest2 |= _varsTable[32];
					}
					ret = -1;
					if (_updateGlobalPosRefObject && (_updateGlobalPosRefObject->flags[1] & 0x100) == 0) {
						_varsTable[14] = _updateGlobalPosRefObject->objKey;
					} else {
						_varsTable[14] = 0;
						_varsTable[6] = xPos;
						_varsTable[7] = zPos;
					}
				}
			}
			xPosPrev = xPos;
			zPosPrev = zPos;
			xPos += dxStep;
			zPos += dzStep;
			if (ret == -1) {
				break;
			}
		}
		++pitch;
	} while (!collidingTest && pitch < 3);
	if (collidingTest) {
		if (isPlayerObject && collidingTest2 == 0) {
			o->pitch += pitchTable[pitch - 1];
		}
	}
	if (!collidingTest && (o->flags[1] & 0x800) != 0 && getCellMapShr19(xPos, zPos)->isDoor) {
		ret = -1;
	}
	return ret;
}

int Game::op_sendShootMessage(int argc, int32_t *argv) {
	assert(argc == 1);
	debug(kDebug_OPCODES, "Game::op_sendShootMessage() [%d]", argv[0]);
	int32_t radius = argv[0];
	int xPos = _currentObject->xPos;
	int zPos = _currentObject->zPos;
	int num = _currentObject->specialData[1][18];
	GameObject *o = _objectsPtrTable[kObjPtrConrad];
	sendShootMessageHelper(o, xPos, zPos, radius, num);
	if (_varsTable[kVarPlayerObject] > 0) {
		o = getObjectByKey(_varsTable[kVarPlayerObject]);
		sendShootMessageHelper(o, xPos, zPos, radius, num);
	}
	o = getRoomObject(xPos, zPos);
	if (o) {
		for (o = o->o_child; o && (o->flags[1] & 0x100) == 0; o = o->o_next) {
			if (o != _currentObject) {
				sendShootMessageHelper(o, xPos, zPos, radius, num);
			}
		}
	}
	return -1;
}

int Game::op_getShootInfo(int argc, int32_t *argv) {
	assert(argc == 5);
	debug(kDebug_OPCODES, "Game::op_getShootInfo() [%d, %d, %d, %d, %d]", argv[0], argv[1], argv[2], argv[3], argv[4]);
	int32_t objKey = argv[0];
	int32_t param = argv[1];
	int32_t ticksStart = argv[2];
	int32_t ticksEnd = argv[3];
	int32_t mode = argv[4];
	GameObject *o = (objKey == 0) ? _currentObject : getObjectByKey(objKey);
	if (!o || !o->o_child) {
		return 0;
	}
	if (o == _objectsPtrTable[kObjPtrGun]) {
		switch (_objectsPtrTable[kObjPtrGun]->specialData[1][22]) {
		case 64:
			break;
		case 4096:
			break;
		case 512:
			if (_gunTicksTable[1] + 4 <= _ticks) {
				_gunTicksTable[1] = _ticks;
			} else {
				return 0;
			}
			break;
		case 256:
			if (_gunTicksTable[2] + 6 <= _ticks) {
				_gunTicksTable[2] = _ticks;
			} else {
				return 0;
			}
			break;
		case 128:
			if (_gunTicksTable[3] + 6 <= _ticks && _varsTable[16] != 0) {
				_gunTicksTable[3] = _ticks;
			} else {
				return 0;
			}
			break;
		case 1024:
			if (_gunTicksTable[4] + 6 <= _ticks && _varsTable[16] != 0) {
				_gunTicksTable[4] = _ticks;
			} else {
				return 0;
			}
			break;
		case 2048:
			if (_gunTicksTable[5] + 6 <= _ticks) {
				_gunTicksTable[5] = _ticks;
			} else {
				return 0;
			}
			break;
		default:
			error("Game::op_getShootInfo() unhandled mode %d", _objectsPtrTable[kObjPtrGun]->specialData[1][22]);
			break;
		}
	} else if (mode == 0) {
	} else {
		const int t1 = _currentObject->specialData[1][24];
		const int t2 = _currentObject->specialData[1][25];
		if (t1 + ticksStart <= _ticks && _varsTable[12] == _currentObject->objKey && t2 >= t1 && t2 + ticksEnd <= _ticks) {
			_currentObject->specialData[1][24] = _ticks;
			_varsTable[13] = 1;
		} else {
			return 0;
		}
	}
	GameObject *o_shoot = o->o_child;
	_currentObject->setData(param, o_shoot->objKey);
	o_shoot->specialData[1][9] = _currentObject->objKey;
	o_shoot->specialData[1][10] = 0x400013;
	setObjectParent(o_shoot, _objectsPtrTable[kObjPtrWorld]);
	_varsTable[29] = _currentObject->objKey;
	return -1;
}

int Game::op_updateTarget(int argc, int32_t *argv) {
	assert(argc == 6);
	debug(kDebug_OPCODES, "Game::op_updateTarget() [%d, %d, %d, %d, %d, %d]", argv[0], argv[1], argv[2], argv[3], argv[4], argv[5]);
	GameObject *o = 0;
	int32_t objKey = argv[0];
	int32_t paramDist  = (argv[1] > 64) ? (argv[1] - 64) : argv[1];
	int32_t paramAngle = (argv[2] > 64) ? (argv[2] - 64) : argv[2];
	int32_t paramX     = (argv[3] > 64) ? (argv[3] - 64) : argv[3];
	int32_t paramY     = (argv[4] > 64) ? (argv[4] - 64) : argv[4];
	int32_t paramZ     = (argv[5] > 64) ? (argv[5] - 64) : argv[5];
	if (objKey == 0) {
		_currentObject->specialData[1][paramY] = 0;
	} else if ((o = getObjectByKey(objKey)) == 0) {
		return 0;
	}
	if (_currentObject->specialData[1][paramY] != 0) {
		int xt, yt, zt, xo, yo, zo;
		if (getShootPos(objKey, &xt, &yt, &zt) == 0) {
			xt = o->xPosParent + o->xPos;
			yt = o->yPosParent + o->yPos;
			zt = o->zPosParent + o->zPos;
		}
		xo = _currentObject->xPosParent + _currentObject->xPos;
		yo = _currentObject->yPosParent + _currentObject->yPos;
		zo = _currentObject->zPosParent + _currentObject->zPos;
		if (_currentObject->specialData[1][paramY] & 2) {
			const int dist = _currentObject->specialData[1][paramDist];
			int dxt_o = xt - xo;
			int dyt_o = yt - yo;
			int dzt_o = zt - zo;
			int xz = getAngleFromPos(xt - xo, zt - zo);
			xz = (xz - (_currentObject->pitch & 1023)) & 1023;
			xz = (1024 - xz) & 1023;
			if (xz > 768) {
				xz -= 768;
			} else {
				xz += 256;
			}
			Vec_xz vec(dxt_o, dzt_o);
			vec.rotate(xz, 15);
			dxt_o = vec.x;
			dzt_o = vec.z;
			if (dzt_o < 0) {
				dzt_o = -dzt_o;
			}
			if ((dzt_o >> 15) != 0) {
				int yz = ((dyt_o << 8) / dzt_o) << 7;
				if (yz > 1747) {
					yz = 1747;
					dyt_o = fixedMul(yz, dzt_o, 15);
				} else if (yz < -1747) {
					yz = -1747;
					dyt_o = fixedMul(yz, dzt_o, 15);
				}
				const int dy = fixedMul(dist << 15, dyt_o, 15) / (dzt_o >> 15);
				yo += dy;
                        }
			_currentObject->yPos = yo - _currentObject->yPosParent;
			int32_t args[2] = { 1, 1 };
			op_updateCollidingHorizontalMask(2, args);
		}
		if (_currentObject->specialData[1][paramY] & 1) {
			int xz = getAngleFromPos(xt - xo, zt - zo) - _currentObject->pitch;
			while (xz > 512) {
				xz -= 1024;
			}
			while (xz < -512) {
				xz += 1024;
			}
			if (xz > 0 && xz <= 512) {
				if (xz > _currentObject->specialData[1][paramAngle]) {
					xz = _currentObject->specialData[1][paramAngle];
				}
			} else {
				if (xz < -_currentObject->specialData[1][paramAngle]) {
					xz = -_currentObject->specialData[1][paramAngle];
				}
			}
			_currentObject->specialData[1][paramX] = (xz * _currentObject->specialData[1][paramAngle] > 0) ? _currentObject->specialData[1][paramAngle] : -_currentObject->specialData[1][paramAngle];
			_currentObject->specialData[1][paramAngle] = xz;
                } else {
                        _currentObject->specialData[1][paramAngle] = 0;
                        _currentObject->specialData[1][paramX] = 0;
                }
		_currentObject->specialData[1][paramY] = 0;
		_currentObject->specialData[1][paramZ] = 0;
	} else {
		_currentObject->specialData[1][paramAngle] = 0;
		_currentObject->specialData[1][paramX] = 0;
	}
	return -1;
}

int Game::op_transformObjectPos(int argc, int32_t *argv) {
	assert(argc == 2);
	debug(kDebug_OPCODES, "Game::op_transformObjectPos() [%d, %d]", argv[0], argv[1]);
	const int32_t obj1Key = argv[0];
	const int32_t obj2Key = argv[1];
	GameObject *o1 = (obj1Key == 0) ? _currentObject : getObjectByKey(obj1Key);
	if (!o1) {
		return 0;
	}
	GameObject *o2 = (obj2Key == 0) ? _currentObject : getObjectByKey(obj2Key);
	if (!o2) {
		return 0;
	}
	const int r = (-(o1->pitch & 1023)) & 1023;
	int cosy = g_cos[r];
	int siny = g_sin[r];
	const int xR = (o2->xPos - (o1->xPosParent + o1->xPos)) >> kPosShift;
	const int zR = (o2->zPos - (o1->zPosParent + o1->zPos)) >> kPosShift;
	const int xT = (cosy * xR + siny * zR) >> kPosShift;
	const int zT = (cosy * zR - siny * xR) >> kPosShift;
	const int div = 16 + zT;
	if (div > 0) {
		_varsTable[0] = (xT << 8) / div;
	} else if (div < 0) {
		_varsTable[0] = (-xT << 8) / div;
	} else {
		_varsTable[0] = 0;
	}
	_varsTable[1] = zT;
	return -1;
}

int Game::op_compareObjectAngle(int argc, int32_t *argv) {
	assert(argc == 4);
	debug(kDebug_OPCODES, "Game::op_compareObjectAngle() [%d, %d, %d, %d]", argv[0], argv[1], argv[2], argv[3]);
	const int objKey = argv[0];
	assert(argv[1] == 268);
	const int angle = argv[2];
	const int delta = argv[3];
	GameObject *o = (objKey == 0) ? _currentObject : getObjectByKey(objKey);
	if (!o) {
		return 0;
	}
	o->pitch &= 1023;
	const int lA = (angle - delta) & 1023;
	const int rA = (angle + delta) & 1023;
	if (lA < rA) {
		return (o->pitch >= lA && o->pitch <= rA) ? -1 : 0;
	} else {
		return (o->pitch >= lA || o->pitch <= rA) ? -1 : 0;
	}
}

int Game::op_moveObjectToObject(int argc, int32_t *argv) {
	assert(argc == 1);
	debug(kDebug_OPCODES, "Game::op_moveObjectToObject() [%d]", argv[0]);
	int32_t objKey = argv[0];
	GameObject *o = (objKey == 0) ? _currentObject : getObjectByKey(objKey);
	if (!o) {
		return 0;
	}
	const uint8_t *p_anikeyf = o->anim.anikeyfData;
	if (!p_anikeyf && o->anim.currentAnimKey != 0) {
		p_anikeyf = _res.getData(kResType_ANI, o->anim.currentAnimKey, "ANIKEYF");
	}
	if (!p_anikeyf) {
		warning("Game::op_moveObjectToObject() no anim key %d", o->anim.currentAnimKey);
		return 0;
	}
	int x = (((int8_t)p_anikeyf[16]) * 8 + (int16_t)READ_LE_UINT16(p_anikeyf + 2)) << 10;
	int y = ((int16_t)READ_LE_UINT16(p_anikeyf + 14)) << 11;
	int z = (((int8_t)p_anikeyf[17]) * 8 + (int16_t)READ_LE_UINT16(p_anikeyf + 6)) << 10;
	Vec_xz vec(x, z);
	vec.rotate(o->pitch & 1023);
	_currentObject->xPos = o->xPosParent + o->xPos + vec.x;
	_currentObject->yPos = o->yPosParent + o->yPos + y;
	_currentObject->zPos = o->zPosParent + o->zPos + vec.z;
	return -1;
}

int Game::op_getProjObject(int argc, int32_t *argv) {
	assert(argc == 1);
	debug(kDebug_OPCODES, "Game::op_getProjObject() [%d]", argv[0]);
	int param = argv[0];
	GameObject *o = _objectsPtrTable[kObjPtrProj]->o_child;
	if (!o) {
		return 0;
	}
	_currentObject->setData(param, o->objKey);
	setObjectParent(o, _objectsPtrTable[kObjPtrWorld]);
	return -1;
}

int Game::op_setObjectParent(int argc, int32_t *argv) {
	assert(argc == 2);
	debug(kDebug_OPCODES, "Game::op_setObjectParent() [%d, %d]", argv[0], argv[1]);
	int32_t objKey = argv[0];
	int32_t parentKey = argv[1];
	GameObject *o = (objKey == 0) ? _currentObject : getObjectByKey(objKey);
	if (!o) {
		return 0;
	}
	if (parentKey == 0) {
		return -1;
	}
	GameObject *o_parent = getObjectByKey(parentKey);
	if (!o_parent) {
		return 0;
	}
	setObjectParent(o, o_parent);
	return -1;
}

int Game::op_removeObjectMessage(int argc, int32_t *argv) {
	assert(argc == 2);
	debug(kDebug_OPCODES, "Game::op_removeObjectMessage() [%d, %d]", argv[0], argv[1]);
	int32_t objKey = argv[0];
	if (objKey != -1) {
		GameObject *o = (objKey == 0) ? _currentObject : getObjectByKey(objKey);
		if (!o) {
			return 0;
		}
	}
	for (int i = 0; i < kPlayerMessagesTableSize; ++i) {
		GamePlayerMessage *msg = &_playerMessagesTable[i];
		if (msg->desc.duration > 0) {
			if (objKey == -1 || objKey == msg->objKey) {
				if (argv[1] == -1 || argv[1] == msg->value) {
					memset(msg, 0, sizeof(GamePlayerMessage));
					--_playerMessagesCount;
				}
			}
		}
	}
	return -1;
}

int Game::op_setCabinetItem(int argc, int32_t *argv) {
	assert(argc == 0);
	debug(kDebug_OPCODES, "Game::op_setCabinetItem() []");
	_cabinetItemCount = 0;
	_cabinetItemObj = _currentObject->o_child;
	if (_cabinetItemObj) {
		++_cabinetItemCount;
		for (GameObject *o = _cabinetItemObj; o->o_next; o = o->o_next) {
			++_cabinetItemCount;
		}
	}
	if (_cabinetItemCount == 0) {
		int32_t argv[] = { _objectsPtrTable[kObjPtrWorld]->objKey, 0 };
		op_addObjectMessage(2, argv);
	}
	return -1;
}


int Game::op_isObjectMoving(int argc, int32_t *argv) {
	assert(argc == 1);
	debug(kDebug_OPCODES, "Game::op_isObjectMoving() [%d]", argv[0]);
	int32_t objKey = argv[0];
	GameObject *o = (objKey == 0) ? _currentObject : getObjectByKey(objKey);
	if (!o) {
		return 0;
	}
	if (o->xPosPrev != o->xPos || o->yPosPrev != o->yPos || o->zPosPrev != o->zPos || o->pitchPrev != o->pitch) {
		return -1;
	}
	return 0;
}

int Game::op_getMessageInfo(int argc, int32_t *argv) {
	assert(argc == 1);
	int32_t num = argv[0];
	debug(kDebug_OPCODES, "Game::op_getMessageInfo() [%d]", argv[0]);
	for (GameMessage *msg = _currentObject->msg; msg; msg = msg->next) {
		if (msg->num == num) {
			GameObject *o = getObjectByKey(msg->objKey);
			const int angle = (o->pitch - _currentObject->pitch + 128) & 1023;
			_varsTable[3] = angle >> 8;
			_varsTable[4] = o->objKey;
			return -1;
		}
	}
	return 0;
}

int Game::op_testObjectsRoom(int argc, int32_t *argv) {
	assert(argc == 2);
	debug(kDebug_OPCODES, "Game::op_testObjectsRoom() [%d, %d]", argv[0], argv[1]);
	return testObjectsRoom(argv[0], argv[1]) ? -1 : 0;
}

int Game::op_setCellMapData(int argc, int32_t *argv) {
	assert(argc == 4);
	debug(kDebug_OPCODES, "Game::op_setCellMapData() [%d, %d, %d, %d]", argv[0], argv[1], argv[2], argv[3]);
	int32_t param = argv[0];
	int32_t value = argv[1];
	int32_t x = argv[2];
	int32_t z = argv[3];
	CellMap *cell = getCellMap(x, z);
	switch (param) {
	case 262:
		cell->type = value;
		break;
	case 263:
		cell->data[0] = value;
		break;
	case 264:
		cell->data[1] = value;
		break;
	case 265:
		cell->texture[0] = value;
		break;
	case 266:
		cell->texture[1] = value;
		break;
	default:
		error("Game::op_setCellMapData() unhandled param %d", param);
		break;
	}
	return -1;
}

int Game::op_addCellMapData(int argc, int32_t *argv) {
	assert(argc == 4);
	debug(kDebug_OPCODES, "Game::op_addCellMapData() [%d, %d, %d, %d]", argv[0], argv[1], argv[2], argv[3]);
	int32_t param = argv[0];
	int32_t value = argv[1];
	int32_t x = argv[2];
	int32_t z = argv[3];
	CellMap *cell = getCellMap(x, z);
	switch (param) {
	case 263:
		cell->data[0] += value;
		break;
	case 264:
		cell->data[1] += value;
		break;
	case 265:
		cell->texture[0] += value;
		break;
	case 266:
		cell->texture[1] += value;
		break;
	default:
		error("Game::op_addCellMapData() unhandled param %d", param);
		break;
	}
	return -1;
}

int Game::op_compareCellMapData(int argc, int32_t *argv) {
	assert(argc == 5);
	debug(kDebug_OPCODES, "Game::op_compareCellMapData() [%d, %d, %d, %d, %d]", argv[0], argv[1], argv[2], argv[3], argv[4]);
	int32_t param = argv[0];
	int32_t x = argv[3];
	int32_t z = argv[4];
	int32_t value = 0;
	CellMap *cell = getCellMap(x, z);
	switch (param) {
	case 264:
		value = cell->data[1];
		break;
	default:
		error("Game::op_compareCellMapData() unhandled param %d", param);
		break;
	}
	return compareHelper(argv[2], value, argv[1]);
}

int Game::op_getObjectDistance(int argc, int32_t *argv) {
	assert(argc == 4);
	debug(kDebug_OPCODES, "Game::op_getObjectDistance() [%d, %d, %d]", argv[0], argv[1], argv[2]);
	int32_t objKey = argv[0];
	uint32_t mask8 = argv[1];
	uint32_t mask21 = argv[2];
	int type = argv[3];

	GameObject *o = (objKey == 0) ? _currentObject : getObjectByKey(objKey);
	if (!o) {
		return 0;
	}

	_varsTable[11] = 0;
	_varsTable[2] = 0;

	int x = o->xPos + o->xPosParent;
	int z = o->zPos + o->zPosParent;

	GameObject *o_tmp, *o_cur = 0;
	int x_tmp, z_tmp, dist = 0;

	switch (type) {
	case 0:
		o_tmp = _objectsPtrTable[kObjPtrConrad];
		x_tmp = o_tmp->xPos + o_tmp->xPosParent;
		z_tmp = o_tmp->zPos + o_tmp->zPosParent;
		if ((o_tmp->specialData[1][8] & mask8) != 0 && (o_tmp->specialData[1][21] & mask21) != 0) {
			dist = getSquareDistance(x_tmp, z_tmp, x, z, kPosShift);
			o_cur = o_tmp;
		}
		o_tmp = getRoomObject(x, z)->o_child;
		for (o_tmp = o_tmp->o_child; o_tmp && (o_tmp->flags[1] & 0x100) == 0; o_tmp = o_tmp->o_next) {
			x_tmp = o_tmp->xPos + o_tmp->xPosParent;
			z_tmp = o_tmp->zPos + o_tmp->zPosParent;
			if ((o_tmp->specialData[1][8] & mask8) != 0 && (o_tmp->specialData[1][21] & mask21) != 0) {
				dist = getSquareDistance(x_tmp, z_tmp, x, z, kPosShift);
				o_cur = o_tmp;
			}
		}
		o_tmp = _objectsPtrTable[kObjPtrWorld]->o_child;
		for (o_tmp = o_tmp->o_child; o_tmp && (o_tmp->flags[1] & 0x100) == 0; o_tmp = o_tmp->o_next) {
			x_tmp = o_tmp->xPos + o_tmp->xPosParent;
			z_tmp = o_tmp->zPos + o_tmp->zPosParent;
			if ((o_tmp->specialData[1][8] & mask8) != 0 && (o_tmp->specialData[1][21] & mask21) != 0) {
				dist = getSquareDistance(x_tmp, z_tmp, x, z, kPosShift);
				o_cur = o_tmp;
			}
		}
		break;
	case 1:
		o_tmp = _objectsPtrTable[kObjPtrConrad];
		x_tmp = o_tmp->xPos + o_tmp->xPosParent;
		z_tmp = o_tmp->zPos + o_tmp->zPosParent;
		if ((o_tmp->specialData[1][8] & mask8) && (o_tmp->specialData[1][21] & mask21) && !testObjectCollision1(o, x, z, x_tmp, z_tmp, 0xFFFFFFFE)) {
			dist = getSquareDistance(x, z, x_tmp, z_tmp, kPosShift);
			o_cur = o_tmp;
		} else {
			dist = 0x7FFFFFFF;
		}
		o_tmp = getRoomObject(x, z)->o_child;
		while (o_tmp && (o_tmp->flags[1] & 0x100) == 0) {
			x_tmp = o_tmp->xPos + o_tmp->xPosParent;
			z_tmp = o_tmp->zPos + o_tmp->zPosParent;
			if (o_tmp != o && (o_tmp->specialData[1][8] & mask8) && (o_tmp->specialData[1][21] & mask21) && !testObjectCollision1(o, x, z, x_tmp, z_tmp, 0xFFFFFFFE)) {
				const int d = getSquareDistance(x, z, x_tmp, z_tmp, kPosShift);
				if (d < dist) {
					dist = d;
					o_cur = o_tmp;
				}
			}
			o_tmp = o_tmp->o_next;
		}
		o_tmp = _objectsPtrTable[kObjPtrWorld]->o_child;
		while (o_tmp && (o_tmp->flags[1] & 0x100) == 0) {
			x_tmp = o_tmp->xPos + o_tmp->xPosParent;
			z_tmp = o_tmp->zPos + o_tmp->zPosParent;
			if (o_tmp != o && (o_tmp->specialData[1][8] & mask8) && (o_tmp->specialData[1][21] & mask21) && testObjectsRoom(o->objKey, o_tmp->objKey) && !testObjectCollision1(o, x, z, x_tmp, z_tmp, 0xFFFFFFFE)) {
				const int d = getSquareDistance(x, z, x_tmp, z_tmp, kPosShift);
				if (d < dist) {
					dist = d;
					o_cur = o_tmp;
				}
			}
			o_tmp = o_tmp->o_next;
		}
		break;
	default:
		error("Game::op_getObjectDistance() unhandled type %d", type);
		break;
	}
	if (o_cur) {
		_varsTable[11] = o_cur->objKey;
		_varsTable[2] = dist;
	}
	return (o_cur != 0) ? -1 : 0;
}

int Game::op_setObjectSpecialCustomData(int argc, int32_t *argv) {
	assert(argc == 3);
	debug(kDebug_OPCODES, "Game::op_setObjectSpecialCustomData() [%d, %d, %d]", argv[0], argv[1], argv[2]);
	int32_t objKey = argv[0];
	int32_t param = argv[1];
	uint32_t value = argv[2];
	GameObject *o = (objKey == 0) ? _currentObject : getObjectByKey(objKey);
	if (!o) {
		return 0;
	}
	int *p;
	if (value >= 64) {
		value -= 64;
		assert(value < 12);
		p = &_currentObject->customData[value];
	} else {
		assert(value < 26);
		p = &_currentObject->specialData[1][value];
	}
	*p = getObjectData(o, param);
	return -1;
}

int Game::op_moveObjectToPos(int argc, int32_t *argv) {
	assert(argc == 3);
	debug(kDebug_OPCODES, "Game::op_moveObjectToPos() [%d, %d, %d]", argv[0], argv[1], argv[2]);
	int32_t objKey = argv[0];
	int32_t type = argv[1];
	int32_t angle = argv[2];
	GameObject *o = (objKey == 0) ? _currentObject : getObjectByKey(objKey);
	if (!o) {
		return 0;
	}
	if (!o->updateColliding && !o->setColliding) {
		addToCollidingObjects(o);
	}
	o->state = 1;
	int16_t key = _res.getChild(kResType_ANI, _currentObject->anim.currentAnimKey);
	assert(key != 0);
	uint8_t *p_anifram = _res.getData(kResType_ANI, key, "ANIFRAM");
	assert(p_anifram);
	_currentObject->anim.aniframData = p_anifram;
	if (p_anifram[2] == 9) {
		_currentObject->anim.currentAnimKey = _res.getChild(kResType_ANI, _currentObject->anim.animKey);
		_currentObject->anim.anikeyfData = _res.getData(kResType_ANI, _currentObject->anim.currentAnimKey, "ANIKEYF");
		uint8_t *p_form3d, *p_poly3d;
		uint8_t *verticesData, *polygonsData;
		p_form3d = initMesh(kResType_F3D, READ_LE_UINT16(p_anifram), &verticesData, &polygonsData, o, &p_poly3d, &o->specialData[1][20]);
		if (!p_form3d) {
			return 0;
		}
		const int verticesCount = READ_LE_UINT16(p_form3d + 18);
		int v2 = 0;
		switch (type) {
		case 0:
//			v1 = READ_LE_UINT32(p_poly3d + 4);
			v2 = READ_LE_UINT32(p_poly3d + 8);
			break;
		case 1:
//			v1 = READ_LE_UINT32(p_poly3d + 12);
			v2 = READ_LE_UINT32(p_poly3d + 16);
			break;
		default:
			error("Game::op_moveObjectToPos() unhandled type %d", type);
			break;
		}
		const int16_t ani_dx = READ_LE_UINT16(_currentObject->anim.anikeyfData + 2);
		const int16_t ani_dy = READ_LE_UINT16(_currentObject->anim.anikeyfData + 4);
		const int16_t ani_dz = READ_LE_UINT16(_currentObject->anim.anikeyfData + 6);
//		assert(v1 < verticesCount);
//		Vertex vert1 = READ_VERTEX32(verticesData + v1 * 4);
//		int x = vert1.x - (ani_dx >> 2);
//		int y = vert1.y * 2;
//		int z = vert1.z - (ani_dz >> 2);
		assert(v2 < verticesCount);
		Vertex vert2 = READ_VERTEX32(verticesData + v2 * 4);
		int x0 = vert2.x - (ani_dx >> 2);
		int y0 = vert2.y * 2;
		int z0 = vert2.z - (ani_dz >> 2);
		o->pitch = (_currentObject->pitch + angle) & 1023;
		Vec_xz vec(x0, z0);
		vec.rotate(o->pitch, 3);
		o->xPos = _currentObject->xPosParent + _currentObject->xPos + vec.x;
		o->zPos = _currentObject->zPosParent + _currentObject->zPos + vec.z;
		o->yPos = _currentObject->yPosParent + _currentObject->yPos;
		if (type != 2) {
			o->yPos -= (ani_dy << 11);
			o->yPos += (y0 << 13) - (64 << 15);
		}
	}
	return -1;
}

int Game::op_setupObjectPath(int argc, int32_t *argv) {
	assert(argc == 2);
	debug(kDebug_OPCODES, "Game::op_setupObjectPath() [%d, %d]", argv[0], argv[1]);
	const int param1 = argv[0];
	const int param2 = argv[1];
	GameObject *o_path = getObjectByKey(_currentObject->getData(param1));
	int path = o_path->getData(param2);
	if (path == 0) {
		return 0;
	}
	_currentObject->setData(param1, path);
	return -1;
}

int Game::op_continueObjectMove(int argc, int32_t *argv) {
	assert(argc == 0);
	debug(kDebug_OPCODES, "Game::op_continueObjectMove() []");
	int x = _currentObject->xPos = _varsTable[6];
	int z = _currentObject->zPos = _varsTable[7];
	if (x < 0 || x >= (64 << 19) || z < 0 || z >= (64 << 19)) {
		return 0;
	}
	const CellMap *cell = getCellMapShr19(x, z);
	for (CollisionSlot *colSlot = cell->colSlot; colSlot; colSlot = colSlot->next) {
		GameObject *o = colSlot->o;
		if (o->specialData[1][21] == 256) {
			_snd.playSfx(o->objKey, _res._sndKeysTable[9]);
			o->specialData[1][18] -= _currentObject->specialData[1][18];
			if (o->specialData[1][18] <= 0) {
				playDeathCutscene(o->objKey);
				_endGame = true;
				return -1;
			}
		}
	}
	return -1;
}

int Game::op_compareInput(int argc, int32_t *argv) {
	assert(argc == 2);
	debug(kDebug_OPCODES, "Game::op_compareInput() [%d, %d]", argv[0], argv[1]);
	uint32_t mask = argv[0];
	int32_t input = argv[1];
	assert(input < _inputsCount);
	return isInputKeyDown(mask, input) ? -1 : 0;
}

int Game::op_clearTarget(int argc, int32_t *argv) {
	assert(argc == 1);
	debug(kDebug_OPCODES, "Game::op_clearTarget() [%d]", argv[0]);
	int32_t flag = argv[0];
	if (flag == -1 || _varsTable[12] == _currentObject->objKey) {
		_varsTable[12] = 0;
		_varsTable[13] = 0;
	}
	return -1;
}

int Game::op_playCutscene(int argc, int32_t *argv) {
	assert(argc == 1);
	debug(kDebug_OPCODES, "Game::op_playCutscene() [%d]", argv[0]);
	_cut->queue(argv[0]);
	return -1;
}

int Game::op_setScriptCond(int argc, int32_t *argv) {
	assert(argc == 1);
	debug(kDebug_OPCODES, "Game::op_setScriptCond() [%d]", argv[0]);
	int count = argv[0];
	for (int i = 0; i < count; ++i) {
		_currentObject->scriptCondData = getNextScriptAnim();
	}
	return 0;
}

int Game::op_isObjectMessageNull(int argc, int32_t *argv) {
	assert(argc == 0);
	debug(kDebug_OPCODES, "Game::op_isObjectMessageNull() []");
	return _currentObject->msg ? 0 : -1;
}

int Game::op_detachObjectChild(int argc, int32_t *argv) {
	assert(argc == 2);
	debug(kDebug_OPCODES, "Game::op_detachObjectChild() [%d, %d]", argv[0], argv[1]);
	int32_t objKey = argv[0];
	int32_t param = argv[1];
	GameObject *o = (objKey == 0) ? _currentObject : getObjectByKey(objKey);
	if (!o || !o->o_child) {
		return 0;
	}
	o = o->o_child;
	_currentObject->setData(param, o->objKey);
	setObjectParent(o, _objectsPtrTable[kObjPtrWorld]);
	GameObject *o_tmp = _currentObject;
	_currentObject = o;
	int32_t arg = o_tmp->objKey;
	op_moveObjectToObject(1, &arg);
	_currentObject->state = 1;
	_currentObject = o_tmp;
	return -1;
}

int Game::op_setCamera(int argc, int32_t *argv) {
	assert(argc == 3);
	debug(kDebug_OPCODES, "Game::op_setCamera() [%d, %d, %d]", argv[0], argv[1], argv[2]);
	int32_t param = argv[0];
	int32_t value = argv[1];
	int32_t ticks = argv[2];
	if (_cameraState != 8) {
		if (_cameraViewKey == _objectsPtrTable[kObjPtrConrad]->objKey) {
			switch (param) {
			case 0:
				if (_cameraState != value) {
					if (value == 2 && _cameraState == 0) {
						_cameraFixDist = _cameraDefaultDist;
						_cameraDefaultDist = 0;
						_yPosObserverTemp = _yPosObserver;
						_yPosObserverValue = _yPosObserver = 0;
						_cameraDistValue = _cameraDist = (kWallWidth * 2) << 15;
						_focalDistance = 0;
						_yPosObserverTicks = 1;
						_cameraDistTicks = 1;
					} else if (value == 0) {
						_cameraDefaultDist = _cameraFixDist;
						_yPosObserver = _yPosObserverTemp;
						if (_cameraDefaultDist) {
							_yPosObserverTicks = 2;
							_cameraDistTicks = 2;
						} else {
							_yPosObserverTicks = 1;
							_cameraDistTicks = 1;
						}
					}
					initViewport();
					_cameraNum = ticks;
					_prevCameraState = _cameraState;
					_cameraState = value;
				}
				break;
			case 1:
				if (_cameraDefaultDist && isConradInShootingPos()) {
					_cameraDeltaRotYValue2 = value;
				} else {
					_cameraDeltaRotY = value;
					_cameraDeltaRotYTicks = ticks;
				}
				break;
			case 2:
				if (_cameraDefaultDist && isConradInShootingPos()) {
					_cameraDistValue2 = value << 15;
				} else {
					_cameraDistValue = value << 15;
					_cameraDistTicks = ticks;
				}
				_yRotObserverPrev = 0;
				break;
			case 3:
				if (_cameraDefaultDist && _level != 6 && isConradInShootingPos()) {
					_yPosObserverValue2 = value << 15;
				} else if (_cameraState == 2) {
					_yPosObserver = _yPosObserverValue = value << 15;
				} else {
					_yPosObserverValue = value << 15;
					_yPosObserverTicks = ticks;
				}
				break;
			case 4:
				_cameraStep1 = _cameraStep2 = value;
				break;
			case 5:
				_cameraDistTicks = value;
				break;
			case 6:
				if (_cameraDefaultDist && _level != 6 && isConradInShootingPos()) {
					_yPosObserverValue2 = value;
				} else if (_cameraState == 2) {
					_yPosObserver = _yPosObserverValue = value;
				} else {
					_yPosObserverValue = value;
					_yPosObserverTicks = ticks;
				}
				break;
			case 7:
				_cameraDelta = value;
				break;
			case 9:
				_focalDistance = value;
				break;
			default:
				error("op_setCamera() unhandled param %d", param);
				break;
			}
		} else {
			// GameInput *inp = &_inputsTable[_objectsPtrTable[kObjPtrConrad]->customData[11]];
			warning("op_setCamera() unhandled param %d (_cameraViewKey %d)", param, _cameraViewKey);
		}
	}
	return -1;
}

int Game::op_getSquareDistance(int argc, int32_t *argv) {
	assert(argc == 2);
	debug(kDebug_OPCODES, "Game::op_getSquareDistance() [%d, %d]", argv[0], argv[1]);
	int32_t obj1Key = argv[0];
	int32_t obj2Key = argv[1];
	GameObject *o1 = (obj1Key == 0) ? _currentObject : getObjectByKey(obj1Key);
	if (!o1) {
		return 0;
	}
	GameObject *o2 = (obj2Key == 0) ? _currentObject : getObjectByKey(obj2Key);
	if (!o2) {
		return 0;
	}
	const int x1 = (o1->xPosParent + o1->xPos) >> kPosShift;
	const int z1 = (o1->zPosParent + o1->zPos) >> kPosShift;
	const int x2 = (o2->xPosParent + o2->xPos) >> kPosShift;
	const int z2 = (o2->zPosParent + o2->zPos) >> kPosShift;
	_varsTable[2] = getSquareDistance(x1, z1, x2, z2);
	return -1;
}

static int getBottomTopMask(int hcollid) {
	if (hcollid & 0x80000000)
		return 0;
	int mask = 0x40000000;
	int y = (4 << 15);
	for (int i = 0; i < 30; i++) {
		if (mask & hcollid)
			return y;
		mask >>= 1;
		y += 2 << 15;
	}
	return 0;
}

static int getTopBottomMask(int hcollid) {
	int mask = 0x00000002;
	int y = (64 << 15);
	for (int i = 0; i < 31; i++) {
		if (mask & hcollid)
			return y;
		mask <<= 1;
		y -= 2 << 15;
	}
	return 0;
}

static int getColMaskYmax(int hcollid) {
	return getBottomTopMask(hcollid) >> 15;
}

static int getColMaskYmin(int hcollid) {
	return getTopBottomMask(hcollid) >> 15;
}

int Game::op_isObjectTarget(int argc, int32_t *argv) {
	assert(argc == 4);
	debug(kDebug_OPCODES, "Game::op_isObjectTarget() [%d, %d, %d, %d]", argv[0], argv[1], argv[2], argv[3]);

	int32_t objKey = argv[0];
	int32_t param = argv[1];
	int32_t raysCount = argv[2];
	int32_t delta = argv[3];

	GameObject *o = (objKey == 0) ? _currentObject : getObjectByKey(objKey);
	if (!o) {
		return 0;
	}

	GameObject *o1 = getObjectByKey(_currentObject->specialData[1][9]);

	_tmpObject.xPosParent = o1->xPosParent;
	_tmpObject.zPosParent = o1->zPosParent;
	_tmpObject.pitch = o->pitch;
	_tmpObject.o_parent = o1;
	_tmpObject.specialData[1][21] = _currentObject->specialData[1][10];

	if (_currentObject->specialData[1][9] == _objectsPtrTable[kObjPtrConrad]->objKey) {
		_tmpObject.specialData[1][8] = -2;
		int yorigin = getColMaskYmax(0x1E000);
		_rayCastedObjectsCount = 0;
		for (int i = -raysCount >> 1; i <= raysCount >> 1; i++) {
			_tmpObject.xPos = o1->xPos;
			_tmpObject.zPos = o1->zPos;
			CellMap tmpCell;
			rayCastMono(&_tmpObject, i * delta, &tmpCell, &Game::rayCastCollisionCb2, _cameraDelta);
		}
		int xs = (o1->xPos + o1->xPosParent) >> kPosShift;
		int zs = (o1->zPos + o1->zPosParent) >> kPosShift;
		for (int i = 0; i < _rayCastedObjectsCount; i++) {
			int xd = _rayCastedObjectsTable[i].x >> kPosShift;
			int zd = _rayCastedObjectsTable[i].z >> kPosShift;
			_rayCastedObjectsTable[i].dist = fixedSqrt((xs - xd) * (xs - xd) + (zs - zd) * (zs - zd));
			_rayCastedObjectsTable[i].ymax = 64 - getColMaskYmax(_rayCastedObjectsTable[i].o->specialData[1][8]) - yorigin;
			_rayCastedObjectsTable[i].ymin = 64 - getColMaskYmin(_rayCastedObjectsTable[i].o->specialData[1][8]) - yorigin;
		}
		for (int i = 0; i < _rayCastedObjectsCount - 1; i++) {
			for (int j = i + 1; j < _rayCastedObjectsCount; j++) {
				if (_rayCastedObjectsTable[i].dist > _rayCastedObjectsTable[j].dist) {
					SWAP(_rayCastedObjectsTable[i], _rayCastedObjectsTable[j]);
				}
			}
		}
		int collidObjectNum = -1;
		for (int i = 0; i < _rayCastedObjectsCount; ++i) {
			if (_rayCastedObjectsTable[i].o->specialData[1][21] == 16 && _rayCastedObjectsTable[i].dist >= 4) {
				const int slope = ((_rayCastedObjectsTable[i].ymax - 4) << 15) / _rayCastedObjectsTable[i].dist;
				bool collid = 0;
				for (int j = 0; j < i && !collid; ++j) {
					if (_rayCastedObjectsTable[j].o->specialData[1][21] != 16) {
						int y = (slope * _rayCastedObjectsTable[j].dist) >> 15;
						if (y >= _rayCastedObjectsTable[j].ymin && y <= _rayCastedObjectsTable[j].ymax) {
							collid = true;
							if (collidObjectNum == -1) {
								collidObjectNum = j;
							}
						}
					}
				}
				if (!collid) {
					_varsTable[5] = _rayCastedObjectsTable[i].o->objKey;
					_currentObject->setData(param, _varsTable[5]);
					return -1;
				}
			}
		}
		if (collidObjectNum != -1) {
			_tmpObject.xPos = _rayCastedObjectsTable[collidObjectNum].x;
			_tmpObject.zPos = _rayCastedObjectsTable[collidObjectNum].z;
			_varsTable[6] = _tmpObject.xPos;
			_varsTable[7] = _tmpObject.zPos;
		} else {
			for (int i = 0; i < _rayCastedObjectsCount; ++i) {
				if (_rayCastedObjectsTable[i].o->specialData[1][8] & 0x10) {
					_tmpObject.xPos = _rayCastedObjectsTable[i].x;
					_tmpObject.zPos = _rayCastedObjectsTable[i].z;
					_varsTable[6] = _tmpObject.xPos;
					_varsTable[7] = _tmpObject.zPos;
				}
			}
		}
		_varsTable[6] = _tmpObject.xPos;
		_varsTable[7] = _tmpObject.zPos;
	} else {
		_tmpObject.specialData[1][8] = o1->specialData[1][8];
		for (int i = -raysCount / 2; i <= raysCount / 2; ++i) {
			_tmpObject.xPos = o1->xPos;
			_tmpObject.zPos = o1->zPos;
			_tmpObject.specialData[1][21] = _currentObject->specialData[1][10];
			CellMap tmpCell;
			const int result = rayCastMono(&_tmpObject, i * delta, &tmpCell, &Game::rayCastCollisionCb1, _cameraDelta);
			if (result > 0) {
				_varsTable[5] = result;
				_currentObject->setData(param, result);
				return -1;
			}
			if (i == 0) {
				_varsTable[6] = _tmpObject.xPos;
				_varsTable[7] = _tmpObject.zPos;
			}
		}
		if (_varsTable[6] > (64 << (4 + kPosShift))) {
			_varsTable[6] = (64 << (4 + kPosShift)) - 1;
		} else if (_varsTable[6] < 0) {
			_varsTable[6] = 0;
		}
		if (_varsTable[7] > (64 << (4 + kPosShift))) {
			_varsTable[7] = (64 << (4 + kPosShift)) - 1;
		} else if (_varsTable[7] < 0) {
			_varsTable[7] = 0;
		}
	}
	return 0;
}

int Game::op_printDebug(int argc, int32_t *argv) {
	assert(argc == 3);
	debug(kDebug_OPCODES, "Game::op_printDebug() [%d, %d, %d]", argv[0], argv[1], argv[2]);
	// no-op
	return -1;
}

int Game::op_translateObject(int argc, int32_t *argv) {
	assert(argc == 3);
	debug(kDebug_OPCODES, "Game::op_translateObject() [%d, %d, %d]", argv[0], argv[1], argv[2]);
	int32_t dx = argv[0];
	int32_t dy = argv[1];
	int32_t dz = argv[2];
	int cosy = g_cos[_currentObject->pitch & 1023];
	int siny = g_sin[_currentObject->pitch & 1023];
	int xPos = _currentObject->xPos + cosy * dx + siny * dz;
	int zPos = _currentObject->zPos + cosy * dz - siny * dx;
	_currentObject->xPos = xPos;
	_currentObject->yPos += (dy << 15);
	_currentObject->zPos = zPos;
	return -1;
}

int Game::op_setupTarget(int argc, int32_t *argv) {
	assert(argc == 4);
	debug(kDebug_OPCODES, "Game::op_setupTarget() [%d, %d, %d, %d]", argv[0], argv[1], argv[2], argv[3]);
	const int ticks1 = argv[0];
	const int ticks2 = argv[1];
	const int type = argv[2];
	const int bypass = argv[3];
	if (bypass == 0 || _varsTable[12] == _currentObject->objKey) {
		return -1;
	}
	int delta = 0;
	switch (type) {
	case 1:
		_tmpObject.specialData[1][8] = _currentObject->specialData[1][8];
		delta = 5;
		break;
	case 2:
		_tmpObject.specialData[1][8] = 0x10;
		delta = 5;
		break;
	}
	if (type != 0) {
		bool foundTargetObject = false;
		_tmpObject.xPosParent = _currentObject->xPosParent;
		_tmpObject.zPosParent = _currentObject->zPosParent;
		_tmpObject.pitch = _currentObject->pitch;
		_tmpObject.o_parent = _currentObject;
		_tmpObject.specialData[1][21] = 19;
		const int deltaTable[] = { -delta, 0, delta };
		for (int i = 0; i < 3; ++i) {
			_tmpObject.xPos = _currentObject->xPos;
			_tmpObject.zPos = _currentObject->zPos;
			CellMap tmpCell;
			const int result = rayCastMono(&_tmpObject, deltaTable[i], &tmpCell, &Game::rayCastCollisionCb1, _cameraDelta);
			const int firstRayX = _tmpObject.xPos;
			const int firstRayZ = _tmpObject.zPos;
			if (result > 0) {
				foundTargetObject = true;
				break;
			}
			_tmpObject.xPos = firstRayX;
			_tmpObject.zPos = firstRayZ;
		}
		if (!foundTargetObject) {
			return 0;
		}
	}
	int x = _objectsPtrTable[kObjPtrConrad]->xPos + _objectsPtrTable[kObjPtrConrad]->xPosParent;
	int z = _objectsPtrTable[kObjPtrConrad]->zPos + _objectsPtrTable[kObjPtrConrad]->zPosParent;
	CellMap *conradCellMap = getCellMapShr19(x, z);
	x = _currentObject->xPos + _currentObject->xPosParent;
	z = _currentObject->zPos + _currentObject->zPosParent;
	const int roomObj = getRoom(x, z);
	const bool sameRoom = (roomObj == conradCellMap->room || roomObj == conradCellMap->room2);
	if (_varsTable[12] == 0 && sameRoom && (_currentObject->specialData[1][24] + ticks1 - ticks2 <= _ticks)) {
		_currentObject->specialData[1][25] = _ticks;
		_varsTable[12] = _currentObject->objKey;
		_varsTable[13] = ticks2 << 3;
		return -1;
	}
	return 0;
}

int Game::op_getTicks(int argc, int32_t *argv) {
	assert(argc == 1);
	debug(kDebug_OPCODES, "Game::op_getTicks() [%d]", argv[0]);
	int32_t param = argv[0];
	_currentObject->setData(param, _ticks);
	return -1;
}

int Game::op_swapFrameXZ(int argc, int32_t *argv) {
	assert(argc == 0);
	debug(kDebug_OPCODES, "Game::op_swapFrameXZ() []");
	CollisionSlot2 slots2[65];
	slots2[0].box = -1;
	GameObject *obj = _currentObject;
	const int ox1 = obj->xFrm1;
	const int oz1 = obj->zFrm1;
	const int ox2 = obj->xFrm2;
	const int oz2 = obj->zFrm2;
	obj->xFrm1 = oz1;
	obj->zFrm1 = ox1;
	obj->xFrm2 = oz2;
	obj->zFrm2 = ox2;
	if (setCollisionSlotsUsingCallback2(obj, obj->xPosParent + obj->xPos, obj->zPosParent + obj->zPos, &Game::collisionSlotCb3, 0xFFFFFFFE, slots2) == 0) {
		obj->xFrm1 = ox1;
		obj->zFrm1 = oz1;
		obj->xFrm2 = ox2;
		obj->zFrm2 = oz2;
		return 0;
	}
	resetCollisionSlot(obj);
	setCollisionSlotsUsingCallback1(obj->xPosParent + obj->xPos, obj->zPosParent + obj->zPos, &Game::collisionSlotCb2);
	return -1;
}

int Game::op_addObjectMessage(int argc, int32_t *argv) {
	assert(argc == 2);
	debug(kDebug_OPCODES, "Game::op_addObjectMessage() [%d, %d]", argv[0], argv[1]);
	int32_t objKey = argv[0];
	int32_t value = argv[1];
	GameObject *o = (objKey == 0) ? _currentObject : getObjectByKey(objKey);
	if (!o) {
		return 0;
	}
	for (int i = 0; i < kPlayerMessagesTableSize; ++i) {
		GamePlayerMessage *msg = &_playerMessagesTable[i];
		if (msg->objKey == o->objKey && msg->value == value) {
			return -1;
		}
	}
	GamePlayerMessage *msg = 0;
	for (int i = 0; i < kPlayerMessagesTableSize; ++i) {
		if (!_playerMessagesTable[i].desc.data) {
			msg = &_playerMessagesTable[i];
			break;
		}
	}
	if (msg) {
		if (!getMessage(o->objKey, value, &msg->desc)) {
			return 0;
		}
		++_playerMessagesCount;
		msg->w = 0;
		msg->h = 0;
		msg->objKey = o->objKey;
		msg->value = value;
		msg->visible = false;
		msg->xPos = msg->desc.xPos;
		msg->yPos = msg->desc.yPos;
		if (msg->desc.font & 0x80) {
			char buf[64];
			snprintf(buf, sizeof(buf), "%s_%d", o->name, value);
			msg->crc = getStringHash(buf);
			_snd.playVoice(msg->objKey, msg->crc);
		}
		getStringRect((const char *)msg->desc.data, msg->desc.font, &msg->w, &msg->h);
		setStringPos(msg);
	}
	return -1;
}

int Game::op_setupCircularMove(int argc, int32_t *argv) {
	assert(argc == 4);
	debug(kDebug_OPCODES, "Game::op_setupCircularMove() [%d, %d, %d, %d]", argv[0], argv[1], argv[2], argv[3]);
	int32_t radius = argv[0];
	int32_t direction = argv[1];
	int32_t param1 = argv[2];
	int32_t param2 = argv[3];
	int cosy = g_cos[_currentObject->pitch & 1023];
	int siny = g_sin[_currentObject->pitch & 1023];
	const int dx = radius * cosy;
	const int dz = radius * -siny;
	switch (direction) {
	case 1:
		_currentObject->setData(param1, _currentObject->xPosParent + _currentObject->xPos + dx);
		_currentObject->setData(param2, _currentObject->zPosParent + _currentObject->zPos + dz);
		break;
	case -1:
		_currentObject->setData(param1, _currentObject->xPosParent + _currentObject->xPos - dx);
		_currentObject->setData(param2, _currentObject->zPosParent + _currentObject->zPos - dz);
		break;
	default:
		error("Game::op_setupCircularMove() invalid direction %d", direction);
		break;
	}
	return -1;
}

int Game::op_moveObjectOnCircle(int argc, int32_t *argv) {
	assert(argc == 5);
	debug(kDebug_OPCODES, "Game::op_moveObjectOnCircle() [%d, %d, %d, %d, %d]", argv[0], argv[1], argv[2], argv[3], argv[4]);
	int32_t xC = argv[0];
	int32_t zC = argv[1];
	int32_t radius = argv[2];
	int32_t delta = argv[3];
	int32_t direction = argv[4];
	int angle, dx, dz;
	switch (direction) {
	case 1:
		angle = (_currentObject->pitch + delta - 256) & 1023;
		dx = fixedMul(g_sin[angle], (radius << 15), 15);
		dz = fixedMul(g_cos[angle], (radius << 15), 15);
		_currentObject->xPos = xC + dx - _currentObject->xPosParent;
		_currentObject->zPos = zC + dz - _currentObject->zPosParent;
		_currentObject->pitch += delta;
		_currentObject->pitch &= 1023;
		break;
	case -1:
		angle = (_currentObject->pitch - delta + 256) & 1023;
		dx = fixedMul(g_sin[angle], (radius << 15), 15);
		dz = fixedMul(g_cos[angle], (radius << 15), 15);
		_currentObject->xPos = xC + dx - _currentObject->xPosParent;
		_currentObject->zPos = zC + dz - _currentObject->zPosParent;
		_currentObject->pitch -= delta;
		_currentObject->pitch &= 1023;
		break;
	default:
		error("Game::op_moveObjectOnCircle() invalid direction %d", direction);
		break;
	}
	return -1;
}

int Game::op_isObjectCollidingType(int argc, int32_t *argv) {
	assert(argc == 5);
	debug(kDebug_OPCODES, "Game::op_isObjectCollidingType() [%d, %d, %d, %d, %d]", argv[0], argv[1], argv[2], argv[3], argv[4]);
	int32_t objKey = argv[0];
	int distance = argv[1];
	uint32_t mask = argv[2];
	int dy = argv[3];
	int type = argv[4];
	GameObject *o = (objKey == 0) ? _currentObject : getObjectByKey(objKey);
	if (!o) {
		return 0;
	}
	int pitchTable[3] = { 0, 0, 0 };
	int pitch = 2;
	if ((o->flags[1] & 0x10000) != 0 && _varsTable[kVarPlayerObject] == o->objKey) {
		if (distance != 0) {
			pitchTable[0] = 0;
			pitchTable[1] = 48;
			pitchTable[2] = -48;
			pitch = 0;
		}
	}
	int ret = 0;
	CollisionSlot2 slots2[65];
	bool collidingTest = false;
	int xPosStart, zPosStart, xPos, zPos;
	do {
		slots2[0].box = -1;
		int xPosPrev = -1;
		int zPosPrev = -1;
		ret = 0;
		const int a = (o->pitch + dy + pitchTable[pitch]) & 1023;
		int xDistance = g_sin[a] * distance;
		int zDistance = g_cos[a] * distance;
		int stepCount, stepCount2;
		int dxStep, dzStep;
		if (xDistance != 0 || zDistance != 0) {
			if (ABS(xDistance) >= ABS(zDistance)) {
				stepCount = (ABS(xDistance) >> kPosShift) + 1;
			} else {
				stepCount = (ABS(zDistance) >> kPosShift) + 1;
			}
			dxStep = xDistance / stepCount;
			dzStep = zDistance / stepCount;
			stepCount2 = stepCount >> 1;
		} else {
			stepCount = 1;
			dxStep = 0;
			dzStep = 0;
			stepCount2 = 0;
		}
		xPosStart = xPos = o->xPosParent + o->xPos;
		zPosStart = zPos = o->zPosParent + o->zPos;
		while (stepCount--) {
			if (stepCount == 0) {
				xPos = xPosStart + xDistance;
				zPos = zPosStart + zDistance;
			} else if (stepCount == stepCount2) {
				xPos = xPosStart + (xDistance >> 1);
				zPos = zPosStart + (zDistance >> 1);
			}
			if (xPos != xPosPrev || zPos != zPosPrev) {
				collidingTest = setCollisionSlotsUsingCallback3(o, xPos, zPos, &Game::collisionSlotCb4, mask, type, slots2);
				if (!collidingTest) {
					ret = -1;
					if (_updateGlobalPosRefObject && (_updateGlobalPosRefObject->flags[1] & 0x100) == 0) {
						_varsTable[14] = _updateGlobalPosRefObject->objKey;
					} else {
						_varsTable[14] = 0;
						_varsTable[6] = xPos;
						_varsTable[7] = zPos;
					}
				}
			}
			xPosPrev = xPos;
			zPosPrev = zPos;
			xPos += dxStep;
			zPos += dzStep;
			if (ret == -1) {
				return -1;
			}
		}
		++pitch;
	} while (!collidingTest && pitch < 3);
	if (collidingTest) {
		o->pitch += pitchTable[pitch - 1];
	}
	if (!collidingTest && (o->flags[1] & 0x800) != 0 && getCellMapShr19(xPos, zPos)->isDoor) {
		ret = -1;
	}
	return ret;
}

int Game::op_setLevelData(int argc, int32_t *argv) {
	assert(argc == 1);
	debug(kDebug_OPCODES, "Game::op_setLevelData() [%d]", argv[0]);
	// no-op
	return -1;
}

int Game::op_drawNumber(int argc, int32_t *argv) {
	assert(argc == 4);
	debug(kDebug_OPCODES, "Game::op_drawNumber() [%d, %d, %d, %d]", argv[0], argv[1], argv[2], argv[3]);
	if (_mainLoopCurrentMode == 1) {
		_drawNumber.x = argv[0]; // getScaledCoord(argv[0]);
		_drawNumber.y = argv[1]; // getScaledCoord(argv[1]);
		int value = argv[2];
		_drawNumber.font = argv[3];
		if ((value % 10) != 0) {
			value = (value / 10) * 10 + 10;
		}
		_drawNumber.value = value;
	}
	return -1;
}

int Game::op_isObjectOnMap(int argc, int32_t *argv) {
	assert(argc == 3);
	debug(kDebug_OPCODES, "Game::op_isObjectOnMap() [%d, %d, %d]", argv[0], argv[1], argv[2]);
	int32_t x = argv[0];
	int32_t z = argv[1];
	uint32_t mask = argv[2];
	const CellMap *cell = getCellMap(x, z);
	for (CollisionSlot *colSlot = cell->colSlot; colSlot; colSlot = colSlot->next) {
		GameObject *o = colSlot->o;
		if ((o->specialData[1][21] & mask) != 0 && o->o_parent != _objectsPtrTable[kObjPtrCimetiere]) {
			return -1;
		}
	}
	return 0;
}

int Game::op_updateFollowingObject(int argc, int32_t *argv) {
	assert(argc == 4);
	debug(kDebug_OPCODES, "Game::op_updateFollowingObject() [%d, %d, %d, %d]", argv[0], argv[1], argv[2], argv[3]);

	int16_t followerObjKey = argv[0];
	int16_t followedObjKey = argv[1];
	int32_t specx = argv[2];
	int32_t specz = argv[3];

	GameObject *o_follower = (followerObjKey == 0) ? _currentObject : getObjectByKey(followerObjKey);
	if (!o_follower) {
		return 0;
	}
	GameObject *o_followed = (followedObjKey == 0) ? _currentObject : getObjectByKey(followedObjKey);
	if (!o_followed) {
		return 0;
	}

	GameFollowingPoint *points = (_followingObjectsTable + o_follower->customData[11])->points;
	int *count = &o_follower->customData[10];
	int xTo = o_followed->xPosParent + o_followed->xPos;
	int zTo = o_followed->zPosParent + o_followed->zPos;
	const int x1Col = ((o_follower->xFrm1 - o_followed->xFrm1) >> 15) - kFollowingMargin;
	const int z1Col = ((o_follower->zFrm1 - o_followed->zFrm1) >> 15) - kFollowingMargin;
	const int x2Col = ((o_follower->xFrm2 - o_followed->xFrm2) >> 15) + kFollowingMargin;
	const int z2Col = ((o_follower->zFrm2 - o_followed->zFrm2) >> 15) + kFollowingMargin;
	if (testObjectCollision2(o_followed, x1Col, z1Col, x2Col, z2Col)) {
		fixCoordinates(o_followed, x1Col, z1Col, x2Col, z2Col, &xTo, &zTo);
	}
	static const uint32_t mask = 0xFFFFFFFE;
	switch (*count) {
	case -1: {
			int xFrom = o_follower->xPosParent + o_follower->xPos;
			int zFrom = o_follower->zPosParent + o_follower->zPos;
			if (!testObjectCollision1(o_follower, xFrom, zFrom, xTo, zTo, mask) || _varsTable[32] != 0) {
				points[0].x = xTo;
				points[0].z = zTo;
				*count = 0;
				_currentObject->setData(specx, points[0].x);
				_currentObject->setData(specz, points[0].z);
	                } else {
				return 0;
			}
		}
		break;
	case 0: {
			int xFrom = o_follower->xPosParent + o_follower->xPos;
			int zFrom = o_follower->zPosParent + o_follower->zPos;
			if (!testObjectCollision1(o_follower, xFrom, zFrom, xTo, zTo, mask) || _varsTable[32] != 0) {
				points[0].x = xTo;
				points[0].z = zTo;
				_currentObject->setData(specx, points[0].x);
				_currentObject->setData(specz, points[0].z);
			} else {
				fixCoordinates2(o_follower, xFrom, zFrom, &points[0].x, &points[0].z, &xTo, &zTo);
				points[1].x = xTo;
				points[1].z = zTo;
				*count = 1;
			}
		}
		break;
	default: {
			int xFrom = o_follower->xPosParent + o_follower->xPos;
			int zFrom = o_follower->zPosParent + o_follower->zPos;
			if (!testObjectCollision1(o_follower, xFrom, zFrom, xTo, zTo, mask) || _varsTable[32] != 0) {
				points[0].x = xTo;
				points[0].z = zTo;
				*count = 0;
				_currentObject->setData(specx, points[0].x);
				_currentObject->setData(specz, points[0].z);
			} else {
				int flag = 0;
				for (int i = 0; i < *count - 1; i++) {
					xFrom = points[i].x;
					zFrom = points[i].z;
					if (!testObjectCollision1(o_follower, xFrom, zFrom, xTo, zTo, mask) || _varsTable[32] != 0) {
						points[i + 1].x = xTo;
						points[i + 1].z = zTo;
						*count = i + 1;
						flag = 1;
						break;
					}
				}
				if (!flag) {
					xFrom = points[*count - 1].x;
					zFrom = points[*count - 1].z;
					if (!testObjectCollision1(o_follower, xFrom, zFrom, xTo, zTo, mask) || _varsTable[32] != 0) {
						points[*count].x = xTo;
						points[*count].z = zTo;
					} else {
						fixCoordinates2(o_follower, points[*count - 1].x, points[*count - 1].z, &points[*count].x, &points[*count].z, &xTo, &zTo);
						if (*count < kFollowingObjectPointsTableSize - 1) {
							(*count)++;
							points[*count].x = xTo;
							points[*count].z = zTo;
						}
					}
				}
				xFrom = o_follower->xPosParent + o_follower->xPos;
				zFrom = o_follower->zPosParent + o_follower->zPos;
				if (!testObjectCollision1(o_follower, xFrom, zFrom, points[1].x, points[1].z, mask) || _varsTable[32] != 0) {
					for (int i = 0; i <= *count; i++) {
						points[i].x = points[i + 1].x;
						points[i].z = points[i + 1].z;
					}
					(*count)--;
					_currentObject->setData(specx, points[0].x);
					_currentObject->setData(specz, points[0].z);
				}
			}
		}
		break;
	}
	return -1;
}

int Game::op_translateObject2(int argc, int32_t *argv) {
	assert(argc == 3);
	debug(kDebug_OPCODES, "Game::op_translateObject2() [%d, %d, %d]", argv[0], argv[1], argv[2]);
	int32_t dx = argv[0];
	int32_t dy = argv[1];
	int32_t dz = argv[2];
	int xPos = _currentObject->xPos;
	int zPos = _currentObject->zPos;
	Vec_xz vec(dx, dz);
	vec.rotate(_currentObject->pitch, 15);
	xPos += vec.x;
	zPos += vec.z;
	_currentObject->xPos = xPos;
	_currentObject->yPos += dy;
	_currentObject->zPos = zPos;
	return -1;
}

int Game::op_updateCollidingHorizontalMask(int argc, int32_t *argv) {
	assert(argc == 2);
	debug(kDebug_OPCODES, "Game::op_updateCollidingHorizontalMask() [%d, %d]", argv[0], argv[1]);
	_currentObject->specialData[1][8] = getCollidingHorizontalMask(_currentObject->yPos, argv[0], argv[1]);
	return -1;
}

int Game::op_createParticle(int argc, int32_t *argv) {
	assert(argc == 7);
	debug(kDebug_OPCODES, "Game::op_createParticle() [%d, %d, %d, %d, %d, %d, %d]", argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]);
	int32_t objKey = argv[0];
	GameObject *o = (objKey == 0) ? _currentObject : getObjectByKey(objKey);
	if (!o) {
		return 0;
	}
	Vec_xz vec(argv[1], argv[3]);
	vec.rotate(o->pitch, kPosShift);
	const int dx = vec.x;
	const int dz = vec.z;
	const int dy = argv[2];
	addParticle(_particleDx, _particleDy, _particleDz, _particleRnd, dx, dy, dz, argv[4], argv[5], argv[6], _particleSpd);
	return -1;
}

int Game::op_setupFollowingObject(int argc, int32_t *argv) {
	assert(argc == 3);
	debug(kDebug_OPCODES, "Game::op_setupFollowingObject() [%d, %d, %d]", argv[0], argv[1], argv[2]);

	int16_t followerObjKey = argv[0];
	int16_t followedObjKey = argv[1];
	int16_t key = argv[2];

	GameObject *o_follower = (followerObjKey == 0) ? _currentObject : getObjectByKey(followerObjKey);
	if (!o_follower) {
		return 0;
	}
	GameObject *o_followed = (followedObjKey == 0) ? _currentObject : getObjectByKey(followedObjKey);
	if (!o_followed) {
		return 0;
	}

        GameFollowingPoint *points = _followingObjectsTable[o_follower->customData[11]].points;
	const int xTo = o_followed->xPosParent + o_followed->xPos;
	const int zTo = o_followed->zPosParent + o_followed->zPos;
	int count = 0;
	while (key > 0) {
		GameObject *o = getObjectByKey(key);
		if (!o) {
			return 0;
		}
		points[count].x = o->xPosParent + o->xPos;
		points[count].z = o->zPosParent + o->zPos;
		key = o->customData[0];
		++count;
	}
	o_follower->customData[10] = count;
        points[count].x = xTo;
        points[count].z = zTo;
	return -1;
}

int Game::op_isObjectCollidingPos(int argc, int32_t *argv) {
	assert(argc == 4);
	debug(kDebug_OPCODES, "Game::op_isObjectCollidingPos() [%d, %d, %d, %d]", argv[0], argv[1], argv[2], argv[3]);
	int32_t objKey = argv[0];
	int32_t xPos = argv[1];
	int32_t zPos = argv[2];
	int32_t mask = argv[3];
	GameObject *o = (objKey == 0) ? _currentObject : getObjectByKey(objKey);
	if (!o) {
		return 0;
	}
	int ret = 0;
	CollisionSlot2 slots2[65];
	slots2[0].box = -1;
	bool collidingTest = false;
	if (!(collidingTest = setCollisionSlotsUsingCallback2(o, xPos, zPos, &Game::collisionSlotCb3, mask, slots2))) {
		ret = -1;
		if (_updateGlobalPosRefObject && (_updateGlobalPosRefObject->flags[1] & 0x100) == 0) {
			_varsTable[14] = _updateGlobalPosRefObject->objKey;
		} else {
			_varsTable[14] = 0;
		}
	}
	if (!collidingTest && (o->flags[1] & 0x800) != 0 && getCellMapShr19(xPos, zPos)->isDoor) {
		ret = -1;
	}
	return ret;
}

int Game::op_setCameraObject(int argc, int32_t *argv) {
	assert(argc == 1);
	debug(kDebug_OPCODES, "Game::op_setCameraObject() [%d]", argv[0]);
	int32_t objKey = argv[0];
	GameObject *o = (objKey == 0) ? _currentObject : getObjectByKey(objKey);
	if (!o) {
		return 0;
	}
	int16_t key;
	if (setCameraObject(o, &key) || setCameraObject(_objectsPtrTable[kObjPtrConrad], &key)) {
		_cameraViewObj = key;
	}
	return -1;
}

int Game::op_setCameraParams(int argc, int32_t *argv) {
	assert(argc == 4);
	debug(kDebug_OPCODES, "Game::op_setCameraParams() [%d, %d, %d, %d]", argv[0], argv[1], argv[2], argv[3]);
	int32_t objKey = argv[0];
	GameObject *o = (objKey == 0) ? _currentObject : getObjectByKey(objKey);
	if (!o) {
		return 0;
	}
	if (_cameraState != 8) {
		warning("Game::op_setCameraParams() unimplemented %d %d %d obj '%s'", argv[1], argv[2], argv[3], o->name);
		return 0;
	}
	return -1;
}

int Game::op_setPlayerObject(int argc, int32_t *argv) {
	assert(argc == 1);
	debug(kDebug_OPCODES, "Game::op_setPlayerObject() [%d]", argv[0]);
	int32_t objKey = argv[0];
	GameObject *o = (objKey == 0) ? _currentObject : getObjectByKey(objKey);
	if (!o) {
		return 0;
	}
	if (o->objKey == _varsTable[30]) {
		return -1;
	}
	if ((o->flags[1] & 0x80) != 0 || (o->flags[1] & 0x4) != 0 || (o->flags[1] & 0x10000) == 0) {
		return 0;
	}
	_newPlayerObject = o->objKey;
	return -1;
}

int Game::op_isCollidingRooms(int argc, int32_t *argv) {
	assert(argc == 0);
	debug(kDebug_OPCODES, "Game::op_isCollidingRooms() []");
	GameObject *o = getObjectByKey(_varsTable[kVarViewpointObject]);
	const int x = (o->xPosParent + o->xPos + _xPosObserver) / 2;
	const int z = (o->zPosParent + o->zPos + _zPosObserver) / 2;
	const CellMap *cell1 = getCellMapShr19(_xPosObserver, _zPosObserver);
	const CellMap *cell2 = getCellMapShr19(x, z);
	if (cell1->room2 != 0) {
		for (CollisionSlot *colSlot = cell1->colSlot; colSlot; colSlot = colSlot->next) {
			if (colSlot->o == _currentObject) {
				return -1;
			}
		}
	}
	if (cell2->room2 != 0) {
		for (CollisionSlot *colSlot = cell2->colSlot; colSlot; colSlot = colSlot->next) {
			if (colSlot->o == _currentObject) {
				return -1;
			}
		}
	}
	return 0;
}

int Game::op_isMessageOnScreen(int argc, int32_t *argv) {
	assert(argc == 2);
	debug(kDebug_OPCODES, "Game::op_isMessageOnScreen() [%d, %d]", argv[0], argv[1]);
	int32_t objKey = argv[0];
	int32_t value = argv[1];
	int16_t msgObjKey = -1;
	if (objKey != -1) {
		GameObject *o = (objKey == 0) ? _currentObject : getObjectByKey(objKey);
		if (!o) {
			return 0;
		}
		msgObjKey = o->objKey;
	}
	for (int i = 0; i < kPlayerMessagesTableSize; ++i) {
		GamePlayerMessage *msg = &_playerMessagesTable[i];
		if (msg->desc.duration > 0 && (msgObjKey == -1 || (msgObjKey == msg->objKey && (value == -1 || value == msg->value)))) {
			return -1;
		}
	}
	return 0;
}

int Game::op_debugBreakpoint(int argc, int32_t *argv) {
	assert(argc == 2);
	debug(kDebug_OPCODES, "Game::op_debugBreakpoint() [%d, %d]", argv[0], argv[1]);
	// no-op
	return -1;
}

int Game::op_isObjectConradNotVisible(int argc, int32_t *argv) {
	assert(argc == 1);
	debug(kDebug_OPCODES, "Game::op_isObjectConradNotVisible() [%d]", argv[0]);
	const int objKey = argv[0];
	GameObject *o = (objKey == 0) ? _currentObject : getObjectByKey(objKey);
	return (o && o == _objectsPtrTable[kObjPtrConrad] && (o->specialData[1][20] & 15) == 5) ? -1 : 0;
}

int Game::op_stopSound(int argc, int32_t *argv) {
	assert(argc == 1);
	debug(kDebug_OPCODES, "Game::op_stopSound() [%d]", argv[0]);
	_snd.stopSfx(_currentObject->objKey, argv[0]);
	return -1;
}
