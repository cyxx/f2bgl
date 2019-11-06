/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#ifndef GAME_H__
#define GAME_H__

#include "util.h"
#include "cutscene.h"
#include "cutscenepsx.h"
#include "resource.h"
#include "sound.h"
#include "spritecache.h"
#include "random.h"
#include "render.h"

enum {
	kObjPtrWorld = 0,
	kObjPtrInventaire = 1,
	kObjPtrCimetiere = 2,
	kObjPtrCible = 3,
	kObjPtrConrad = 4,
	kObjPtrTarget = 5,
	kObjPtrTargetCommand = 6,
	kObjPtrShield = 7,
	kObjPtrUtil = 8,
	kObjPtrProj = 9,
	kObjPtrScan = 10,
	kObjPtrGun = 11,
	kObjPtrObject1 = 12,
	kObjPtrObject2 = 13,
	kObjPtrFondScanInfo = 14,
	kObjPtrMusic = 15,
	kObjPtrSaveloadOption = 16,
	kObjPtrObjetTemporaire = 17,
	kObjPtrScenesCine = 18,
	kObjPtrIconesSouris = 19,
	kObjPtrFadeToBlack = 20,

	kObjectPtrsTableSize = 21
};

enum {
	kVarsCount = 64,
	kOpcodesCount = 80,
	kGunTicksTableSize = 8,
	kPalKeysTableSize = 16,
	kRoomsTableSize = 128,
	kParticlesTableSize = 256,
	kObjectKeysTableSize = 900,
	kSceneObjectsTableSize = 64,
	kChangedObjectsTableSize = 64,
	kInputKeySize = 2,
	kPlayerMessagesTableSize = 16,
	kFontTableSize = 6,
	kFontGlyphsCount = 222,
	kMapSizeX = 64,
	kMapSizeZ = 64,
	kGroundY = 64,
	kWallThick = 4,
	kWallWidth = 16,
	kWallHeight = 64,
	kSpritesTableSize = 3,
	kPosShift = 15,
	kAniShift = 4,
	kObjectsDrawListSize = 64,
	kCollidingObjectsTableSize = 64,
	kCutsceneMessagesTableSize = 128,
	kSoundKeysTableSize = 10,
	kScreenWidth = 320,
	kScreenHeight = 200,
	kRayCastedObjectsTableSize = 32,
	kFollowingObjectPointsTableSize = 30,
	kViewportMax = 20,
	kTickDurationMs = 40,
	kLevelGameOver = 14,
	kSaveLoadTexKey = 10000,
	kSaveLoadSlots = 8,
	kPlayerInputPointersCount = 4,
	kFollowingMargin = 2,
	kTexKeyBlob = 20000,
	kTexKeyWall = 30000,
};

enum {
	kVarMessageObjectKey = 4,
	kVarConradLife = 20,
	kVarPlayerObject = 30,
	kVarViewpointObject = 31
};

enum {
	kMsgGameSave = 1,
	kMsgGameLoad = 2,
	kMsgPause = 8,
};

enum {
	kIndirectColorShadow = 0,
	kIndirectColorLight,
	kIndirectColorGreen,
	kIndirectColorRed,
	kIndirectColorYellow,
	kIndirectColorBlue,

	kIndirectColorTableSize
};

enum {
	kIconModeGame,
	kIconModeCabinet,
};

enum {
	kIconActionRun,
	kIconActionJump,
	kIconActionDuck,
	kIconActionLoad,
	kIconActionGun,
	kIconActionHand,
	kIconActionHandUse,
	kIconActionDirTurn180,
	kIconActionDirUp,
	kIconActionDirDown,
	kIconActionDirLeft,
	kIconActionDirRight,
	kIconActionInventory,
	kIconActionOptions,
};

enum {
	kFontNormale = 0,
	kFontMenuCart,
	kFontNameCart,
	kFontNbCart,
	kFontNameInvent,
	kFontNameCineTypo,
};

enum {
	kCheatLifeCounter = 1 << 0,
	// control tweak #1 - gun will automatically reload if ammo equals 0, this avoids using the 'enter' key to reload
	kCheatAutoReloadGun = 1 << 1,
	// control tweak #2 - space can be used to shoot, this avoids using the 'ctrl' key to shoot
	kCheatActivateButtonToShoot = 1 << 2,
	// control tweak #3 - pressing 'up' or 'down' when shooting allows to do back and foot steps, this avoids using the 'pageup' and 'pagedown' keys
	kCheatStepWithUpDownInShooting = 1 << 3,
	// control tweak #4 - front and back steps when pressing 'ctrl' with 'up' and 'down' keys, this avois using the 'pageup' and 'pagedown' keys
	kCheatShootButtonToStep = 1 << 4,
};

