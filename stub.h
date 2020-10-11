/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#ifndef STUB_H__
#define STUB_H__

struct StubMixProc {
	void (*proc)(void *data, uint8_t *buf, int size);
	void *data;
};

enum {
	kKeyCodeLeft = 1,
	kKeyCodeRight,
	kKeyCodeUp,
	kKeyCodeDown,
	kKeyCodeAlt,     // toggle gun mode
	kKeyCodeCtrl,    // shoot
	kKeyCodeShift,   // walk
	kKeyCodeSpace,   // activate
	kKeyCodeReturn,  // reload gun
	kKeyCodeTab,     // change inventory category
	kKeyCodeEscape,  // menu
	kKeyCodeI,       // inventory
	kKeyCodeJ,       // jump
	kKeyCodeU,       // use
	kKeyCode1,       // item #1
	kKeyCode2,       // item #2
	kKeyCode3,       // item #3
	kKeyCode4,       // item #4
	kKeyCode5,       // item #5
	kKeyCodePageUp,   // foot step
	kKeyCodePageDown, // back foot step
	kKeyCodeFarNear,
	kKeyCodeCheatLifeCounter,
	kKeyCodeToggleFog,
	kKeyCodeToggleGouraudShading,
};

enum {
	kDisplayModeWindow,
	kDisplayModeFullscreen,
};

struct GameStub {
	virtual int setArgs(int argc, char *argv[]) = 0;
	virtual int getDisplayMode() = 0;
	virtual float getAspectRatio(bool widescreen) = 0;
	virtual bool hasCursor() = 0;
	virtual int init() = 0;
	virtual void quit() = 0;
	virtual StubMixProc getMixProc(int rate, int fmt, void (*lock)(int)) = 0;
	virtual void queueKeyInput(int keycode, int pressed) = 0;
	virtual void queueTouchInput(int pointer, int x, int y, int down) = 0;
	virtual void doTick(unsigned int ticks) = 0;
	virtual void initGL(int w, int h, float *ar) = 0;
	virtual void drawGL() = 0;
	virtual void loadState(int slot) = 0;
	virtual void saveState(int slot) = 0;
	virtual void takeScreenshot() = 0;
	virtual bool shouldVibrate() = 0;
};

extern "C" {
	GameStub *GameStub_create();
}

#endif // STUB_H__
