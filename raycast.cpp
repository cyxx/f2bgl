/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include "game.h"
#include "trigo.h"

static const int kDepth = 16;
static const int kRayShift = 22;
static const int kFracShift = 16;

static int _dxRay, _dzRay;
static int _xPosRay, _zPosRay;
static int _xStepDistance, _zStepDistance;
static int _resXRayX, _resZRayX, _resXRayZ, _resZRayZ;
static int _zRayStepX, _zRayStepZ, _xRayStepX, _xRayStepZ;
static int _xRayMask, _zRayMask;
static bool _xTransparent, _zTransparent;

void Game::rayCastInit(int sx) {
	const int xb = ((sx - (kScreenWidth / 2)) << 1) - 1;
	const int zb = (256 - kDepth) << 1;
	const int rxa =  _ySinObserver << 5;
	const int rza = -_yCosObserver << 5;
	const int rxb = (_yCosObserver * xb) - (_ySinObserver * zb);
	const int rzb = (_ySinObserver * xb) + (_yCosObserver * zb);
	int dx = (rxb - rxa) >> 2;
	int dz = (rzb - rza) >> 2;
	static const int kMin = fixedInt(1, kFracShift);
	static const int kMax = fixedInt(1024, kFracShift) - 1;
	if (dx > 0) {
		if (dx < kMin) {
			dx = kMin;
		} else if (dx > kMax) {
			dx = kMax;
		}
	} else {
		if (dx > -kMin) {
			dx = -kMin;
		} else if (dx < -kMax) {
			dx = -kMax;
		}
	}
	if (dz > 0) {
		if (dz < kMin) {
			dz = kMin;
		} else if (dz > kMax) {
			dz = kMax;
		}
	} else {
		if (dz > -kMin) {
			dz = -kMin;
		} else if (dz < -kMax) {
			dz = -kMax;
		}
	}

	_zRayStepX = fixedInt(1, kRayShift);
	_zRayStepZ = fixedDiv(dz, kRayShift, dx);
	if (dx < 0) {
		_zRayStepX = -_zRayStepX;
		_zRayStepZ = -_zRayStepZ;
	}
	if (_zRayStepX > 0) {
		_zRayMask = -1;
	} else {
		_zRayMask = 0;
	}

	_xRayStepZ = fixedInt(1, kRayShift);
	_xRayStepX = fixedDiv(dx, kRayShift, dz);
	if (dz < 0) {
		_xRayStepZ = -_xRayStepZ;
		_xRayStepX = -_xRayStepX;
	}
	if (_xRayStepZ > 0) {
		_xRayMask = 0;
	} else {
		_xRayMask = -1;
	}

	_xStepDistance = fixedMul(_yInvSinObserver, _xRayStepX, kFracShift);
	_xStepDistance += fixedMul(_yInvCosObserver, _xRayStepZ, kFracShift);
	if (_xStepDistance < 0) {
		_xStepDistance = -_xStepDistance;
	}
	_zStepDistance = fixedMul(_yInvSinObserver, _zRayStepX, kFracShift);
	_zStepDistance += fixedMul(_yInvCosObserver, _zRayStepZ, kFracShift);
	if (_zStepDistance < 0) {
		_zStepDistance = -_zStepDistance;
	}
}

static bool get2dIntersection(int data, int ex, int ez, int stepx, int stepz, int *x, int *z) {
	int cx = *x;
	int cz = *z;
	const int dx = (ex << kRayShift) + (data << (kRayShift - 6));
	const int dz = (stepx > 0) ? stepz : -stepz;
	cz += fixedMul(dz, (dx - cx), kRayShift);
	if ((cz >> kRayShift) == ez) {
		*x = dx;
		*z = cz;
		return true;
	}
	return false;
}