struct CollisionSlot;

enum {
	kCellMapDrawGround = 1 << 0,
	kCellMapDrawWall = 1 << 1,
};

struct CellMap {
	int16_t texture[2];
	int8_t data[2];
	int16_t type;
	uint8_t camera;
	uint8_t camera2;
	uint8_t room;
	uint8_t room2;
	bool isDoor;
	int rayCastCounter;
	bool fixed;
	int draw;
	int16_t north, south, west, east;
	CollisionSlot *colSlot;
};

struct CameraPosMap {
	int32_t x, z, ry;
	int32_t l_ry, r_ry;
};

struct SpriteImage {
	int16_t w, h;
	uint8_t *data;
	int16_t key;
};

struct GameObjectAnimation {
	int16_t animKey;
	int16_t currentAnimKey;
	int ticksCount;
	int framesCount;
	uint8_t *aniheadData;
	uint8_t *anikeyfData;
	uint8_t *aniframData;
};

struct GameMessage {
	int16_t objKey;
	int num;
	int ticks;
	GameMessage *next;
};

struct GamePlayerMessage {
	ResMessageDescription desc;
	int16_t objKey;
	uint32_t value;
	int w, h;
	int xPos, yPos;
	bool visible;
	uint32_t crc;
};

struct GameObject {
	const char *text;
	const char *name;
	int16_t objKey;
	int16_t scriptKey;
	GameObjectAnimation anim;
	int pal;
	int room;
	int inSceneList;
	int16_t scriptStateKey;
	int16_t scriptCondKey;
	int16_t startScriptKey;
	int pitchPrev, pitch;
	int xPosPrev, yPosPrev, zPosPrev;
	int xPos, yPos, zPos;
	int xPosParentPrev, yPosParentPrev, zPosParentPrev;
	int xPosParent, yPosParent, zPosParent;
	int xFrm1, zFrm1, xFrm2, zFrm2;
	struct GameObject *o_child;
	struct GameObject *o_next;
	struct GameObject *o_parent;
	uint32_t flags[2];

	int specialData[2][26];
	int customData[12];
	void setData(int i, int value) {
	        if (i < 64) {
			assert(i < 26);
			specialData[1][i] = value;
		} else {
			i -= 64;
			assert(i < 12);
			customData[i] = value;
		}
	}
	int getData(int i) const {
		if (i < 64) {
			assert(i < 26);
			return specialData[1][i];
		} else {
			i -= 64;
			assert(i < 12);
			return customData[i];
		}
	}

	const uint8_t *scriptStateData;
	const uint8_t *scriptCondData;
	int state;
	bool setColliding;
	bool updateColliding;
	bool parentChanged;
	GameMessage *msg;
	int rayCastCounter;
	int xPosWorld, yPosWorld, zPosWorld;
	CollisionSlot *colSlot;
};

struct CollisionSlot {
	GameObject *o;
	CollisionSlot *next, *prev;
	CollisionSlot *list;
	CellMap *cell;
};

struct CollisionSlot2 {
	int box;
	bool validObj;
	uint8_t type;
	uint16_t hitsCount;
	CellMap *cell;
};

struct GameRoom {
	GameObject *o;
	int16_t palKey;
	uint16_t fl;
};

struct GameInput {
	uint8_t inputKey0;
	uint8_t inputKey1;
	uint32_t keymaskPrev, keymask, keymaskToggle;
	uint32_t sysmaskPrev, sysmask;
};

struct SceneAnimation {
	int32_t typeInit;
	int32_t frameNumInit;
	int16_t aniKey;
	int16_t frmKey;
	int ticksCount;
	int framesCount;
	int32_t type;
	int32_t frameNum;
	int16_t frm2Key;
	int16_t msgKey;
	int32_t msg;
	int32_t next;
	int16_t prevFrame;
	int16_t direction;
	int16_t pauseTicksCount;
	int16_t frameIndex;
	int16_t frame2Index;
	int16_t flags;
};

struct SceneAnimationState {
	uint16_t flags;
	uint16_t len;
};

struct SceneTexture {
	int framesCount;
	int16_t key;
};

