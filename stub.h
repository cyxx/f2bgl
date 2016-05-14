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
	kKeyCodeAlt,     // gun
	kKeyCodeCtrl,    // shoot
	kKeyCodeShift,   // walk
	kKeyCodeSpace,   // use / open
	kKeyCodeReturn,  // recharge gun
	kKeyCodeTab,     // skip cutscene
	kKeyCodeEscape,  // menu
	kKeyCodeI,       // inventory
	kKeyCodeJ,       // jump
	kKeyCodeU,
	kKeyCode1,       // item #1
	kKeyCode2,       // item #2
	kKeyCode3,       // item #3
	kKeyCode4,       // item #4
	kKeyCode5,       // item #5
	kKeyCodeCheatLifeCounter,
};

struct GameStub {
	virtual int init(int argc, char *argv[]) = 0;
	virtual void quit() = 0;
	virtual StubMixProc getMixProc(int rate, int fmt, void (*lock)(int)) = 0;
	virtual void queueKeyInput(int keycode, int pressed) = 0;
	virtual void doTick(unsigned int ticks) = 0;
	virtual void initGL(int w, int h) = 0;
	virtual void drawGL() = 0;
	virtual void loadState(int slot) = 0;
	virtual void saveState(int slot) = 0;
};

extern "C" {
	GameStub *GameStub_create();
}

#endif // STUB_H__