static bool getRayIntersection(int xRef, int zRef, int x1, int z1, int x2, int z2, int *dstX, int *dstZ) {
	bool intersects = false;
	int nearestX = 0;
	int nearestZ = 0;
	if (_zRayStepX > 0) {
		if (_zRayStepZ > 0) {
			intersects = (x2 > xRef) && (z2 > zRef);
		} else {
			intersects = (x2 > xRef) && (z1 < zRef);
		}
	} else {
		if (_zRayStepZ > 0) {
			intersects = (x1 < xRef) && (z2 > zRef);
		} else {
			intersects = (x1 < xRef) && (z1 < zRef);
		}
	}
	int ix, iz;
	if (intersects) {
		intersects = false;
		int minDist = 0x7FFFFFFF;
		iz = z1;
		ix = (_xRayStepX >> (kRayShift - 15)) * (iz >> 15);
		if (_xRayStepZ < 0) {
			ix = -ix;
		}
		if (ix >= x1 && ix <= x2) {
			intersects = true;
			nearestX = ix;
			nearestZ = iz;
			minDist = ((ix - xRef) >> 15) * ((ix - xRef) >> 15) + ((iz - zRef) >> 15) * ((iz - zRef) >> 15);
		}
		iz = z2;
		ix = (_xRayStepX >> (kRayShift - 15)) * (iz >> 15);
		if (_xRayStepZ < 0) {
			ix = -ix;
		}
		if (ix >= x1 && ix <= x2) {
			intersects = true;
			const int squareDist = ((ix - xRef) >> 15) * ((ix - xRef) >> 15) + ((iz - zRef) >> 15) * ((iz - zRef) >> 15);
			if (squareDist < minDist) {
				minDist = squareDist;
				nearestX = ix;
				nearestZ = iz;
			}
		}
		ix = x1;
		iz = (_zRayStepZ >> (kRayShift - 15)) * (ix >> 15);
		if (_zRayStepX < 0) {
			iz = -iz;
		}
		if (iz >= z1 && iz <= z2) {
			intersects = true;
			const int squareDist = ((ix - xRef) >> 15) * ((ix - xRef) >> 15) + ((iz - zRef) >> 15) * ((iz - zRef) >> 15);
			if (squareDist < minDist) {
				minDist = squareDist;
				nearestX = ix;
				nearestZ = iz;
			}
		}
		ix = x2;
		iz = (_zRayStepZ >> (kRayShift - 15)) * (ix >> 15);
		if (_zRayStepX < 0) {
			iz = -iz;
		}
		if (iz >= z1 && iz <= z2) {
			intersects = true;
			const int squareDist = ((ix - xRef) >> 15) * ((ix - xRef) >> 15) + ((iz - zRef) >> 15) * ((iz - zRef) >> 15);
			if (squareDist < minDist) {
				minDist = squareDist;
				nearestX = ix;
				nearestZ = iz;
			}
		}
	}
	*dstX = nearestX;
	*dstZ = nearestZ;
	return intersects;
}

int Game::rayCastCollisionCb1(GameObject *o, CellMap *cell, int ox, int oz) {
	CollisionSlot *slot = cell->colSlot;
	const int xOffset = (_xPosRay >> (kRayShift - 19));
	const int zOffset = (_zPosRay >> (kRayShift - 19));
	const int xRef = o->xPosParent + o->xPos - xOffset;
	const int zRef = o->zPosParent + o->zPos - zOffset;
	while (slot) {
		GameObject *obj = slot->o;
		if ((obj->flags[1] & 0x100) != 0 || (obj != o && obj != o->o_parent && obj->specialData[1][21] & o->specialData[1][21])) {
			if (((obj->specialData[1][21] == 16) && (o->specialData[1][8] & obj->specialData[1][8])) || ((obj->specialData[1][21] != 16) && (0x10 & obj->specialData[1][8]))) {
				int x2 = obj->xPosParent + obj->xPos - xOffset;
				int z2 = obj->zPosParent + obj->zPos - zOffset;
				int x1 = x2 + obj->xFrm1;
				int z1 = z2 + obj->zFrm1;
				x2 += obj->xFrm2;
				z2 += obj->zFrm2;
				int ix, iz;
				if (getRayIntersection(xRef, zRef, x1, z1, x2, z2, &ix, &iz)) {
					if (obj->flags[1] & 0x100) {
						ix += xOffset;
						iz += zOffset;
						o->xPos = ix - o->xPosParent;
						o->zPos = iz - o->zPosParent;
						return -1;
					} else {
						o->xPos = obj->xPos + obj->xPosParent - o->xPosParent;
						o->zPos = obj->zPos + obj->zPosParent - o->zPosParent;
						return obj->objKey;
					}
				}
			}
		}
		slot = slot->next;
	}
	return 0;
}