struct SceneObject {
	uint8_t *polygonsData;
	int verticesCount;
	uint8_t *verticesData;
	SpriteImage spr;
	int xmin, zmin, xmax, zmax;
	GameObject *o;
	int x, y, z;
	int pitch;
};

struct GameFollowingPoint {
	int x, z;
};

struct GameFollowingObject {
	GameFollowingPoint points[kFollowingObjectPointsTableSize];
};

struct Particle {
	int xPos, yPos, zPos;
	int dx, dy, dz;
	int fl;
	int16_t ticks;
	int16_t speed;
	bool isBlob;
};

struct Font {
	int h;
	int w;
	int spacing;
	SpriteImage glyphs[kFontGlyphsCount];
};

struct Icon {
	int x, y;
	int xMargin, yMargin;
	SpriteImage spr;
	int action;
	uint8_t *scaledSprData;
	int num;
	bool visible;

	bool isCursorOver(int xCursor, int yCursor) const {
		if (xCursor < x - xMargin || xCursor > x + spr.w - 1 + xMargin) {
			return false;
		}
		if (yCursor < y - yMargin || yCursor > y + spr.h - 1 + yMargin) {
			return false;
		}
		return true;
	}
};

struct RayCastedObject {
	GameObject *o;
	int x, z;
	int dist;
	int ymin, ymax;
};

enum {
	kInputDirUp    = 1 << 0,
	kInputDirDown  = 1 << 1,
	kInputDirLeft  = 1 << 2,
	kInputDirRight = 1 << 3
};

struct PlayerInput {
	uint8_t dirMaskPrev;
	uint8_t dirMask;
	bool altKey;
	bool shiftKey;
	bool ctrlKey;
	bool spaceKey;
	bool enterKey;
	bool tabKey;
	bool useKey;
	bool inventoryKey;
	bool jumpKey;
	bool escapeKey;
	bool numKeys[10];
	bool footStepKey;
	bool backStepKey;
	bool farNear;
	uint8_t lookAtDir;
	struct {
		int x, y;
		bool down;
	} pointers[kPlayerInputPointersCount][2];
};

struct DrawBuffer {
	uint8_t *ptr;
	int w, h, pitch;
	void (*draw)(DrawBuffer *buf, int x, int y, int w, int h, const uint8_t *src);
};

struct DrawNumber {
	int x, y;
	int font;
	int value;
};

struct Render;

struct GameParams {
	GameParams() : playDemo(false), levelNum(0), subtitles(false), sf2(0), mouseMode(false), touchMode(false) {}
	bool playDemo;
	int levelNum;
	bool subtitles;
	const char *sf2;
	bool mouseMode;
	bool touchMode;
};

struct Game {
	typedef int (Game::*OpcodeProc)(int argc, int32_t *argv);
	typedef bool (Game::*CollisionSlotCallbackType1)(CellMap *cell);
	typedef bool (Game::*CollisionSlotCallbackType2)(GameObject *o, CellMap *cell, int x, int z, uint32_t a);
	typedef bool (Game::*CollisionSlotCallbackType3)(GameObject *o, CellMap *cell, int x, int z, uint32_t a, int b);
	typedef int (Game::*RayCastCallbackType)(GameObject *o, CellMap *cell, int x, int z);

	Resource _res;
	Cutscene *_cut;
	Sound _snd;
	Render *_render;
	GameParams _params;
	SpriteCache _spriteCache;
	Random _rnd, _rnd2;
	int _cheats;
	int _gameStateMsg;
	bool _musicPaused;

	DrawBuffer _drawCharBuf;
	DrawNumber _drawNumber;
	int _ticks;
	int _level;
	int _skillLevel;
	bool _changeLevel;
	int _room, _roomPrev;
	bool _endGame;
	int _displayPsxLevelLoadingScreen;
	int _mainLoopCurrentMode;
	int _conradHit;

