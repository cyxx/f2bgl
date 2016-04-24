
#include "file.h"
#include "game.h"

static const char *kFn = "f2bgl-level%d-%02d.sav";

static const char *kSaveText = "1.00 " __DATE__ "  " __TIME__ " (c) 1995 Delphine Software, France";
static int kHeaderSize = 96;
static int kSaveVersion = 21;

enum {
	kModeSave,
	kModeLoad,
};

#define P(x, type) \
	template <int M> \
	static void persist(FILE *fp, type &value) { \
		if (M == kModeLoad) { \
			value = fileRead ## x (fp); \
		} else if (M == kModeSave) { \
			fileWrite ## x (fp, value); \
		} \
	} \

P(Byte,         bool)
P(Byte,       int8_t)
P(Byte,      uint8_t)
P(Uint16LE,  int16_t)
P(Uint16LE, uint16_t)
P(Uint32LE,  int32_t)
P(Uint32LE, uint32_t)

#undef P

template <int M, typename T>
static void persistPtr(FILE *fp, const T *&ptr, const T *base) {
	static const uint32_t kPtr = 0xFFFFFFFF;
	if (M == kModeLoad) {
		const uint32_t offset = fileReadUint32LE(fp);
		ptr = (offset == kPtr) ? 0 : base + offset;
	 }else if (M == kModeSave) {
		const uint32_t offset = base && ptr ? uint32_t(ptr - base) : kPtr;
		assert(offset == kPtr || (offset & 0x80000000) == 0);
		fileWriteUint32LE(fp, offset);
	}
}

template <int M>
static void pad(FILE *fp, int len) {
	if (M == kModeLoad) {
		fileSetPos(fp, len, kFilePosition_CUR);
	} else if (M == kModeSave) {
		for (int i = 0; i < len; ++i) {
			fileWriteByte(fp, 0);
		}
	}
}

template <int M>
static void persistCellMap(FILE *fp, CellMap &m) {
	persist<M>(fp, m.texture[0]);
	persist<M>(fp, m.texture[1]);
	persist<M>(fp, m.data[0]);
	persist<M>(fp, m.data[1]);
	persist<M>(fp, m.type);
	persist<M>(fp, m.camera);
	persist<M>(fp, m.camera2);
	persist<M>(fp, m.room);
	persist<M>(fp, m.room2);
	persist<M>(fp, m.isDoor);
	persist<M>(fp, m.rayCastCounter);
	persist<M>(fp, m.north);
	persist<M>(fp, m.south);
	persist<M>(fp, m.west);
	persist<M>(fp, m.east);
	if (M == kModeLoad) {
		m.colSlot = 0;
	}
}

template <int M>
static void persistSceneTexture(FILE *fp, SceneTexture &t) {
	persist<M>(fp, t.framesCount);
	persist<M>(fp, t.key);
	pad<M>(fp, sizeof(uint16_t));
}

template <int M>
static void persistSceneAnimation(FILE *fp, SceneAnimation &s) {
	persist<M>(fp, s.typeInit);
	persist<M>(fp, s.frameNumInit);
	persist<M>(fp, s.aniKey);
	persist<M>(fp, s.frmKey);
	persist<M>(fp, s.ticksCount);
	persist<M>(fp, s.framesCount);
	persist<M>(fp, s.type);
	persist<M>(fp, s.frameNum);
	persist<M>(fp, s.frm2Key);
	persist<M>(fp, s.msgKey);
	persist<M>(fp, s.msg);
	persist<M>(fp, s.next);
	persist<M>(fp, s.prevFrame);
	persist<M>(fp, s.direction);
	persist<M>(fp, s.pauseTicksCount);
	persist<M>(fp, s.frameIndex);
	persist<M>(fp, s.frame2Index);
	persist<M>(fp, s.flags);
};

template <int M>
static void persistSceneAnimationState(FILE *fp, SceneAnimationState &s) {
	persist<M>(fp, s.flags);
	persist<M>(fp, s.len);
}