int Game::rayCastCollisionCb2(GameObject *o, CellMap *cell, int ox, int oz) {
	const int xOffset = _xPosRay >> (kRayShift - 19);
	const int zOffset = _zPosRay >> (kRayShift - 19);
	CollisionSlot *slot = cell->colSlot;
	const int xRef = o->xPosParent + o->xPos - xOffset;
	const int zRef = o->zPosParent + o->zPos - zOffset;
	while (slot) {
		GameObject *obj = slot->o;
		if (obj->specialData[1][23] != 57 || (obj->flags[1] & 0x84) == 0) {
			if ((obj->flags[1] & 0x100) != 0 || (obj != o && obj != o->o_parent && obj->specialData[1][21] != 8)) {
				if (obj->specialData[1][8] & -2) {
					int x2 = obj->xPosParent + obj->xPos - xOffset;
					int z2 = obj->zPosParent + obj->zPos - zOffset;
					int x1 = x2 + obj->xFrm1;
					int z1 = z2 + obj->zFrm1;
					x2 += obj->xFrm2;
					z2 += obj->zFrm2;
					int ix, iz;
					if (getRayIntersection(xRef, zRef, x1, z1, x2, z2, &ix, &iz)) {
						int i = 0;
						while (i < _rayCastedObjectsCount && _rayCastedObjectsTable[i].o != obj) {
							++i;
						}
						if (i >= _rayCastedObjectsCount) {
							ix += xOffset;
							iz += zOffset;
							_rayCastedObjectsTable[_rayCastedObjectsCount].o = obj;
							_rayCastedObjectsTable[_rayCastedObjectsCount].x = ix - o->xPosParent;
							_rayCastedObjectsTable[_rayCastedObjectsCount].z = iz - o->zPosParent;
							_rayCastedObjectsCount++;
						}
					}
				}
			}
		}
		slot = slot->next;
	}
	return 0;
}

int Game::rayCastCameraCb1(GameObject *o, CellMap *cell, int ox, int oz) {
	CollisionSlot *slot = cell->colSlot;
	const int xRef = o->xPosParent + o->xPos - (_xPosRay >> (kRayShift - 19));
	const int zRef = o->zPosParent + o->zPos - (_zPosRay >> (kRayShift - 19));
	while (slot) {
		GameObject *obj = slot->o;
		if ((obj != o) && (obj != o->o_parent) && (obj->specialData[1][21] & o->specialData[1][21])) {
			if (o->specialData[1][8] & obj->specialData[1][8]) {
				int x = obj->xPosParent + obj->xPos - (_xPosRay >> (kRayShift - 19));
				int z = obj->zPosParent + obj->zPos - (_zPosRay >> (kRayShift - 19));
				int x1 = x + obj->xFrm1;
				int z1 = z + obj->zFrm1;
				int x2 = x + obj->xFrm2;
				int z2 = z + obj->zFrm2;
				int ix, iz;
				if (getRayIntersection(xRef, zRef, x1, z1, x2, z2, &ix, &iz)) {
					ix += (_xPosRay >> (kRayShift - 19));
					iz += (_zPosRay >> (kRayShift - 19));
					o->xPos = ix;
					o->zPos = iz;
					return (obj->objKey);
				}
			}
		}
		slot = slot->next;
	}
	return 0;
}

int Game::rayCastCameraCb2(GameObject * o, CellMap * cell, int ox, int oz) {
	const int xRef = o->xPosParent + o->xPos - (_xPosRay >> (kRayShift - 19));
	const int zRef = o->zPosParent + o->zPos - (_zPosRay >> (kRayShift - 19));
	if (cell->type == -3) {
		return 0;
	}
	bool foundObject = false;
	CollisionSlot *slot = cell->colSlot;
	while (slot != 0) {
		if (slot->o == getObjectByKey(_cameraViewKey)) {
			foundObject = true;
			break;
		}
		slot = slot->next;
	}
	if (foundObject) {
		slot = cell->colSlot;
		while (slot != 0) {
			GameObject *obj = slot->o;
			bool flag = (obj != getObjectByKey(_cameraViewKey));
			flag = flag && (obj->flags[1] & 4) == 0;
			flag = flag && (obj->specialData[1][23] != 57);
			flag = flag && (obj->specialData[1][8] & _observerColMask);
			if (flag) {
				int x = obj->xPosParent + obj->xPos - (_xPosRay >> (kRayShift - 19));
				int z = obj->zPosParent + obj->zPos - (_zPosRay >> (kRayShift - 19));
				int x1 = x + obj->xFrm1;
				int z1 = z + obj->zFrm1;
				int x2 = x + obj->xFrm2;
				int z2 = z + obj->zFrm2;
				int ix, iz;
				if (getRayIntersection(xRef, zRef, x1, z1, x2, z2, &ix, &iz)) {
					ix >>= 15;
					iz >>= 15;
					const int dist = (ix * ix) + (iz * iz);
					if (dist < ((_cameraDist + (kDepth << 15)) >> 15) * ((_cameraDist + (kDepth << 15)) >> 15)) {
						return obj->objKey;
					}
				}
			}
			slot = slot->next;
		}
		return o->objKey;
	} else {
		slot = cell->colSlot;
		while (slot != 0) {
			GameObject *obj = slot->o;
			if ((obj->specialData[1][8] & _observerColMask) && (((obj->flags[1] & 4) == 0) || (obj->specialData[1][23] != 57))) {
				int x = obj->xPosParent + obj->xPos - (_xPosRay >> (kRayShift - 19));
				int z = obj->zPosParent + obj->zPos - (_zPosRay >> (kRayShift - 19));
				int x1 = x + obj->xFrm1;
				int z1 = z + obj->zFrm1;
				int x2 = x + obj->xFrm2;
				int z2 = z + obj->zFrm2;
				int ix, iz;
				if (getRayIntersection(xRef, zRef, x1, z1, x2, z2, &ix, &iz)) {
					return (obj->objKey);
				}
			}
			slot = slot->next;
		}
		return 0;
	}
}