	int32_t _varsTable[kVarsCount];
	int32_t _gunTicksTable[kGunTicksTableSize];
	GameObject *_objectsPtrTable[kObjectPtrsTableSize];
	GameObject *_currentObject;
	GameObject *_objectKeysTable[kObjectKeysTableSize];
	GameObject _tmpObject;
	int _changedObjectsCount;
	GameObject *_changedObjectsTable[kChangedObjectsTableSize];
	int _followingObjectsCount;
	GameFollowingObject *_followingObjectsTable;
	int16_t _conradObjectKey, _worldObjectKey;
	int16_t _currentScriptKey;
	int16_t _newPlayerObject;
	int _objectsCount, _objectsSetupCount;
	int _objectsDrawCount;
	GameObject *_objectsDrawList[kObjectsDrawListSize];
	GameObject *_updateGlobalPosRefObject;
	int _collidingObjectsCount;
	GameObject *_collidingObjectsTable[kCollidingObjectsTableSize];
	int32_t _conradEnvAni;
	int _cabinetItemCount;
	GameObject *_cabinetItemObj;

	int _roomsTableCount;
	GameRoom _roomsTable[kRoomsTableSize];
	int16_t _mapKey;
	int16_t _palKeysTable[kPalKeysTableSize];
	CellMap _sceneCellMap[kMapSizeX][kMapSizeZ];
	int32_t _sceneGroundMap[kMapSizeX][kMapSizeZ];
	int _sceneCamerasCount;
	CameraPosMap _sceneCameraPosTable[256];
	int _sceneAnimationsCount, _sceneAnimationsCount2;
	SceneAnimation _sceneAnimationsTable[512];
	SceneAnimationState _sceneAnimationsStateTable[512];
	SpriteImage _sceneAnimationsTextureTable[512];
	int _sceneTexturesCount;
	SceneTexture _sceneTexturesTable[256];
	SpriteImage _sceneTextureImagesBuffer[256];
	int _sceneObjectsCount;
	SceneObject _sceneObjectsTable[kSceneObjectsTableSize];
	Font _fontsTable[kFontTableSize];
	int16_t _spritesTable[kSpritesTableSize];
	SpriteImage _infoPanelSpr;
	int _inventoryBackgroundKey;
	int _inventoryCursor[4];
	int16_t _scannerBackgroundKey;
	int _scannerCounter;
	int _iconsCount;
	Icon _iconsTable[20];

	PlayerInput inp;
	int _inputsCount;
	GameInput *_inputsTable;
	uint8_t _inputDirKeyReleased[kInputKeySize];
	uint8_t _inputDirKeyPressed[kInputKeySize];
	uint8_t _inputButtonKey[kInputKeySize];
	uint8_t _inputButtonMouse;
	uint8_t _inputDirMouse;
	bool _inputKeyAction, _inputKeyUse, _inputKeyJump;
	int _demoInput;

	bool _updatePalette;
	uint8_t _screenPalette[256 * 3];
	uint8_t _indirectPalette[kIndirectColorTableSize][256];
	uint32_t _mrkBuffer[260];
	int _decorTexture;

	int _particleDx, _particleDy, _particleDz, _particleRnd, _particleSpd;
	int _particlesCount;
	Particle _particlesTable[kParticlesTableSize];

	int _playerMessagesCount;
	GamePlayerMessage _playerMessagesTable[kPlayerMessagesTableSize];
	int _cutsceneMessagesCount;
	GamePlayerMessage _cutsceneMessagesTable[kCutsceneMessagesTableSize];
	ResMessageDescription _tmpMsg;

	bool _fixedViewpoint;
	int _xPosObserver, _yPosObserver, _zPosObserver;
	int _xPosObserverPrev, _yPosObserverPrev, _zPosObserverPrev;
	int _yRotObserver, _yInvRotObserver, _yRotObserverPrev, _yRotObserverPrev2, _yPosObserverTemp;
	int _cameraDefaultDist, _cameraStandingDist, _cameraUsingGunDist;
	bool _conradUsingGun;
	bool _cameraDistInitDone;
	int _focalDistance;
	int _yCosObserver, _ySinObserver, _yInvCosObserver, _yInvSinObserver;
	int _yPosObserverValue, _yPosObserverValue2, _yPosObserverTicks;
	int _xPosViewpoint, _yPosViewpoint, _zPosViewpoint;
	int _x0PosViewpoint, _y0PosViewpoint, _z0PosViewpoint;
	int _yRotViewpoint;

	int _cameraFixDist;
	int _cameraState, _prevCameraState, _cameraDelta, _cameraStep1, _cameraStep2, _cameraNum;
	int _cameraDistTicks;
	int _cameraDist;
	int _cameraDistValue, _cameraDistValue2;
	int16_t _cameraViewKey;
	int16_t _cameraViewObj;
	int _viewportSize;
	int _zTransform;
	int _currentCamera;
	int _cameraDeltaRotY, _cameraDeltaRotYValue2, _cameraDeltaRotYTicks;
	int _cameraDx, _cameraDy, _cameraDz;
	int _animDx, _animDz;
	int _pitchObserverCamera;
	int _observerColMask;

