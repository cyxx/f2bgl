/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include "trigo.h"
#include "game.h"
#include "render.h"

void Game::updateObserverSinCos() {
	_yRotObserver &= 1023;
	_yInvRotObserver = (-_yRotObserver) & 1023;
	_yCosObserver = g_cos[_yRotObserver] * 2;
	_ySinObserver = g_sin[_yRotObserver] * 2;
	_yInvCosObserver = g_cos[_yInvRotObserver] * 2;
	_yInvSinObserver = g_sin[_yInvRotObserver] * 2;
}

void Game::rotateObserver(int ix, int iz, int *rx, int *rz) {
	const int xPos = (ix - _xPosObserver) >> 15;
	const int zPos = (iz - _zPosObserver) >> 15;
	const int rRot = -_yInvCosObserver * xPos - _yInvSinObserver * zPos;
	const int zRot =  _yInvSinObserver * xPos + _yInvCosObserver * zPos;
	*rx = rRot >> 18;
	*rz = zRot;
}

int Game::getClosestAngle(int a, int x, int z, int l, int r, int fl) {
	int x1, z1;
	if (fl == 0) {
		rotateObserver(x, z, &x1, &z1);
	} else {
		const int angle = a;
		int counter = 0;
		do {
			rotateObserver(x, z, &x1, &z1);
			if (z1 < 0) {
				a += 512;
			} else {
				if (x1 > 0) {
					a += 4;
				} else {
					a -= 4;
				}
			}
			_yRotObserver = a & 1023;
			_yCosObserver = g_cos[_yRotObserver] * 2;
			_ySinObserver = g_sin[_yRotObserver] * 2;
			_yInvSinObserver = -_ySinObserver;
			_yInvCosObserver =  _yCosObserver;
			++counter;
		} while (ABS(x1) > 4 && counter < 8);
		if (counter == 8) {
			warning("getClosestAngle() too many iterations for x1 %d angle %d,%d", x1, angle, a);
		}
	}
	a += x1;
	a &= 1023;
	l &= 1023;
	r &= 1023;
	if (l >= r) {
		if (a > l) {
			a = l;
		} else if (a < r) {
			a = r;
		}
	} else {
		if (a <= 512) {
			if (a > l) {
				a = l;
			}
		} else {
			if (a < r) {
				a = r;
			}
		}
	}
	return a;
}

void Game::setCameraPos(int xPos, int zPos) {
	int fl = 0;
	CellMap *cm = getCellMapShr19(xPos, zPos);
	if ((_currentCamera != cm->camera && _currentCamera != cm->camera2) || _prevCameraState != 4) {
		_prevCameraState = 4;
		_currentCamera = cm->camera;
		fl = 1;
	} else {
		_currentCamera = (_currentCamera == 255) ? 0 : _currentCamera;
	}
	const CameraPosMap *camera = &_sceneCameraPosTable[_currentCamera];
	_xPosObserver = camera->x;
	_zPosObserver = camera->z;
	if (fl == 1) {
		_yRotObserver = camera->ry;
	}
	_yRotObserver = getClosestAngle(_yRotObserver, xPos, zPos, camera->l_ry, camera->r_ry, fl);
}

void Game::getFixedCameraPos(int xPos, int zPos, int *xPosCam, int *zPosCam, int *yRotCam) {
	const CellMap *cm = getCellMapShr19(xPos, zPos);
	switch (_cameraNum) {
	case 1:
		_currentCamera = cm->camera;
		_cameraNum = 0;
		break;
	case 2:
		_currentCamera = cm->camera2;
		_cameraNum = 0;
		break;
	default:
		_currentCamera = cm->camera;
		break;
	}
	if (_currentCamera == 255 || (cm->camera != _currentCamera && cm->camera2 != _currentCamera)) {
		_currentCamera = cm->camera;
	}
	if (_currentCamera == 255) {
		_currentCamera = 0;
	}
	*xPosCam = _sceneCameraPosTable[_currentCamera].x;
	*zPosCam = _sceneCameraPosTable[_currentCamera].z;
	*yRotCam = _sceneCameraPosTable[_currentCamera].ry;
}