enum {
	kRayCastMono,
	kRayCastCamera,
	kRayCastWall,
};

static int testRayX(int sx, CellMap *cell, int x, int z, int ex, int ez, int type) {
	_xTransparent = false;
	if (type == kRayCastWall) {
		switch (cell->type) {
		case 10:
			if (get2dIntersection(cell->data[0], ex, ez, _zRayStepX, _zRayStepZ, &x, &z)) {
				int num = ((z >> kFracShift) ^ _zRayMask) & ((kWallWidth << 2) - 1);
				if ((num >> 2) & 1) {
					return 0;
				}
				_resXRayX = x;
				_resZRayX = z;
				return 1;
			}
			return 0;
		case 11:
			if (get2dIntersection(cell->data[0], ez, ex, _xRayStepZ, _xRayStepX, &z, &x)) {
				int num = ((x >> kFracShift) ^ _xRayMask) & ((kWallWidth << 2) - 1);
				if ((num >> 2) & 1) {
					return 0;
				}
				_resXRayX = x;
				_resZRayX = z;
				return 1;
			}
			return 0;
		case 16:
			if (get2dIntersection(cell->data[0], ex, ez, _zRayStepX, _zRayStepZ, &x, &z)) {
				int num = (z >> kFracShift) & ((kWallWidth << 2) - 1);
				if (num > cell->data[1]) {
					return 0;
				}
				_xTransparent = true;
				return 0;
			}
			return 0;
		case 17:
			if (get2dIntersection(cell->data[0], ex, ez, _zRayStepX, _zRayStepZ, &x, &z)) {
				int num = (z >> kFracShift) & ((kWallWidth << 2) - 1);
				if (num < 63 - cell->data[1]) {
					return 0;
				}
				_xTransparent = true;
				return 0;
			}
			return 0;
		case 18:
			if (get2dIntersection(cell->data[0], ez, ex, _xRayStepZ, _xRayStepX, &z, &x)) {
				int num = (x >> kFracShift) & ((kWallWidth << 2) - 1);
				if (num > cell->data[1]) {
					return 0;
				}
				_xTransparent = true;
				return 0;
			}
			return 0;
		case 19:
			if (get2dIntersection(cell->data[0], ez, ex, _xRayStepZ, _xRayStepX, &z, &x)) {
				int num = (x >> kFracShift) & ((kWallWidth << 2) - 1);
				if (num < 63 - cell->data[1]) {
					return 0;
				}
				_xTransparent = true;
				return 0;
			}
			return 0;
		}
	}
	switch (cell->type) {
	case 2:
		if (get2dIntersection(cell->data[0], ex, ez, _zRayStepX, _zRayStepZ, &x, &z)) {
			_resXRayX = x;
			_resZRayX = z;
			return 1;
		}
		break;
	case 3:
		if (get2dIntersection(cell->data[0], ez, ex, _xRayStepZ, _xRayStepX, &z, &x)) {
			_resXRayX = x;
			_resZRayX = z;
			return 1;
		}
		break;
	case 4:
	case 16:
		if (get2dIntersection(cell->data[0], ex, ez, _zRayStepX, _zRayStepZ, &x, &z)) {
			const int num = (z >> kFracShift) & ((kWallWidth << 2) - 1);
			if (num > cell->data[1]) {
				return 0;
			}
			_resXRayX = x;
			_resZRayX = z;
			return 1;
		}
		break;
	case 5:
	case 17:
		if (get2dIntersection(cell->data[0], ex, ez, _zRayStepX, _zRayStepZ, &x, &z)) {
			const int num = (z >> kFracShift) & ((kWallWidth << 2) - 1);
			if (num < 63 - cell->data[1]) {
				return 0;
			}
			_resXRayX = x;
			_resZRayX = z;
			return 1;
		}
		break;
	case 6:
	case 18:
		if (get2dIntersection(cell->data[0], ez, ex, _xRayStepZ, _xRayStepX, &z, &x)) {
			const int num = (x >> kFracShift) & ((kWallWidth << 2) - 1);
			if (num > cell->data[1]) {
				return 0;
			}
			_resXRayX = x;
			_resZRayX = z;
			return 1;
		}
		break;
	case 7:
	case 19:
		if (get2dIntersection(cell->data[0], ez, ex, _xRayStepZ, _xRayStepX, &z, &x)) {
			const int num = (x >> kFracShift) & ((kWallWidth << 2) - 1);
			if (num < 63 - cell->data[1]) {
				return 0;
			}
			_resXRayX = x;
			_resZRayX = z;
			return 1;
		}
		break;
	case 10:
		return 0;
	case 11:
		return 0;
	case 20:
		return 1;
	case 32:
		return 0;
	}
	return 0;
}