	int _rayCastCounter;
	int _rayCastedObjectsCount;
	RayCastedObject _rayCastedObjectsTable[kRayCastedObjectsTableSize];

	int _saveLoadTextureIdTable[kSaveLoadSlots];

	Game(Render *render, const GameParams *params);
	~Game();

	GameObject *getObjectByKey(int objKey) {
		assert(objKey >= 0 && objKey < kObjectKeysTableSize);
		return _objectKeysTable[objKey];
	}

	bool checkCellMap(int x, int z) const {
		return INRANGE(x, 0, kMapSizeX << 19) && INRANGE(z, 0, kMapSizeZ << 19);
	}
	CellMap *getCellMap(int x, int z) {
		assert(x >= 0 && x < kMapSizeX && z >= 0 && z < kMapSizeZ);
		return &_sceneCellMap[x][z];
	}
	CellMap *getCellMapShr19(int x, int z) {
		return getCellMap(x >> (4 + kPosShift), z >> (4 + kPosShift));
	}

	int getRoom(int x, int z) {
		CellMap *cellMap = getCellMapShr19(x, z);
		const int i = (cellMap->room != 0) ? cellMap->room : cellMap->room2;
		return i;
	}
	GameObject *getRoomObject(int x, int z) {
		const int i = getRoom(x, z);
		return _roomsTable[i].o;
	}

	bool isConradInShootingPos() const {
		return _varsTable[9] == 1 || _varsTable[9] == 8 || _varsTable[9] == 5 || _varsTable[9] == 6;
	}

	bool isInputKeyDown(uint32_t mask, int input) const {
		return (_inputsTable[input].keymask & mask) == mask;
	}

	void setGameStateLoad(int slot) {
		_gameStateMsg = kMsgGameLoad;
	}
	void setGameStateSave(int slot) {
		_gameStateMsg = kMsgGameSave;
	}

	// cabinet.cpp
	void initCabinet();
	void finiCabinet();
	void setCabinetItem(GameObject *o);
	void doCabinet();