bool Game::setCameraObject(GameObject *o, int16_t *cameraObjKey) {
	*cameraObjKey = 0;
	for (int i = 0; i < kSceneObjectsTableSize; ++i) {
		if (_sceneObjectsTable[i].o == o) {
			*cameraObjKey = i;
			break;
		}
	}
	if (o->flags[1] & 0x84) {
		return false;
	}
	for (GameObject *o_tmp = o; o_tmp != _objectsPtrTable[kObjPtrWorld]; o_tmp = o_tmp->o_parent) {
		const int state = (o_tmp->state == 1 || o_tmp->state == 3);
		if (!state) {
			return false;
		}
	}
	const int prevCameraDefaultDist = _cameraDefaultDist;
	GameObject *o_camera = getObjectByKey(_cameraViewKey);
	if ((o_camera->flags[1] & 0x10000) != 0 && o_camera->customData[11] < _inputsCount) {
		//
	}
	_cameraViewKey = o->objKey;
	_xPosViewpoint = _x0PosViewpoint = o->xPosParent + o->xPos;
	_yPosViewpoint = _y0PosViewpoint = o->yPosParent + o->yPos;
	_zPosViewpoint = _z0PosViewpoint = o->zPosParent + o->zPos;
	_yRotViewpoint = o->pitch;
	_varsTable[31] = _cameraViewKey;
	if ((o->flags[1] & 0x10000) != 0 && o->customData[11] < _inputsCount) {
		//
	}
	_animDx = 0;
	_animDz = 0;
	if (_cameraDefaultDist != prevCameraDefaultDist) {
		_cameraDefaultDist = !_cameraDefaultDist;
		fixCamera();
	}
	return true;
}

void Game::fixCamera() {
	GameObject *o = getObjectByKey(_cameraViewKey);
	if ((_cameraViewKey == _objectsPtrTable[kObjPtrConrad]->objKey) || (o && o->specialData[1][21] == 2 && o->specialData[1][22] == 0x8000000)) {
		if (_cameraDefaultDist) {
			_cameraDeltaRotY = _cameraDeltaRotYValue2;
			_cameraDistValue = _cameraDistValue2;
			_yPosObserverValue = _yPosObserverValue2;
			_cameraDistTicks = 1;
			_yPosObserverTicks = 1;
			_cameraDeltaRotYTicks = 1;
			if (!isConradInShootingPos()) {
				_cameraDistValue = (16 * 2) << 15;
				_focalDistance = 0;
				_yPosObserverValue = _yPosObserver = 0;
			}
		} else {
			_cameraDeltaRotYValue2 = _cameraDeltaRotY;
			_cameraDistValue2 = _cameraDistValue;
			_yPosObserverValue2 = _yPosObserverValue;
			_cameraDistTicks = 2;
			_yPosObserverTicks = 2;
			_cameraDeltaRotYTicks = 2;
		}
		_cameraDistInitDone = true;
		_cameraDefaultDist = !_cameraDefaultDist;
		_cameraFixDist = _cameraDefaultDist;
		_yRotObserverPrev2 = _yRotObserverPrev = _yRotObserver;
	}
}