static int testRayZ(int sx, CellMap *cell, int x, int z, int ex, int ez, int type) {
	_zTransparent = false;
	if (type == kRayCastWall) {
		switch (cell->type) {
		case 10:
			if (get2dIntersection(cell->data[0], ex, ez, _zRayStepX, _zRayStepZ, &x, &z)) {
				int num = ((z >> kFracShift) ^ _zRayMask) & ((kWallWidth << 2) - 1);
				if ((num >> 2) & 1) {
					return 0;
				}
				_resXRayZ = x;
				_resZRayZ = z;
				return 1;
			}
			return 0;
		case 11:
			if (get2dIntersection(cell->data[0], ez, ex, _xRayStepZ, _xRayStepX, &z, &x)) {
				int num = ((x >> kFracShift) ^ _xRayMask) & ((kWallWidth << 2) - 1);
				if ((num >> 2) & 1) {
					return 0;
				}
				_resXRayZ = x;
				_resZRayZ = z;
				return 1;
			}
			return 0;
		case 16:
			if (get2dIntersection(cell->data[0], ex, ez, _zRayStepX, _zRayStepZ, &x, &z)) {
				int num = (z >> kFracShift) & ((kWallWidth << 2) - 1);
				if (num > cell->data[1]) {
					return 0;
				}
				_zTransparent = true;
				return 0;
			}
			return 0;
		case 17:
			if (get2dIntersection(cell->data[0], ex, ez, _zRayStepX, _zRayStepZ, &x, &z)) {
				int num = (z >> kFracShift) & ((kWallWidth << 2) - 1);
				if (num < 63 - cell->data[1]) {
					return 0;
				}
				_zTransparent = true;
				return 0;
			}
			return 0;
		case 18:
			if (get2dIntersection(cell->data[0], ez, ex, _xRayStepZ, _xRayStepX, &z, &x)) {
				int num = (x >> kFracShift) & ((kWallWidth << 2) - 1);
				if (num > cell->data[1]) {
					return 0;
				}
				_zTransparent = true;
				return 0;
			}
			return 0;
		case 19:
			if (get2dIntersection(cell->data[0], ez, ex, _xRayStepZ, _xRayStepX, &z, &x)) {
				int num = (x >> kFracShift) & ((kWallWidth << 2) - 1);
				if (num < 63 - cell->data[1]) {
					return 0;
				}
				_zTransparent = true;
				return 0;
			}
			return 0;
		}
	}
	switch (cell->type) {
	case 2:
		if (get2dIntersection(cell->data[0], ex, ez, _zRayStepX, _zRayStepZ, &x, &z)) {
			_resXRayZ = x;
			_resZRayZ = z;
			return 1;
		}
		break;
	case 3:
		if (get2dIntersection(cell->data[0], ez, ex, _xRayStepZ, _xRayStepX, &z, &x)) {
		  _resXRayZ = x;
		  _resZRayZ = z;
		  return 1;
		}
		break;
	case 4:
	case 16:
		if (get2dIntersection(cell->data[0], ex, ez, _zRayStepX, _zRayStepZ, &x, &z)) {
			const int num = (z >> kFracShift) & ((kWallWidth << 2) - 1);
			if (num > cell->data[1]) {
				return 0;
			}
			_resXRayZ = x;
			_resZRayZ = z;
			return 1;
		}
		break;
	case 5:
	case 17:
		if (get2dIntersection(cell->data[0], ex, ez, _zRayStepX, _zRayStepZ, &x, &z)) {
			const int num = (z >> kFracShift) & ((kWallWidth << 2) - 1);
			if (num < 63 - cell->data[1]) {
				return 0;
			}
			_resXRayZ = x;
			_resZRayZ = z;
			return 1;
		}
		break;
	case 6:
	case 18:
		if (get2dIntersection(cell->data[0], ez, ex, _xRayStepZ, _xRayStepX, &z, &x)) {
			const int num = (x >> kFracShift) & ((kWallWidth << 2) - 1);
			if (num > cell->data[1]) {
				return 0;
			}
			_resXRayZ = x;
			_resZRayZ = z;
			return 1;
		}
		break;
	case 7:
	case 19:
		if (get2dIntersection(cell->data[0], ez, ex, _xRayStepZ, _xRayStepX, &z, &x)) {
			const int num = (x >> kFracShift) & ((kWallWidth << 2) - 1);
			if (num < 63 - cell->data[1]) {
				return 0;
			}
			_resXRayZ = x;
			_resZRayZ = z;
			return 1;
		}
		break;
	case 10:
		break;
	case 11:
		break;
	case 20:
		return 1;
	case 32:
		return 0;
	}
	return 0;
}

