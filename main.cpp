/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include <SDL.h>
#include "stub.h"

const char *g_caption = "Fade2Black/OpenGL";

static const int kDefaultW = 640;
static const int kDefaultH = kDefaultW * 3 / 4;

static int gWindowW = kDefaultW;
static int gWindowH = kDefaultH;

static int gScale = 2;
static int gSaveSlot = 1;

static const int kTickDuration = 40;

static int gKeyScancodeMap[512];

static void setupKeyMap() {
	memset(gKeyScancodeMap, 0, sizeof(gKeyScancodeMap));
	gKeyScancodeMap[SDL_SCANCODE_LEFT]   = kKeyCodeLeft;
	gKeyScancodeMap[SDL_SCANCODE_RIGHT]  = kKeyCodeRight;
	gKeyScancodeMap[SDL_SCANCODE_UP]     = kKeyCodeUp;
	gKeyScancodeMap[SDL_SCANCODE_DOWN]   = kKeyCodeDown;
	gKeyScancodeMap[SDL_SCANCODE_LALT]   = kKeyCodeAlt;
	gKeyScancodeMap[SDL_SCANCODE_RALT]   = kKeyCodeAlt;
	gKeyScancodeMap[SDL_SCANCODE_LSHIFT] = kKeyCodeShift;
	gKeyScancodeMap[SDL_SCANCODE_RSHIFT] = kKeyCodeShift;
	gKeyScancodeMap[SDL_SCANCODE_LCTRL]  = kKeyCodeCtrl;
	gKeyScancodeMap[SDL_SCANCODE_RCTRL]  = kKeyCodeCtrl;
	gKeyScancodeMap[SDL_SCANCODE_SPACE]  = kKeyCodeSpace;
	gKeyScancodeMap[SDL_SCANCODE_RETURN] = kKeyCodeReturn;
	gKeyScancodeMap[SDL_SCANCODE_TAB]    = kKeyCodeTab;
	gKeyScancodeMap[SDL_SCANCODE_ESCAPE] = kKeyCodeEscape;
	gKeyScancodeMap[SDL_SCANCODE_H]      = kKeyCodeH;
	gKeyScancodeMap[SDL_SCANCODE_I]      = kKeyCodeI;
	gKeyScancodeMap[SDL_SCANCODE_J]      = kKeyCodeJ;
	gKeyScancodeMap[SDL_SCANCODE_K]      = kKeyCodeK;
	for (int i = 0; i < 5; ++i) {
		gKeyScancodeMap[SDL_SCANCODE_1 + i] = kKeyCode1 + i;
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
	SDL_AudioSpec desired, obtained;
	memset(&desired, 0, sizeof(desired));
	desired.freq = 22050;
	desired.format = AUDIO_S16SYS;
	desired.channels = 2;
	desired.samples = 4096;
	StubMixProc mix = stub->getMixProc(desired.freq, desired.format, lockAudio);
	if (mix.proc) {
		desired.callback = mix.proc;
		desired.userdata = mix.data;
		if (SDL_OpenAudio(&desired, &obtained) == 0) {
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
	SDL_Window *window = SDL_CreateWindow(g_caption, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, gWindowW, gWindowH, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (!window) {
		return -1;
	}
	SDL_GetWindowSize(window, &gWindowW, &gWindowH);
	SDL_GLContext glcontext = SDL_GL_CreateContext(window);
	const int ret = stub->init(argc, argv);
	if (ret != 0) {
		return ret;
	}
	setupKeyMap();
	setupAudio(stub);
	stub->initGL(gWindowW, gWindowH);
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
			case SDL_WINDOWEVENT:
				switch (ev.window.event) {
				case SDL_WINDOWEVENT_RESIZED:
					w = ev.window.data1;
					h = ev.window.data2;
					break;
				case SDL_WINDOWEVENT_FOCUS_GAINED:
				case SDL_WINDOWEVENT_FOCUS_LOST:
					paused = (ev.window.event == SDL_WINDOWEVENT_FOCUS_LOST);
					SDL_PauseAudio(paused);
					break;
				}
				break;
			case SDL_KEYUP:
				if (gKeyScancodeMap[ev.key.keysym.scancode] != 0) {
					stub->queueKeyInput(gKeyScancodeMap[ev.key.keysym.scancode], 0);
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
				} else if (ev.key.keysym.mod & KMOD_CTRL) {
					switch (ev.key.keysym.sym) {
					case SDLK_s:
						stub->saveState(gSaveSlot);
						break;
					case SDLK_l:
						stub->loadState(gSaveSlot);
						break;
					case SDLK_KP_PLUS:
					case SDLK_PAGEUP:
						if (gSaveSlot < 99) {
							++gSaveSlot;
						}
						break;
					case SDLK_KP_MINUS:
					case SDLK_PAGEDOWN:
						if (gSaveSlot > 1) {
							--gSaveSlot;
						}
						break;
					}
				}
				if (gKeyScancodeMap[ev.key.keysym.scancode] != 0) {
					stub->queueKeyInput(gKeyScancodeMap[ev.key.keysym.scancode], 1);
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
			stub->initGL(gWindowW, gWindowH);
		}
		if (!paused) {
			const unsigned int ticks = SDL_GetTicks();
			stub->doTick(ticks);
			stub->drawGL();
			SDL_GL_SwapWindow(window);
		}
		SDL_Delay(kTickDuration);
	}
	SDL_PauseAudio(1);
	stub->quit();
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