	// game.cpp
	void clearGlobalData();
	void clearLevelData();
	void freeLevelData();
	void countObjects(int16_t parentKey);
	int gotoStartScriptAnim();
	int gotoNextScriptAnim();
	void gotoEndScriptAnim(int fl);
	int16_t getObjectScriptAnimKey(int16_t groupKey, int num);
	void setObjectFlags(const uint8_t *p, GameObject *o);
	int getObjectFlag(GameObject *o, int flag);
	void setObjectFlag(GameObject *o, int flag, int value);
	void setupObjectScriptAnim(GameObject *o);
	void setupObjectsChangeOrder(GameObject *o);
	GameObject *setupObjectsHelper(int16_t prevKey, GameObject *o_parent);
	void setupObjects();
	void getAllPalKeys(int16_t mapKey);
	void getSceneAnimationTexture(SceneAnimation *sa, uint16_t *len, uint16_t *flags, SpriteImage *spr);
	void loadColorMark(int16_t key);
	void setupIndirectColorPalette();
	void setPalette(int16_t key);
	void setPaletteColor(int color, int r, int g, int b);
	void updatePalette();
	void loadSceneMap(int16_t key);
	bool updateSceneAnimationsKeyFrame(int rnd, int index, SceneAnimation *sa, SceneAnimationState *sas);
	void updateSceneAnimations();
	void getSceneTexture(int16_t key, int framesSkip, SpriteImage *spr);
	void loadSceneTextures(int16_t key);
	void updateSceneTextures();
	void initScene();
	void init();
	bool displayPsxLevelLoadingScreen();
	void initLevel(bool keepInventoryObjects = false);
	void setupConradObject();
	void changeRoom(int room);
	void playMusic(int num);
	void clearObjectMessage(GameObject *o);
	int isScriptAnimFrameEnd();
	void setObjectData(GameObject *o, int field, int32_t value);
	int32_t getObjectData(GameObject *o, int field);
	int32_t getObjectScriptParam(GameObject *o, int field);
	int executeObjectScriptOpcode(GameObject *o, uint32_t op, const uint8_t *data);
	uint8_t *getStartScriptAnim();
	uint8_t *getNextScriptAnim();
	int executeObjectScript(GameObject *o);
	void runObject(GameObject *o);
	void setPlayerRoomObjectsData(int fl);
	void setPlayerObject(int16_t objKey);
	void updatePlayerObject();
	void addToChangedObjects(GameObject *o);
	void updateChangedObjects();
	GameObject *getNextObject(GameObject *o);
	GameObject *getPreviousObject(GameObject *o);
	void setObjectParent(GameObject *o, GameObject *o_parent);
	void fixRoomDataHelper(int x, int z, uint8_t room);
	void fixRoomData();
	void updateObjects();
	void doTick();
	void initSprite(int type, int16_t key, SpriteImage *spr);
	void clearMessage(ResMessageDescription *desc);
	bool getMessage(int16_t key, uint32_t value, ResMessageDescription *desc);
	uint8_t *initMesh(int resType, int16_t key, uint8_t **verticesData, uint8_t **polygonsData, GameObject *o, uint8_t **poly3dData, int *env);
	bool addSceneObjectToList(int xPos, int yPos, int zPos, GameObject *o);
	void clearObjectsDrawList();
	void addObjectsToScene();
	void clearKeyboardInput();
	int getCurrentInput();
	void updateCameraViewpoint(int xPos, int yPos, int zPos);
	bool updateGlobalPos(int dx, int dy, int dz, int dx0, int dz0, int flag);
	void drawPolygons(Vertex *polygonPoints, int count, int color);
	bool isTranslucentSceneObject(const SceneObject *so) const;
	void drawSceneObjectShadow(SceneObject *so);
	void drawSceneObjectMesh(SceneObject *so, int flags = 0);
	void redrawScene();
	void drawWall(const Vertex *vertices, int verticesCount, int texture, int type);
	bool redrawSceneGridCell(int x, int z, CellMap *cell);
	void redrawSceneGroundWalls();
	bool findRoom(const CollisionSlot *colSlot, int room1, int room2);
	bool testObjectsRoom(int16_t obj1Key, int16_t obj2Key);
	void readInputEvents();
	void setupScannerObjects();
	void updateScanner();
	void drawScanner();
	void setupInventoryObjects();
	void saveInventoryObjects();
	void loadInventoryObjects();
	GameObject *findObjectByName(GameObject *o, const char *name);
	void addParticle(int xPos, int yPos, int zPos, int rnd, int dx, int dy, int dz, int count, int ticks, int fl, int speed);
	void addParticleBlob(SceneObject *so, int xPos, int yPos, int zPos, int rnd, int ticks, int fl);
	void updateParticles();
	void drawParticles();
	void initSprites();
	void updateScreen();
	void printGameMessages();
	bool sendMessage(int msg, int16_t objKey);
	void addToCollidingObjects(GameObject *o);
	void updateCollidingObjects();
	int getCollidingHorizontalMask(int yPos, int upDir, int downDir);
	void sendShootMessageHelper(GameObject *o, int xPos, int zPos, int radius, int num);
	void initViewport();
	void drawInfoPanel();
	void getCutsceneMessages(int num);
	void playCutscene(int num);
	void playDeathCutscene(int objKey);
	void displayTarget(int cx, int cy);
	int getShootPos(int16_t objKey, int *x, int *y, int *z);
	void drawSprite(int x, int y, int sprKey);
	bool updateCutscene(uint32_t ticks);

	// camera.cpp
	void updateObserverSinCos();
	void rotateObserver(int ix, int iz, int *rx, int *rz);
	int getClosestAngle(int a, int x, int z, int l, int r, int fl);
	void setCameraPos(int xPos, int zPos);
	void getFixedCameraPos(int xPos, int zPos, int *xPosCam, int *zPosCam, int *yRotCam);
	bool setCameraObject(GameObject *o, int16_t *cameraObjKey);
	void fixCamera();
	void updateSceneCameraPos();
	int testCameraPos(int viewpointx, int viewpointz, int viewpointry, int *retx, int *retz, CollisionSlot2 *slots2);
	int testCameraRay(int x, int z, int ry);
	int updateCameraDist(int x, int z, int ry, int maxDistance);
	int getCameraAngle(int viewpointx, int viewpointz, int viewpointry, int *retx, int *retz, int *retry);

