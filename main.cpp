/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include <SDL.h>
#include "stub.h"

static const int kDefaultW = 640;
static const int kDefaultH = kDefaultW * 3 / 4;

static int gWindowW = kDefaultW;
static int gWindowH = kDefaultH;

static int gScale = 2;

static const int kTickDuration = 30;
static const char *kCaption = "Fade2Black/OpenGL";

static int gKeySymMap[512];
static int gKeyScanMap[256];

static void setupKeyMap() {
	memset(gKeySymMap, 0, sizeof(gKeySymMap));
	gKeySymMap[SDLK_LEFT]   = kKeyCodeLeft;
	gKeySymMap[SDLK_RIGHT]  = kKeyCodeRight;
	gKeySymMap[SDLK_UP]     = kKeyCodeUp;
	gKeySymMap[SDLK_DOWN]   = kKeyCodeDown;
	gKeySymMap[SDLK_LALT]   = kKeyCodeAlt;
	gKeySymMap[SDLK_LSHIFT] = kKeyCodeShift;
	gKeySymMap[SDLK_LCTRL]  = kKeyCodeCtrl;
	gKeySymMap[SDLK_SPACE]  = kKeyCodeSpace;
	gKeySymMap[SDLK_RETURN] = kKeyCodeReturn;
	gKeySymMap[SDLK_TAB]    = kKeyCodeTab;
	gKeySymMap[SDLK_ESCAPE] = kKeyCodeEscape;
	gKeySymMap[SDLK_h]      = kKeyCodeH;
	gKeySymMap[SDLK_i]      = kKeyCodeI;
	gKeySymMap[SDLK_j]      = kKeyCodeJ;
	gKeySymMap[SDLK_k]      = kKeyCodeK;
	gKeySymMap[SDLK_u]      = kKeyCodeU;
	memset(gKeyScanMap, 0, sizeof(gKeyScanMap));
	for (int i = 0; i < 5; ++i) {
		gKeyScanMap[10 + i] = kKeyCode1 + i;
	}
}

static void lockAudio(int lock) {
	if (lock) {
		SDL_LockAudio();
	} else {
		SDL_UnlockAudio();
	}
}

static void setupAudio(GameStub *stub) {
	SDL_AudioSpec desired;
	memset(&desired, 0, sizeof(desired));
	desired.freq = 22050;
	desired.format = AUDIO_S16SYS;
	desired.channels = 2;
	desired.samples = 4096;
	StubMixProc mix = stub->getMixProc(desired.freq, desired.format, lockAudio);
	if (mix.proc) {
		desired.callback = mix.proc;
		desired.userdata = mix.data;
		if (SDL_OpenAudio(&desired, 0) == 0) {
			SDL_PauseAudio(0);
		}
	}
}

struct GetStub_impl {
	GameStub *getGameStub() {
		return GameStub_create();
	}
};

int main(int argc, char *argv[]) {
	GetStub_impl gs;
	GameStub *stub = gs.getGameStub();
	if (!stub) {
		return -1;
	}
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	SDL_ShowCursor(SDL_DISABLE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_SetVideoMode(gWindowW, gWindowH, 0, SDL_OPENGL | SDL_RESIZABLE);
	const int ret = stub->init(argc - 1, argv + 1);
	if (ret != 0) {
		return ret;
	}
	setupKeyMap();
	setupAudio(stub);
	stub->initGL(gWindowW, gWindowH);
	SDL_WM_SetCaption(kCaption, 0);
	bool quitGame = false;
	bool paused = false;
	while (1) {
		int w = gWindowW;
		int h = gWindowH;
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				quitGame = true;
				break;
			case SDL_ACTIVEEVENT:
				if (ev.active.state & SDL_APPINPUTFOCUS) {
					paused = !ev.active.gain;
					SDL_PauseAudio(paused);
				}
				break;
			case SDL_VIDEORESIZE:
				w = ev.resize.w;
				h = ev.resize.h;
				break;
			case SDL_KEYUP:
				if (gKeySymMap[ev.key.keysym.sym] != 0) {
					stub->queueKeyInput(gKeySymMap[ev.key.keysym.sym], 0);
				} else if (gKeyScanMap[ev.key.keysym.scancode] != 0) {
					stub->queueKeyInput(gKeyScanMap[ev.key.keysym.scancode], 0);
				}
				break;
			case SDL_KEYDOWN:
				if (ev.key.keysym.mod & KMOD_MODE) {
					switch (ev.key.keysym.sym) {
					case SDLK_PAGEUP:
						if (gScale < 8) {
							++gScale;
						}
						break;
					case SDLK_PAGEDOWN:
						if (gScale > 1) {
							--gScale;
						}
						break;
					default:
						break;
					}
					w = (int)(kDefaultW * gScale * .5);
					h = (int)(kDefaultH * gScale * .5);
					break;
				}
				if (gKeySymMap[ev.key.keysym.sym] != 0) {
					stub->queueKeyInput(gKeySymMap[ev.key.keysym.sym], 1);
				} else if (gKeyScanMap[ev.key.keysym.scancode] != 0) {
					stub->queueKeyInput(gKeyScanMap[ev.key.keysym.scancode], 1);
				}
				break;
			default:
				break;
			}
		}
		if (quitGame) {
			break;
		}
		if (w != gWindowW || h != gWindowH) {
			gWindowW = w;
			gWindowH = h;
			SDL_SetVideoMode(gWindowW, gWindowH, 0, SDL_OPENGL | SDL_RESIZABLE);
			stub->initGL(gWindowW, gWindowH);
		}
		if (!paused) {
			const unsigned int ticks = SDL_GetTicks();
			stub->doTick(ticks);
			stub->drawGL();
			SDL_GL_SwapBuffers();
		}
		SDL_Delay(kTickDuration);
	}
	stub->quit();
	return 0;
}