void Game::updateSceneCameraPos() {
	CollisionSlot2 slots2[65];
	int deltaPitch;
	int direction;
	int camsiny, camcosy;

	GameObject *o_cam = getObjectByKey(_cameraViewKey);
	GameObject *o_inp = getObjectByKey(_varsTable[kVarPlayerObject]);
	int input = getCurrentInput();

	const bool flag = o_inp != 0 && o_inp->specialData[1][21] == 2 && o_inp->specialData[1][22] == 0x8000000;

	if (_yRotObserverPrev != _yRotObserver) {
		_yRotObserverPrev2 = _yRotObserverPrev;
	}
	if (_cameraState == 2) {
		_yPosObserverValue = _yPosObserver = 0;
	} else if (_cameraState == 0 && !_cameraDefaultDist) {
		if (!flag && _level != 6 && !isConradInShootingPos()) {
			_cameraDistValue = _cameraDist = (16 * 2) << 15;
			_focalDistance = 0;
			_yPosObserverValue = _yPosObserver = -1;
			_yPosObserverPrev = 0;
		}
	}
	if (_cameraState == 0 && _cameraDefaultDist) {
		_xPosObserverPrev = _xPosObserver;
		_yPosObserverPrev = _yPosObserver;
		_zPosObserverPrev = _zPosObserver;
		_yRotObserverPrev = _yRotObserver;
		if (_cameraState == 0) {
			if (isConradInShootingPos() && isInputKeyDown(16384, input)) {
				int angle = (-o_cam->pitch) & 1023;
				const int y2CosTemp =  g_cos[angle & 1023];
				const int y2SinTemp = -g_sin[angle & 1023];
				angle = (-o_cam->pitch + 256) & 1023;
				const int y1CosTemp =  g_cos[angle & 1023];
				const int y1SinTemp = -g_sin[angle & 1023];
				int xTmp = _xPosViewpoint + fixedMul(y1SinTemp, 16 << 15, 15) + fixedMul(y2SinTemp, 6 << 15, 15);
				int zTmp = _zPosViewpoint + fixedMul(y1CosTemp, 16 << 15, 15) + fixedMul(y2CosTemp, 6 << 15, 15);
				_yRotObserver = angle;
				camcosy = y1CosTemp;
				camsiny = y1SinTemp;
				const CellMap *cell = getCellMapShr19(xTmp, zTmp);
				if (cell->type == 0) {
					_xPosObserver = xTmp;
					_zPosObserver = zTmp;
				} else {
					_xPosObserver = xTmp - fixedMul(y2SinTemp, 6 << 15, 15);
					_zPosObserver = zTmp - fixedMul(y2CosTemp, 6 << 15, 15);
				}
				return;
			}
			if (isConradInShootingPos() && isInputKeyDown(32768, input)) {
				int angle = (-o_cam->pitch) & 1023;
				const int y2CosTemp =  g_cos[angle & 1023];
				const int y2SinTemp = -g_sin[angle & 1023];
				angle = (-o_cam->pitch - 256) & 1023;
				const int y1CosTemp =  g_cos[angle & 1023];
				const int y1SinTemp = -g_sin[angle & 1023];
				int xTmp = _xPosViewpoint + fixedMul(y1SinTemp, 16 << 15, 15) + fixedMul(y2SinTemp, 6 << 15, 15);
				int zTmp = _zPosViewpoint + fixedMul(y1CosTemp, 16 << 15, 15) + fixedMul(y2CosTemp, 6 << 15, 15);
				_yRotObserver = angle;
				camcosy = y1CosTemp;
				camsiny = y1SinTemp;
				const CellMap *cell = getCellMapShr19(xTmp, zTmp);
				if (cell->type == 0) {
					_xPosObserver = xTmp;
					_zPosObserver = zTmp;
				} else {
					_xPosObserver = xTmp - fixedMul(y2SinTemp, 6 << 15, 15);
					_zPosObserver = zTmp - fixedMul(y2CosTemp, 6 << 15, 15);
				}
				return;
			}
			if (isConradInShootingPos() && isInputKeyDown(65536, input)) {
				_yRotObserver = (-o_cam->pitch - 512) & 1023;
				camcosy =  g_cos[_yRotObserver & 1023];
				camsiny = -g_sin[_yRotObserver & 1023];
				_xPosObserver = _xPosViewpoint + fixedMul(camsiny, 16 << 15, 15);
				_zPosObserver = _zPosViewpoint + fixedMul(camcosy, 16 << 15, 15);
				return;
			}
			if (isConradInShootingPos() && isInputKeyDown(131072, input)) {
				_yRotObserver = (-o_cam->pitch) & 1023;
				camcosy =  g_cos[_yRotObserver & 1023];
				camsiny = -g_sin[_yRotObserver & 1023];
				_xPosObserver = _xPosViewpoint + fixedMul(camsiny, 16 << 15, 15);
				_zPosObserver = _zPosViewpoint + fixedMul(camcosy, 16 << 15, 15);
				return;
			}
		}
		if (_cameraState >= 6) {
			_cameraState = 0;
		}
		int _yRotObserver_dest = -_yRotViewpoint + _cameraDeltaRotY;
		_yRotObserver_dest &= 1023;
		if (_cameraDistInitDone) {
			_yRotObserver = _yRotObserver_dest;
		} else {
			deltaPitch = getAngleDiff(_yRotObserver_dest, _yRotObserver) / _cameraDeltaRotYTicks;
			if (ABS(deltaPitch) < 4) {
				deltaPitch = 0;
			}
			_yRotObserver += deltaPitch;
			_yRotObserver &= 1023;
		}
		int _cameraDist_max;
		if (!flag && _level != 6) {
			_cameraDist_max = (32 + 16) << 15;
			_cameraDistTicks = 15;
		} else {
			_cameraDist_max = 16 << 15;
			_cameraDistTicks = 8;
		}
		_cameraDistValue = updateCameraDist(_xPosViewpoint, _zPosViewpoint, (-_yRotObserver + 512) & 1023, _cameraDist_max) - ((16 - 1) << 15);
		if (_cameraDistInitDone) {
			_cameraDist = _cameraDistValue;
		} else {
			if (_cameraDistValue > _cameraDist) {
				if (ABS(_cameraDistValue - _cameraDist) >> 12 < _cameraDistTicks) {
					_cameraDist = _cameraDistValue;
				} else {
					_cameraDist += (_cameraDistValue - _cameraDist) / _cameraDistTicks;
				}
			} else {
				_cameraDist = _cameraDistValue;
			}
		}
		int decal_y;
		if (flag) {
			decal_y = 0;
		} else if (_level == 6) {
			decal_y = 8;
		} else if (_varsTable[9] == 8 || _varsTable[9] == 7) {
			decal_y = 29;
		} else if (_varsTable[9] == 1) {
			decal_y = 21;
		} else {
			decal_y = 20;
		}
		if (!flag && _level != 6) {
			if (_cameraDist > ((kWallWidth * 2) << 15)) {
				_cameraDist = ((kWallWidth * 2) << 15);
			}
			const int tmp = fixedDiv(decal_y << 15, 15, (kWallWidth * 2) << 15);
			_yPosObserverValue = fixedMul(tmp, _cameraDist, 15) - (decal_y << 15);
		} else {
			_yPosObserverValue -= (decal_y << 15);
			if ((_yPosObserverValue >> 15) < -60) {
				_yPosObserverValue = -(60 << 15);
				_cameraDistInitDone = true;
			}
		}
		if (_cameraDistInitDone) {
			_yPosObserver = _yPosObserverValue;
			_observerColMask = getCollidingHorizontalMask(-_yPosObserver - (64 << 15), 1, 1);
		} else if (_yPosObserver != _yPosObserverValue) {
			if (ABS(_yPosObserverValue - _yPosObserver) >> 5 < _yPosObserverTicks) {
				_yPosObserver = _yPosObserverValue;
			} else {
				_yPosObserver += (_yPosObserverValue - _yPosObserver) / _yPosObserverTicks;
			}
			_observerColMask = getCollidingHorizontalMask (-_yPosObserver - (64 << 15), 1, 1);
		}
		if (!flag && _level != 6 && _cameraDist < -(12 << 15)) {
			_cameraDist = -(12 << 15);
		}
		camcosy =  g_cos[_yRotObserver & 1023];
		camsiny = -g_sin[_yRotObserver & 1023];
		_xPosObserver = _xPosViewpoint - fixedMul(camsiny, _cameraDist, 15);
		_zPosObserver = _zPosViewpoint - fixedMul(camcosy, _cameraDist, 15);
		_cameraDistInitDone = false;
		_xPosObserver = (_xPosObserver + _xPosObserverPrev) / 2;
		_zPosObserver = (_zPosObserver + _zPosObserverPrev) / 2;
	} else {
		_cameraDistInitDone = true;
		_xPosObserverPrev = _xPosObserver;
		_yPosObserverPrev = _yPosObserver;
		_zPosObserverPrev = _zPosObserver;
		_yRotObserverPrev = _yRotObserver;
		if (!_cameraDefaultDist && _yPosObserver != _yPosObserverValue) {
			if (ABS(_yPosObserverValue - _yPosObserver) >> 5 < _yPosObserverTicks) {
				_yPosObserver = _yPosObserverValue;
			} else {
				_yPosObserver += (_yPosObserverValue - _yPosObserver) / _yPosObserverTicks;
			}
			_observerColMask = getCollidingHorizontalMask (-_yPosObserver - (64 << 15), 1, 1);
		}
		if (!_cameraDefaultDist && isInputKeyDown(16384, input)) {
			int angle = (-o_cam->pitch) & 1023;
			const int y2CosTemp =  g_cos[angle & 1023];
			const int y2SinTemp = -g_sin[angle & 1023];
			angle = (-o_cam->pitch + 256) & 1023;
			const int y1CosTemp =  g_cos[angle & 1023];
			const int y1SinTemp = -g_sin[angle & 1023];
			int xTmp = _xPosViewpoint + fixedMul(y1SinTemp, 16 << 15, 15) + fixedMul(y2SinTemp, 6 << 15, 15);
			int zTmp = _zPosViewpoint + fixedMul(y1CosTemp, 16 << 15, 15) + fixedMul(y2CosTemp, 6 << 15, 15);
			_yRotObserver = angle;
			camcosy = y1CosTemp;
			camsiny = y1SinTemp;
			const CellMap *cell = getCellMapShr19(xTmp, zTmp);
			if (cell->type == 0) {
				_xPosObserver = xTmp;
				_zPosObserver = zTmp;
			} else {
				_xPosObserver = xTmp - fixedMul(y2SinTemp, 6 << 15, 15);
				_zPosObserver = zTmp - fixedMul(y2CosTemp, 6 << 15, 15);
			}
			_yRotObserverPrev2 = _yRotObserver - 1;
			return;
		}
		if (!_cameraDefaultDist && isInputKeyDown(32768, input)) {
			int angle = (-o_cam->pitch) & 1023;
			const int y2CosTemp =  g_cos[angle & 1023];
			const int y2SinTemp = -g_sin[angle & 1023];
			angle = (-o_cam->pitch - 256) & 1023;
			const int y1CosTemp =  g_cos[angle & 1023];
			const int y1SinTemp = -g_sin[angle & 1023];
			int xTmp = _xPosViewpoint + fixedMul(y1SinTemp, 16 << 15, 15) + fixedMul(y2SinTemp, 6 << 15, 15);
			int zTmp = _zPosViewpoint + fixedMul(y1CosTemp, 16 << 15, 15) + fixedMul(y2CosTemp, 6 << 15, 15);
			_yRotObserver = angle;
			camcosy = y1CosTemp;
			camsiny = y1SinTemp;
			const CellMap *cell = getCellMapShr19(xTmp, zTmp);
			if (cell->type == 0) {
				_xPosObserver = xTmp;
				_zPosObserver = zTmp;
			} else {
				_xPosObserver = xTmp - fixedMul(y2SinTemp, 6 << 15, 15);
				_zPosObserver = zTmp - fixedMul(y2CosTemp, 6 << 15, 15);
			}
			_yRotObserverPrev2 = _yRotObserver - 1;
			return;
		}
		if (!_cameraDefaultDist && isInputKeyDown(65536, input)) {
			_yRotObserver = (-o_cam->pitch - 512) & 1023;
			camcosy =  g_cos[_yRotObserver & 1023];
			camsiny = -g_sin[_yRotObserver & 1023];
			_xPosObserver = _xPosViewpoint + fixedMul(camsiny, 16 << 15, 15);
			_zPosObserver = _zPosViewpoint + fixedMul(camcosy, 16 << 15, 15);
			_yRotObserverPrev2 = _yRotObserver - 1;
			return;
		}
		if (!_cameraDefaultDist && isInputKeyDown(131072, input)) {
			_yRotObserver = (-o_cam->pitch) & 1023;
			camcosy =  g_cos[_yRotObserver & 1023];
			camsiny = -g_sin[_yRotObserver & 1023];
			_xPosObserver = _xPosViewpoint + fixedMul(camsiny, 16 << 15, 15);
			_zPosObserver = _zPosViewpoint + fixedMul(camcosy, 16 << 15, 15);
			_yRotObserverPrev2 = _yRotObserver - 1;
			return;
		}
		if (_cameraState >= 6) {
			_cameraState = 0;
		}
		if (_cameraDistValue != _cameraDist) {
			if ((ABS(_cameraDistValue - _cameraDist) >> 5) < _cameraDistTicks) {
				_cameraDist = _cameraDistValue;
			} else {
				_cameraDist += (_cameraDistValue - _cameraDist) / _cameraDistTicks;
			}
		}
		switch (_cameraState) {
		case 0:
			if (_level == 12) {
				_xPosObserver = _xPosViewpoint - fixedMul(-g_sin[_yRotViewpoint & 1023], _cameraDist, 15);
				_zPosObserver = _zPosViewpoint - fixedMul( g_cos[_yRotViewpoint & 1023], _cameraDist, 15);
				_yRotObserver = 0;
				direction = 1;
			} else {
				direction = getCameraAngle(_xPosViewpoint, _zPosViewpoint, -_yRotViewpoint + _cameraDeltaRotY, &_xPosObserver, &_zPosObserver, &_yRotObserver);
			}
			if (_yPosObserver != _yPosObserverValue) {
				if (ABS(_yPosObserverValue - _yPosObserver) >> 5 < _yPosObserverTicks) {
					_yPosObserver = _yPosObserverValue;
				} else {
					_yPosObserver += (_yPosObserverValue - _yPosObserver) / _yPosObserverTicks;
				}
				_observerColMask = getCollidingHorizontalMask (-_yPosObserver - (64 << 15), 1, 1);
			}
			_yRotObserver &= 1023;
			_pitchObserverCamera &= 1023;
			switch (direction) {
			case 0:
				setCameraPos(_xPosViewpoint, _zPosViewpoint);
				_pitchObserverCamera = _yRotObserver;
				break;
			case 1:
			case 2:
				if (_pitchObserverCamera == _yRotObserver) {
					break;
				}
				deltaPitch = getAngleDiff(_yRotObserver, _pitchObserverCamera) / _cameraDeltaRotYTicks;
				if (ABS(deltaPitch) < 4) {
					deltaPitch = 0;
				}
				_pitchObserverCamera += deltaPitch;
				_pitchObserverCamera &= 1023;
				slots2[0].box = -1;
				if (testCameraPos(_xPosViewpoint, _zPosViewpoint, _pitchObserverCamera, &_xPosObserver, &_zPosObserver, slots2)) {
					_yRotObserver = _pitchObserverCamera;
				}
				_pitchObserverCamera = _yRotObserver;
				break;
			}
			break;
		case 1:
			_yRotObserver = (-_sceneObjectsTable[_cameraViewObj].pitch + _cameraDeltaRotY) & 1023;
			_yCosObserver =  g_cos[_yRotObserver];
			_ySinObserver = -g_sin[_yRotObserver];
			_xPosObserver = _xPosViewpoint;
			_zPosObserver = _zPosViewpoint;
			_xPosObserver -= _ySinObserver * (_cameraDist >> 15);
			_zPosObserver -= _yCosObserver * (_cameraDist >> 15);
			break;
		case 2:
			_x0PosViewpoint = _xPosViewpoint = o_cam->xPos;
			_y0PosViewpoint = _yPosViewpoint = o_cam->yPos;
			_z0PosViewpoint = _zPosViewpoint = o_cam->zPos;
			_yRotViewpoint = o_cam->pitch;
			_cameraDist = _cameraDistValue;
			_fixedViewpoint = 1;
			deltaPitch = 0;
			getFixedCameraPos(_xPosViewpoint, _zPosViewpoint, &_xPosObserver, &_zPosObserver, &_yRotObserver);
			_xPosObserverPrev = _xPosObserver;
			_yPosObserverPrev = _yPosObserver;
			_zPosObserverPrev = _zPosObserver;
			_yRotObserverPrev = _yRotObserver;
			_yRotViewpoint = _yRotObserver;
			break;
		case 3:
			_yRotObserver = _sceneCameraPosTable[_currentCamera].ry;
			_xPosObserver = _sceneCameraPosTable[_currentCamera].x;
			_zPosObserver = _sceneCameraPosTable[_currentCamera].z;
			break;
		case 4:
			setCameraPos(_xPosViewpoint, _zPosViewpoint);
			break;
		case 5:
			_yRotObserver = (-_sceneObjectsTable[_cameraViewObj].pitch + _cameraDeltaRotY) & 1023;
			_yCosObserver =  g_cos[_yRotObserver];
			_ySinObserver = -g_sin[_yRotObserver];
			_xPosObserver = _xPosViewpoint;
			_zPosObserver = _zPosViewpoint;
			_xPosObserver -= _ySinObserver * (_cameraDist >> 15);
			_zPosObserver -= _yCosObserver * (_cameraDist >> 15);
			break;
		}
	}
	const int dx = 0;
	const int dz = (_zTransform * _focalDistance) << kPosShift;
	_render->setCameraPos(_xPosObserver + dx, _yPosObserver, _zPosObserver + dz, kPosShift);
	_render->setCameraPitch(_yRotObserver & 1023);
}