template <int M>
static void persistMap(FILE *fp, Game &g) {
	for (int x = 0; x < kMapSizeX; ++x) {
		for (int z = 0; z < kMapSizeZ; ++z) {
			persistCellMap<M>(fp, g._sceneCellMap[x][z]);
			persist<M>(fp, g._sceneGroundMap[x][z]);
		}
	}
	// _sceneGridX
	// _sceneGridZ
	for (int i = 0; i < ARRAYSIZE(g._sceneAnimationsTable); ++i) {
		persistSceneAnimation<M>(fp, g._sceneAnimationsTable[i]);
	}
	for (int i = 0; i < ARRAYSIZE(g._sceneAnimationsStateTable); ++i) {
		persistSceneAnimationState<M>(fp, g._sceneAnimationsStateTable[i]);
	}
	persist<M>(fp, g._sceneTexturesCount);
	for (int i = 0; i < g._sceneTexturesCount; ++i) {
		persistSceneTexture<M>(fp, g._sceneTexturesTable[i]);
	}
	persist<M>(fp, g._sceneAnimationsCount);
	persist<M>(fp, g._sceneAnimationsCount2);
	persist<M>(fp, g._sceneCamerasCount);
}

template <int M>
static void persistGameObjectPtrByKey(FILE *fp, Game &g, GameObject *&o) {
	if (M == kModeSave) {
		int16_t objKey = o ? o->objKey : -1;
		persist<kModeSave>(fp, objKey);
	} else if (M == kModeLoad) {
		int16_t objKey = -1;
		persist<kModeLoad>(fp, objKey);
		o = (objKey != -1) ? g.getObjectByKey(objKey) : 0;
	}
}

template <int M>
static void persistGameObjectAnimation(FILE *fp, GameObjectAnimation &a) {
	persist<M>(fp, a.animKey);
	persist<M>(fp, a.currentAnimKey);
	persist<M>(fp, a.ticksCount);
	persist<M>(fp, a.framesCount);
}

template <int M>
static void persistGameMessage(FILE *fp, GameMessage &m) {
	persist<M>(fp, m.objKey);
	persist<M>(fp, m.num);
	persist<M>(fp, m.ticks);
}

template <int M>
static void persistGameMessageList(FILE *fp, GameObject *o) {
	int count = 0;
	if (M == kModeSave) {
		for (GameMessage *m = o->msg; m; m = m->next) {
			++count;
		}
	}
	persist<M>(fp, count);
	if (M == kModeLoad) {
		GameMessage *msg = o->msg;
		while (msg) {
			GameMessage *next = msg->next;
			free(msg);
			msg = next;
		}
		o->msg = 0;
		GameMessage *prev = 0;
		for (int i = 0; i < count; ++i) {
			GameMessage *m = (GameMessage *)calloc(1, sizeof(GameMessage));
			if (!prev) {
				o->msg = m;
			} else {
				prev->next = m;
			}
			prev = m;
		}
	}
	for (GameMessage *m = o->msg; m; m = m->next) {
		persistGameMessage<M>(fp, *m);
	}
}