int Game::rayCastHelper(GameObject *o, int sx, RayCastCallbackType callback, int type) {
	++_rayCastCounter;

	const int ry = -o->pitch & 1023;
	_yCosObserver = g_cos[ry] * 2;
	_ySinObserver = g_sin[ry] * 2;
	const int invry = -ry & 1023;
	_yInvCosObserver = g_cos[invry] * 2;
	_yInvSinObserver = g_sin[invry] * 2;

	_xPosRay = (o->xPos + o->xPosParent) << (kRayShift - 19);
	_zPosRay = (o->zPos + o->zPosParent) << (kRayShift - 19);

	const uint32_t rayxex = _xPosRay >> 22;
	const uint32_t rayxez = _zPosRay >> 22;

	if (rayxex >= kMapSizeX || rayxez >= kMapSizeZ) {
		return 0;
	}
	if (rayxex < kMapSizeX - 1 && rayxez < kMapSizeZ - 1) {
		CellMap *cellMap = getCellMap(rayxex, rayxez);
		if (cellMap->colSlot && cellMap->rayCastCounter != _rayCastCounter) {
			int16_t objKey = (this->*callback)(o, cellMap, _xPosRay, _zPosRay);
			if (objKey) {
				return (objKey == -1) ? 0 : objKey;
			}
		}
	}
	const int xStartRay = (kScreenWidth / 2) + sx;
	return rayCast(o, xStartRay, callback, type);
}