int Game::testCameraRay (int x, int z, int ry) {
	GameObject tmpObj;
	tmpObj.objKey = _cameraViewKey;
	tmpObj.xPosParent = 0;
	tmpObj.zPosParent = 0;
	tmpObj.xPos = x;
	tmpObj.zPos = z;
	tmpObj.specialData[1][8] = -2;
	tmpObj.pitch = -ry;
	CellMap tmpCell;
	const int rep = rayCastCamera(&tmpObj, 0, &tmpCell, &Game::rayCastCameraCb2);
	return (rep == _cameraViewKey);
}

int Game::updateCameraDist(int x, int z, int ry, int maxDistance) {
	GameObject tmpObj;
	tmpObj.xPosParent = _objectsPtrTable[kObjPtrConrad]->xPosParent;
	tmpObj.zPosParent = _objectsPtrTable[kObjPtrConrad]->zPosParent;
	tmpObj.specialData[1][21] = 1;
	tmpObj.specialData[1][8] = -512;
	tmpObj.pitch = ry;
	tmpObj.o_parent = _objectsPtrTable[kObjPtrConrad];
	tmpObj.xPos = x;
	tmpObj.zPos = z;
	CellMap tmpCell;
	rayCastCamera(&tmpObj, 0, &tmpCell, &Game::rayCastCameraCb1);
	const int xTmp = (x - tmpObj.xPos) >> 15;
	const int zTmp = (z - tmpObj.zPos) >> 15;
	const int dist = fixedSqrt(zTmp * zTmp + xTmp * xTmp);
	if (dist < (maxDistance >> 15)) {
		return dist << 15;
	} else {
		return maxDistance;
	}
}