template <int M>
static void persistGameObject(FILE *fp, Game &g, GameObject *o) {
        persist<M>(fp, o->scriptKey);
	persistGameObjectAnimation<M>(fp, o->anim);
	if (M == kModeLoad) {
		g.setupObjectScriptAnim(o);
	}
	persist<M>(fp, o->pal);
	persist<M>(fp, o->room);
	persist<M>(fp, o->inSceneList);
	persist<M>(fp, o->scriptStateKey);
	persist<M>(fp, o->startScriptKey);
	persist<M>(fp, o->pitchPrev);
	persist<M>(fp, o->pitch);
	persist<M>(fp, o->xPosPrev);
	persist<M>(fp, o->yPosPrev);
	persist<M>(fp, o->zPosPrev);
	persist<M>(fp, o->xPos);
	persist<M>(fp, o->yPos);
	persist<M>(fp, o->zPos);
	persist<M>(fp, o->xPosParentPrev);
	persist<M>(fp, o->yPosParentPrev);
	persist<M>(fp, o->zPosParentPrev);
	persist<M>(fp, o->xPosParent);
	persist<M>(fp, o->yPosParent);
	persist<M>(fp, o->zPosParent);
	persist<M>(fp, o->xFrm1);
	persist<M>(fp, o->zFrm1);
	persist<M>(fp, o->xFrm2);
	persist<M>(fp, o->zFrm2);
	persistGameObjectPtrByKey<M>(fp, g, o->o_child);
	persistGameObjectPtrByKey<M>(fp, g, o->o_next);
	persistGameObjectPtrByKey<M>(fp, g, o->o_parent);
	for (int i = 0; i < ARRAYSIZE(o->flags); ++i) {
		persist<M>(fp, o->flags[i]);
	}
	for (int i = 0; i < ARRAYSIZE(o->specialData[1]); ++i) {
		persist<M>(fp, o->specialData[1][i]);
	}
	for (int i = 0; i < ARRAYSIZE(o->customData); ++i) {
		persist<M>(fp, o->customData[i]);
	}
	uint8_t *stmStateData = 0;
	uint8_t *stmCondData = 0;
	if (o->scriptStateKey > 0) {
		stmStateData = g._res.getData(kResType_STM, o->scriptStateKey, "STMSTATE");
		if (o->scriptCondKey > 0) {
			stmCondData = g._res.getData(kResType_STM, o->scriptCondKey, "STMCOND");
		}
        }
	persistPtr<M>(fp, o->scriptStateData, stmStateData);
	persistPtr<M>(fp, o->scriptCondData, stmCondData);
	persist<M>(fp, o->state);
	persist<M>(fp, o->setColliding);
	persist<M>(fp, o->updateColliding);
	persist<M>(fp, o->parentChanged);
	persistGameMessageList<M>(fp, o);
	persist<M>(fp, o->rayCastCounter);
	persist<M>(fp, o->xPosWorld);
	persist<M>(fp, o->yPosWorld);
	persist<M>(fp, o->zPosWorld);
	if (M == kModeLoad) {
		o->colSlot = 0;
	}
}

template <int M>
static void persisGameFollowingObject(FILE *fp, GameFollowingObject &o) {
	for (int i = 0; i < ARRAYSIZE(o.points); ++i) {
		persist<M>(fp, o.points[i].x);
		persist<M>(fp, o.points[i].z);
	}
}

template <int M>
static void persistObjects(FILE *fp, Game &g) {
	for (int i = 0; i < ARRAYSIZE(g._objectKeysTable); ++i) {
		GameObject *o = g._objectKeysTable[i];
		if (M == kModeSave) {
			if (o) {
				persist<kModeSave>(fp, o->objKey);
				persistGameObject<kModeSave>(fp, g, o);
			} else {
				int16_t objKey = -1;
				persist<kModeSave>(fp, objKey);
			}
		} else if (M == kModeLoad) {
			int16_t objKey = -1;
			persist<kModeLoad>(fp, objKey);
			if (objKey == -1) {
				assert(!o);
			} else {
				assert(o);
				assert(o->objKey == objKey);
				persistGameObject<kModeLoad>(fp, g, o);
			}
		}
	}
	for (int i = 0; i < ARRAYSIZE(g._varsTable); ++i) {
		persist<M>(fp, g._varsTable[i]);
	}
	for (int i = 0; i < ARRAYSIZE(g._gunTicksTable); ++i) {
		persist<M>(fp, g._gunTicksTable[i]);
	}
	persist<M>(fp, g._followingObjectsCount);
	if (M == kModeLoad) {
		free(g._followingObjectsTable);
		g._followingObjectsTable = ALLOC<GameFollowingObject>(g._followingObjectsCount);
	}
	for (int i = 0; i < ARRAYSIZE(g._objectsPtrTable); ++i) {
		persistGameObjectPtrByKey<M>(fp, g, g._objectsPtrTable[i]);
	}
	persist<M>(fp, g._objectsDrawCount);
	for (int i = 0; i < g._objectsDrawCount; ++i) {
		persistGameObjectPtrByKey<M>(fp, g, g._objectsDrawList[i]);
	}
	persist<M>(fp, g._changedObjectsCount);
	for (int i = 0; i < g._changedObjectsCount; ++i) {
		persistGameObjectPtrByKey<M>(fp, g, g._changedObjectsTable[i]);
	}
	persist<M>(fp, g._collidingObjectsCount);
	for (int i = 0; i < g._collidingObjectsCount; ++i) {
		persistGameObjectPtrByKey<M>(fp, g, g._collidingObjectsTable[i]);
	}
	persist<M>(fp, g._currentObjectKey);
	persist<M>(fp, g._newPlayerObject);
}

