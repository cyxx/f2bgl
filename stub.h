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
	kKeyCodeAlt,
	kKeyCodeCtrl,
	kKeyCodeShift,
	kKeyCodeSpace,
	kKeyCodeReturn,
	kKeyCodeTab,
	kKeyCodeEscape,
	kKeyCodeH,
	kKeyCodeI,
	kKeyCodeJ,
	kKeyCodeK,
	kKeyCodeU,
	kKeyCode1,
	kKeyCode2,
	kKeyCode3,
	kKeyCode4,
	kKeyCode5
};

struct GameStub {
	virtual int init(int argc, char *argv[]) = 0;
	virtual void quit() = 0;
	virtual StubMixProc getMixProc(int rate, int fmt, void (*lock)(int)) = 0;
	virtual void queueKeyInput(int keycode, int pressed) = 0;
	virtual void doTick(unsigned int ticks) = 0;
	virtual void initGL(int w, int h) = 0;
	virtual void drawGL() = 0;
};

extern "C" {
	GameStub *GameStub_create();
}

#endif // STUB_H__