int Game::rayCast(GameObject *o, int xStartRay, RayCastCallbackType callback, int type) {

	rayCastInit(xStartRay);

	static const uint32_t kResRayMask = 0xFFFFFFFF << kRayShift;

	uint32_t rayxex = _xPosRay >> 22;
	uint32_t rayxez = _zPosRay >> 22;
	uint32_t rayzex;
	uint32_t rayzez;

	int zDelta;
	_resXRayZ = _xPosRay & kResRayMask;
	if (_zRayStepX > 0) {
		_resXRayZ += fixedInt(1, kRayShift);
		rayzex = _resXRayZ >> kRayShift;
		_dxRay = 1;
		zDelta = _resXRayZ - _xPosRay;
	} else {
		rayzex = (_resXRayZ >> kRayShift) - 1;
		_dxRay = -1;
		zDelta = _xPosRay - _resXRayZ;
	}
	int zRayDistance = fixedMul(ABS(zDelta), _zStepDistance, kRayShift);
	zDelta = fixedMul(zDelta, _zRayStepZ, kRayShift);
	_resZRayZ = _zPosRay + zDelta;

	int xDelta;
	_resZRayX = _zPosRay & kResRayMask;
	if (_xRayStepZ > 0) {
		_resZRayX += fixedInt(1, kRayShift);
		rayxez = _resZRayX >> kRayShift;
		_dzRay = 1;
		xDelta = _resZRayX - _zPosRay;
	} else {
		_dzRay = -1;
		rayxez = (_resZRayX >> kRayShift) - 1;
		xDelta = _zPosRay - _resZRayX;
	}
	int xRayDistance = fixedMul(ABS(xDelta), _xStepDistance, kRayShift);
	xDelta = fixedMul(xDelta, _xRayStepX, kRayShift);
	_resXRayX = _xPosRay + xDelta;

	int xray = -2;
	int zray = -2;
	while (1) {
		if (xRayDistance < zRayDistance) {
			rayxex = _resXRayX >> kRayShift;
			if (rayxex >= kMapSizeX || rayxez >= kMapSizeZ) {
				xray = 0;
				xRayDistance = 0x7FFFFFFF;
				if (zray != 0) {
					continue;
				}
				break;
			}
			CellMap *cellMap = getCellMap(rayxex, rayxez);
			if (type == kRayCastWall) {
				cellMap->draw |= kCellMapDrawGround;
				if (cellMap->type == 20) {
					_decorTexture = cellMap->texture[0];
				}
			}
			if (cellMap->type > 0) {
				if (cellMap->type == 1) {
					const int num = (_dzRay > 0) ? cellMap->south : cellMap->north;
					if (num) {
						if (type == kRayCastWall) {
							cellMap->draw |= kCellMapDrawWall;
						}
						xray = 1;
						break;
					}
				} else {
					if (cellMap->colSlot && cellMap->rayCastCounter != _rayCastCounter) {
						if (type == kRayCastWall) {
							addObjectToDrawList(cellMap);
						} else {
							int16_t objKey = (this->*callback)(o, cellMap, _resXRayX, _resZRayX);
							if (objKey) {
								return (objKey == -1) ? 0 : objKey;
							}
						}
						cellMap->rayCastCounter = _rayCastCounter;
					}
					if ((type == kRayCastCamera) || testRayX(xStartRay, cellMap, _resXRayX, _resZRayX, rayxex, rayxez, type)) {
						if (type == kRayCastWall) {
							cellMap->draw |= kCellMapDrawWall;
						}
						xray = 2;
						break;
					}
					if (_xTransparent) {
						if (type == kRayCastWall) {
							cellMap->draw |= kCellMapDrawWall;
						}
					}
				}
			} else {
				if (cellMap->colSlot && cellMap->rayCastCounter != _rayCastCounter) {
					if (type == kRayCastWall) {
						switch (cellMap->type) {
						case 0:
						case -3:
							addObjectToDrawList(cellMap);
							break;
						}
					} else {
						int16_t objKey = (this->*callback)(o, cellMap, _resXRayX, _resZRayX);
						if (objKey) {
							return (objKey == -1) ? 0 : objKey;
						}
					}
					cellMap->rayCastCounter = _rayCastCounter;
				}
			}
			_resXRayX += _xRayStepX;
			_resZRayX += _xRayStepZ;
			rayxez += _dzRay;
			xRayDistance += _xStepDistance;
		} else {
			rayzez = _resZRayZ >> kRayShift;
			if (rayzex >= kMapSizeX || rayzez >= kMapSizeZ) {
				zray = 0;
				zRayDistance = 0x7FFFFFFF;
				if (xray != 0) {
					continue;
				}
				break;
			}
			CellMap *cellMap = getCellMap(rayzex, rayzez);
			if (type == kRayCastWall) {
				cellMap->draw |= kCellMapDrawGround;
				if (cellMap->type == 20) {
					_decorTexture = cellMap->texture[0];
				}
			}
			if (cellMap->type > 0) {
				if (cellMap->type == 1) {
					const int num = (_dxRay > 0) ? cellMap->west : cellMap->east;
					if (num) {
						if (type == kRayCastWall) {
							cellMap->draw |= kCellMapDrawWall;
						}
						zray = 1;
						break;
					}
				} else {
					if (cellMap->colSlot && cellMap->rayCastCounter != _rayCastCounter) {
						if (type == kRayCastWall) {
							addObjectToDrawList(cellMap);
						} else {
							int16_t objKey = (this->*callback)(o, cellMap, _resXRayZ, _resZRayZ);
							if (objKey) {
								return (objKey == -1) ? 0 : objKey;
							}
						}
						cellMap->rayCastCounter = _rayCastCounter;
					}
					if ((type == kRayCastCamera) || testRayZ(xStartRay, cellMap, _resXRayZ, _resZRayZ, rayzex, rayzez, type)) {
						if (type == kRayCastWall) {
							cellMap->draw |= kCellMapDrawWall;
						}
						zray = 2;
						break;
					}
					if (_zTransparent) {
						if (type == kRayCastWall) {
							cellMap->draw |= kCellMapDrawWall;
						}
					}
				}
			} else {
				if (cellMap->colSlot && cellMap->rayCastCounter != _rayCastCounter) {
					if (type == kRayCastWall) {
						switch (cellMap->type) {
						case 0:
						case -3:
							addObjectToDrawList(cellMap);
							break;
						}
					} else {
						int16_t objKey = (this->*callback)(o, cellMap, _resXRayZ, _resZRayZ);
						if (objKey) {
							return (objKey == -1) ? 0 : objKey;
						}
					}
					cellMap->rayCastCounter = _rayCastCounter;
				}
			}
			rayzex += _dxRay;
			_resXRayZ += _zRayStepX;
			_resZRayZ += _zRayStepZ;
			zRayDistance += _zStepDistance;
		}
	}
	int rrzx;
	if (xray <= 0) {
		rrzx = 0x7FFFFFFF;
	} else if (xray == 2) {
		const int z = _resZRayX - _zPosRay;
		const int x = _resXRayX - _xPosRay;
		rrzx = fixedMul(_yInvSinObserver, x, kFracShift) + fixedMul(_yInvCosObserver, z, kFracShift);
	} else {
		rrzx = xRayDistance - fixedInt(1, kRayShift);
	}
	int rrzz;
	if (zray <= 0) {
		rrzz = 0x7FFFFFFF;
	} else if (zray == 2) {
		const int z = _resZRayZ - _zPosRay;
		const int x = _resXRayZ - _xPosRay;
		rrzz = fixedMul(_yInvSinObserver, x, kFracShift) + fixedMul(_yInvCosObserver, z, kFracShift);
	} else {
		rrzz = zRayDistance - fixedInt(1, kRayShift);
	}
	if (type == kRayCastWall) {
		if (rrzx < rrzz) {
			rrzx >>= kFracShift;
			if (rrzx < -64 || rrzx >= 4096) {
				rrzx = -64;
			}
		} else {
			rrzz >>= kFracShift;
			if (rrzz < -64 || rrzz >= 4096) {
				rrzz = -64;
			}
		}
		return 0;
	}
	if (rrzx < rrzz) {
		o->xPos = _resXRayX >> (kRayShift - 19);
		o->zPos = _resZRayX >> (kRayShift - 19);
	} else {
		o->xPos = _resXRayZ >> (kRayShift - 19);
		o->zPos = _resZRayZ >> (kRayShift - 19);
	}
	return 0;
}