template <int M>
static void persistGameInput(FILE *fp, GameInput &i) {
	persist<M>(fp, i.inputKey0);
	persist<M>(fp, i.inputKey1);
	persist<M>(fp, i.keymaskPrev);
	persist<M>(fp, i.keymask);
	persist<M>(fp, i.keymaskToggle);
	persist<M>(fp, i.sysmaskPrev);
	persist<M>(fp, i.sysmask);
}

template <int M>
static void persistCameraPosMap(FILE *fp, CameraPosMap &m) {
	persist<M>(fp, m.x);
	persist<M>(fp, m.z);
	persist<M>(fp, m.ry);
	persist<M>(fp, m.l_ry);
	persist<M>(fp, m.r_ry);
}

template <int M>
static void persistCamera(FILE *fp, Game &g) {
	persist<M>(fp, g._cameraViewKey);
	persist<M>(fp, g._cameraViewObj);
	persist<M>(fp, g._xPosViewpoint);
	persist<M>(fp, g._yPosViewpoint);
	persist<M>(fp, g._zPosViewpoint);
	persist<M>(fp, g._x0PosViewpoint);
	persist<M>(fp, g._y0PosViewpoint);
	persist<M>(fp, g._z0PosViewpoint);
	persist<M>(fp, g._yRotViewpoint);
	persist<M>(fp, g._fixedViewpoint);
	persist<M>(fp, g._xPosObserver);
	persist<M>(fp, g._yPosObserver);
	persist<M>(fp, g._zPosObserver);
	persist<M>(fp, g._yRotObserver);
	persist<M>(fp, g._xPosObserverPrev);
	persist<M>(fp, g._yPosObserverPrev);
	persist<M>(fp, g._zPosObserverPrev);
	persist<M>(fp, g._yRotObserverPrev);
	persist<M>(fp, g._yPosObserverValue);
	persist<M>(fp, g._yPosObserverValue2);
	persist<M>(fp, g._yPosObserverTicks);
	persist<M>(fp, g._cameraStep1);
	persist<M>(fp, g._cameraStep2);
	persist<M>(fp, g._cameraDist);
	persist<M>(fp, g._cameraDistValue2);
	persist<M>(fp, g._cameraDistValue);
	persist<M>(fp, g._cameraDistTicks);
	persist<M>(fp, g._cameraDeltaRotY);
	persist<M>(fp, g._cameraDeltaRotYValue2);
	persist<M>(fp, g._cameraDeltaRotYTicks);
	persist<M>(fp, g._rayCastCounter);
	persist<M>(fp, g._cameraState);
	persist<M>(fp, g._prevCameraState);
	persist<M>(fp, g._currentCamera);
	persist<M>(fp, g._animDx);
	persist<M>(fp, g._animDz);
	pad<M>(fp, sizeof(uint32_t));
	pad<M>(fp, sizeof(uint32_t));
	pad<M>(fp, sizeof(uint32_t));
	pad<M>(fp, sizeof(uint32_t));
	persist<M>(fp, g._sceneCamerasCount);
	for (int i = 0; i < g._sceneCamerasCount; ++i) {
		persistCameraPosMap<M>(fp, g._sceneCameraPosTable[i]);
	}
	pad<M>(fp, sizeof(uint32_t));
	pad<M>(fp, sizeof(uint32_t)); // _focalDistance);
	persist<M>(fp, g._roomsTableCount);
	persist<M>(fp, g._cameraDefaultDist);
	persist<M>(fp, g._cameraFixDist);
	persist<M>(fp, g._cameraStandingDist);
	persist<M>(fp, g._cameraUsingGunDist);
	persist<M>(fp, g._conradUsingGun);
	for (int i = 0; i < ARRAYSIZE(g._roomsTable); ++i) {
		persist<M>(fp, g._roomsTable[i].fl);
	}
}