int Game::testCameraPos(int xRef, int zRef, int pitchRef, int * retx, int * retz, CollisionSlot2 * slots2) {
	const int camcosy =  g_cos[pitchRef & 1023];
	const int camsiny = -g_sin[pitchRef & 1023];
	int distx, distz;
	if (_cameraDist < 0) {
		distx = -fixedMul(camsiny, _cameraDist + (16 << 15) + (0 << 15), 15);
		distz = -fixedMul(camcosy, _cameraDist + (16 << 15) + (0 << 15), 15);
	} else {
		distx = -fixedMul(camsiny, _cameraDist + (16 << 15) + (2 << 15), 15);
		distz = -fixedMul(camcosy, _cameraDist + (16 << 15) + (2 << 15), 15);
	}
	int xstart = xRef;
	int zstart = zRef;
	int camx = xRef + distx;
	int camz = zRef + distz;
	int k = 1 << 4;
	static const int k2 = 1 << (4 - 1);
	int stepx = -(distx >> 4);
	int stepz = -(distz >> 4);
	int x = xstart + distx;
	int z = zstart + distz;
	int m = 0;
	while (k--) {
		if (k == 0) {
			x = xstart;
			z = zstart;
		} else if (k == k2) {
			x = xstart + (distx >> 1);
			z = zstart + (distz >> 1);
		}
		const int cx = x >> 19;
		const int cz = z >> 19;
		int box = cx | (cz << 8);
		if (slots2[m].box != box) {
			for (m = 0; m < 64; m++) {
				if (slots2[m].box == -1) {
					if (!checkCellMap(cx << 19, cz << 19)) {
						return 0;
					}
					slots2[m].box = box;
					CellMap *cell = slots2[m].cell = getCellMap(cx, cz);
					slots2[m].type = cell->type;
					slots2[m + 1].box = -1;
					break;
				} else if (slots2[m].box == box) {
					box = -1;
					break;
				}
			}
		}
		if (slots2[m].type != 0 && slots2[m].type != 32) {
			return 0;
		}
		x += stepx;
		z += stepz;
	}
	if (checkCellMap(camx, camz)) {
		CellMap *cell = getCellMap(camx >> 19, camz >> 19);
		if (cell->type == 0 && testCameraRay (camx, camz, pitchRef)) {
			*retx = xRef - fixedMul(camsiny, _cameraDist, 15);
			*retz = zRef - fixedMul(camcosy, _cameraDist, 15);
			return 1;
		}
	}
	return 0;
}