int Game::rayCastMono(GameObject *o, int sx, CellMap *cm, RayCastCallbackType callback, int delta) {
	const int objKey = rayCastHelper(o, sx, callback, kRayCastMono);
	if (delta != 0 || (delta == 0 && objKey == 0)) {
		o->xPos += _ySinObserver << 2;
		o->zPos -= _yCosObserver << 2;
	}
	return objKey;
}

int Game::rayCastCamera(GameObject *o, int sx, CellMap *cm, RayCastCallbackType callback) {
	const int objKey = rayCastHelper(o, sx, callback, kRayCastCamera);
	o->xPos += _ySinObserver << 2;
	o->zPos -= _yCosObserver << 2;
	return objKey;
}

void Game::rayCastWall(int x, int z) {
	++_rayCastCounter;

	_xPosRay = x << 2;
	_zPosRay = z << 2;

	_xPosRay += _ySinObserver << 6;
	_zPosRay -= _yCosObserver << 6;

	const uint32_t rayxex = _xPosRay >> 22;
	const uint32_t rayxez = _zPosRay >> 22;

	if (rayxex >= kMapSizeX || rayxez >= kMapSizeZ) {
		return;
	}
	if (rayxex < kMapSizeX && rayxez < kMapSizeZ) {
		CellMap *cellMap = getCellMap(rayxex, rayxez);
		cellMap->draw |= kCellMapDrawGround;
		if (cellMap->colSlot && cellMap->rayCastCounter != _rayCastCounter) {
			switch (cellMap->type) {
			case 0:
			case -3:
				addObjectToDrawList(cellMap);
				break;
			}
			cellMap->rayCastCounter = _rayCastCounter;
		}
	}
	for (int x = 0; x < kScreenWidth; ++x) {
		rayCast(0, x, 0, kRayCastWall);
	}
}