	// collision.cpp
	CollisionSlot *createCollisionSlot(CollisionSlot *prev, CollisionSlot *next, GameObject *o, CellMap *cell);
	void destroyCollisionSlot(CellMap *cell);
	bool collisionSlotCb1(CellMap *cellMap);
	bool collisionSlotCb2(CellMap *cellMap);
	void initCollisionSlot(GameObject *o);
	void resetCollisionSlot(GameObject *o);
	bool setCollisionSlotsUsingCallback1(int x, int z, CollisionSlotCallbackType1 callback);
	bool setCollisionSlotsUsingCallback2(GameObject *o, int x, int z, CollisionSlotCallbackType2 callback, uint32_t a, CollisionSlot2 *slot2);
	bool setCollisionSlotsUsingCallback3(GameObject *o, int x, int z, CollisionSlotCallbackType3 callback, uint32_t a, int b, CollisionSlot2 *slot2);
	void addObjectToDrawList(CellMap *cell);
	bool testCollisionSlotRect(GameObject *o1, GameObject *o2) const;
	bool testCollisionSlotRect2(GameObject *o1, GameObject *o2, int x, int z) const;
	bool collisionSlotCb3(GameObject *o, CellMap *cell, int x, int z, uint32_t a);
	bool collisionSlotCb4(GameObject *o, CellMap *cell, int x, int z, uint32_t a, int b);
	void updateCurrentObjectCollisions();
	bool collisionSlotCb5(GameObject * ptr_object, CellMap *map, int x, int z, uint32_t mask8);
	int testObjectCollision2(GameObject *o, int dx1, int dz1, int dx2, int dz2);
	void fixCoordinates(GameObject *o, int dx1, int dz1, int dx2, int dz2, int *fx, int *fz);
	int testObjectCollision1(GameObject *o, int xFrom, int zFrom, int xTo, int zTo, uint32_t mask8);
	void fixCoordinates2(GameObject *o_following, int x1, int z1, int *x2, int *z2, int *x3, int *z3);

	// icons.cpp
	void loadIcon(int16_t key, int num, int x, int y, int action);
	void initIcons(int iconMode);
	void finiIcons();
	void drawIcons();

	// font.cpp
	void loadFont(int num, int h, int w, int spacing, int16_t fontKey);
	void initFonts();
	void drawChar(int x, int y, SpriteImage *spr, int color);
	void drawString(int x, int y, const char *str, int font, int color);
	int getStringRect(const char *str, int font, int *w, int *h);
	void setStringPos(GamePlayerMessage *msg);

	// input.cpp
	void updateInput();
	void updateMouseInput();
	void updateGameInput();
	bool testInputKeyMask(int num, int dir, int button, int index) const;
	bool testInputKeyMaskEq(int num, int dir, int button, int index) const;
	bool testInputKeyMaskPrev(int num, int dir, int button, int index) const;
	void updateInputKeyMask(int inputIndex);

	// installer.cpp
	void initInstaller();
	void finiInstaller();
	void doInstaller();

	// inventory.cpp
	bool initInventory();
	void loadInventoryObjectMesh(GameObject *o);
	void closeInventory();
	void updateInventoryInput();
	void doInventory();

	// menu.cpp
	void initMenu();
	void finiMenu();
	void resetObjectAnim(GameObject *o);
	void loadMenuObjectMesh(GameObject *o, int16_t key);
	bool doMenu();