int Game::getCameraAngle(int xRef, int zRef, int pitchRef, int *x, int *z, int *pitch) {
	CollisionSlot2 slots2[65];
	slots2[0].box = -1;
	int leftPitch = 0;
	int rightPitch = 1;
	int retLeftPitch = -1;
	int retRightPitch = -1;
	do {
		if (testCameraPos(xRef, zRef, pitchRef - leftPitch, x, z, slots2)) {
			retLeftPitch = pitchRef - leftPitch;
		}
		if (testCameraPos(xRef, zRef, pitchRef + rightPitch, x, z, slots2)) {
			retRightPitch = pitchRef + rightPitch;
		}
		if (retLeftPitch != -1 || retRightPitch != -1) {
			if (_yRotObserverPrev2 == (retLeftPitch & 1023)) {
				if (testCameraPos(xRef, zRef, _yRotObserverPrev, x, z, slots2)) {
					*pitch = _yRotObserverPrev;
					return 1;
				}
				retLeftPitch = retRightPitch = -1;
			} else if (_yRotObserverPrev2 == (retRightPitch & 1023)) {
				if (testCameraPos(xRef, zRef, _yRotObserverPrev, x, z, slots2)) {
					*pitch = _yRotObserverPrev;
					return 2;
				}
				retLeftPitch = retRightPitch = -1;
			} else if (retLeftPitch != -1) {
				*pitch = retLeftPitch;
				return 1;
			} else {
				*pitch = retRightPitch;
				return 2;
			}
		}
		++leftPitch;
		++rightPitch;
	} while (leftPitch <= 512);
	return 0;
}