template <int M>
static void persistInput(FILE *fp, Game &g) {
	persist<M>(fp, g._inputsCount);
	for (int i = 0; i < g._inputsCount; ++i) {
		persistGameInput<M>(fp, g._inputsTable[i]);
	}
	persist<M>(fp, g._conradEnvAni);
}

template <int M>
static void persistOption(FILE *fp, Game &g) {
	persist<M>(fp, g._ticks);
	pad<M>(fp, sizeof(uint32_t));
	persist<M>(fp, g._rnd._randSeed);
	for (int i = 0; i < ARRAYSIZE(g._cut._playedTable); ++i) {
		persist<M>(fp, g._cut._playedTable[i]);
	}
}

template <int M>
static void persistResMessageDescription(FILE *fp, Game &g, ResMessageDescription &d) {
	persistPtr<M>(fp, d.data, g._res._objectTextData);
	persist<M>(fp, d.frameSync);
	persist<M>(fp, d.duration);
	persist<M>(fp, d.xPos);
	persist<M>(fp, d.yPos);
	persist<M>(fp, d.font);
}

template <int M>
static void persistGamePlayerMessage(FILE *fp, Game &g, GamePlayerMessage &m) {
	persistResMessageDescription<M>(fp, g, m.desc);
	persist<M>(fp, m.objKey);
	persist<M>(fp, m.value);
	persist<M>(fp, m.w);
	persist<M>(fp, m.h);
	persist<M>(fp, m.xPos);
	persist<M>(fp, m.yPos);
	persist<M>(fp, m.visible);
	persist<M>(fp, m.crc);
}

template <int M>
static void persistMessage(FILE *fp, Game &g) {
	persist<M>(fp, g._playerMessagesCount);
	for (int i = 0; i < g._playerMessagesCount; ++i) {
		persistGamePlayerMessage<M>(fp, g, g._playerMessagesTable[i]);
	}
}

template <int M>
static void persistMusic(FILE *fp, Game &g) {
	pad<M>(fp, sizeof(uint32_t)); // _soundMode
	persist<M>(fp, g._snd._musicMode);
	persist<M>(fp, g._snd._musicKey);
}

template <int M>
static void persistGameState(FILE *fp, Game &g) {
	pad<M>(fp, sizeof(uint32_t));
	persist<M>(fp, g._room);
	persistMap<M>(fp, g);
	persistObjects<M>(fp, g);
	persistCamera<M>(fp, g);
	persistInput<M>(fp, g);
	persistOption<M>(fp, g);
	persistMessage<M>(fp, g);
	persistMusic<M>(fp, g);
}

void Game::saveGameState(int num) {
	char filename[32];
	snprintf(filename, sizeof(filename), kFn, _level + 1, num);
	FILE *fp = fileOpen(filename, 0, kFileType_SAVE, false);
	if (!fp) {
		return;
	}
	char header[kHeaderSize];
	snprintf(header, sizeof(header), "PC__%4d : %s", kSaveVersion, kSaveText);
	fileWrite(fp, header, sizeof(header));
	// 160x100 thumbnail
	persist<kModeSave>(fp, _level);
	persistGameState<kModeSave>(fp, *this);
	fileClose(fp);
}

void Game::loadGameState(int num) {
	char filename[32];
	snprintf(filename, sizeof(filename), kFn, _level + 1, num);
	FILE *fp = fileOpen(filename, 0, kFileType_LOAD, false);
	if (!fp) {
		return;
	}
	char header[kHeaderSize];
	fileRead(fp, header, sizeof(header));
	int version;
	if (sscanf(header, "PC__%4d", &version) == 1 && version == kSaveVersion) {
		int level = -1;
		persist<kModeLoad>(fp, level);
		debug(kDebug_SAVELOAD, "level %d currentLevel %d", level, _level);
		if (level != _level) {
			_level = level;
			initLevel();
		}
		persistGameState<kModeLoad>(fp, *this);
	}
	fileClose(fp);
}