	// opcodes.cpp
	int op_true(int argc, int32_t *argv);
	int op_toggleInput(int argc, int32_t *argv);
	int op_compareCamera(int argc, int32_t *argv);
	int op_sendMessage(int argc, int32_t *argv);
	int op_getObjectMessage(int argc, int32_t *argv);
	int op_setParticleParams(int argc, int32_t *argv);
	int op_setVar(int argc, int32_t *argv);
	int op_compareConst(int argc, int32_t *argv);
	int op_evalVar(int argc, int32_t *argv);
	int op_playSound(int argc, int32_t *argv);
	int op_getAngle(int argc, int32_t *argv);
	int op_setObjectData(int argc, int32_t *argv);
	int op_evalObjectData(int argc, int32_t *argv);
	int op_compareObjectData(int argc, int32_t *argv);
	int op_jumpToNextLevel(int argc, int32_t *argv);
	int op_rand(int argc, int32_t *argv);
	int op_isObjectColliding(int argc, int32_t *argv);
	int op_sendShootMessage(int argc, int32_t *argv);
	int op_getShootInfo(int argc, int32_t *argv);
	int op_moveObjectToObject(int argc, int32_t *argv);
	int op_getProjObject(int argc, int32_t *argv);
	int op_setObjectParent(int argc, int32_t *argv);
	int op_removeObjectMessage(int argc, int32_t *argv);
	int op_setCabinetItem(int argc, int32_t *argv);
	int op_isObjectMoving(int argc, int32_t *argv);
	int op_getMessageInfo(int argc, int32_t *argv);
	int op_testObjectsRoom(int argc, int32_t *argv);
	int op_setCellMapData(int argc, int32_t *argv);
	int op_addCellMapData(int argc, int32_t *argv);
	int op_compareCellMapData(int argc, int32_t *argv);
	int op_getObjectDistance(int argc, int32_t *argv);
	int op_setObjectSpecialCustomData(int argc, int32_t *argv);
	int op_updateTarget(int argc, int32_t *argv);
	int op_transformObjectPos(int argc, int32_t *argv);
	int op_compareObjectAngle(int argc, int32_t *argv);
	int op_moveObjectToPos(int argc, int32_t *argv);
	int op_setupObjectPath(int argc, int32_t *argv);
	int op_continueObjectMove(int argc, int32_t *argv);
	int op_compareInput(int argc, int32_t *argv);
	int op_clearTarget(int argc, int32_t *argv);
	int op_playCutscene(int argc, int32_t *argv);
	int op_setScriptCond(int argc, int32_t *argv);
	int op_isObjectMessageNull(int argc, int32_t *argv);
	int op_detachObjectChild(int argc, int32_t *argv);
	int op_setCamera(int argc, int32_t *argv);
	int op_getSquareDistance(int argc, int32_t *argv);
	int op_isObjectTarget(int argc, int32_t *argv);
	int op_printDebug(int argc, int32_t *argv);
	int op_translateObject(int argc, int32_t *argv);
	int op_setupTarget(int argc, int32_t *argv);
	int op_getTicks(int argc, int32_t *argv);
	int op_swapFrameXZ(int argc, int32_t *argv);
	int op_addObjectMessage(int argc, int32_t *argv);
	int op_setupCircularMove(int argc, int32_t *argv);
	int op_moveObjectOnCircle(int argc, int32_t *argv);
	int op_isObjectCollidingType(int argc, int32_t *argv);
	int op_setLevelData(int argc, int32_t *argv);
	int op_drawNumber(int argc, int32_t *argv);
	int op_isObjectOnMap(int argc, int32_t *argv);
	int op_updateFollowingObject(int argc, int32_t *argv);
	int op_translateObject2(int argc, int32_t *argv);
	int op_updateCollidingHorizontalMask(int argc, int32_t *argv);
	int op_createParticle(int argc, int32_t *argv);
	int op_setupFollowingObject(int argc, int32_t *argv);
	int op_isObjectCollidingPos(int argc, int32_t *argv);
	int op_setCameraObject(int argc, int32_t *argv);
	int op_setCameraParams(int argc, int32_t *argv);
	int op_setPlayerObject(int argc, int32_t *argv);
	int op_isCollidingRooms(int argc, int32_t *argv);
	int op_isMessageOnScreen(int argc, int32_t *argv);
	int op_debugBreakpoint(int argc, int32_t *argv);
	int op_isObjectConradNotVisible(int argc, int32_t *argv);
	int op_stopSound(int argc, int32_t *argv);

	// raycast.cpp
	void rayCastInit(int sx);
	int rayCastCollisionCb1(GameObject *o, CellMap *cell, int ox, int oz);
	int rayCastCollisionCb2(GameObject *o, CellMap *cell, int ox, int oz);
	int rayCastCameraCb1(GameObject *o, CellMap *cell, int ox, int oz);
	int rayCastCameraCb2(GameObject *o, CellMap *cell, int ox, int oz);
	int rayCastHelper(GameObject *o, int x, RayCastCallbackType callback, int);
	int rayCast(GameObject *o, int x, RayCastCallbackType callback, int type);
	int rayCastMono(GameObject *o, int x, CellMap *cellMap, RayCastCallbackType callback, int delta);
	int rayCastCamera(GameObject *o, int x, CellMap *cellMap, RayCastCallbackType callback);
	void rayCastWall(int x, int z);

	// saveload.cpp
	bool saveGameState(int num);
	bool loadGameState(int num);
	void saveScreenshot(bool saveState, int num);
	bool hasSavedGameState(int num) const;
};

#endif // GAME_H__
