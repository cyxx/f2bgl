/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include <SDL.h>
#include <string.h>
#include "stub.h"

const char *g_caption = "Fade2Black/OpenGL";

static const char *kIconBmp = "icon.bmp";

static const float kAspectRatio = 4 / 3.;

static const int kDefaultW = 640;
static const int kDefaultH = kDefaultW / kAspectRatio;
//static const int kDefaultH = kDefaultW / (16 / 9.);

static int gWindowW = kDefaultW;
static int gWindowH = kDefaultH;

static float _aspectRatio[4];

static int gScale = 2;
static int gSaveSlot = 1;

static const int kTickDuration = 40;

static const int kJoystickIndex = 0;
static const int kJoystickCommitValue = 16384;

static const int kJoystickMapSize = 8;
static int gJoystickMap[kJoystickMapSize];

static int gGamepadMap[SDL_CONTROLLER_BUTTON_MAX];

static int gKeyScancodeMap[512];

static void setupKeyMap() {
	// keyboard
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
	gKeyScancodeMap[SDL_SCANCODE_I]      = kKeyCodeI;
	gKeyScancodeMap[SDL_SCANCODE_J]      = kKeyCodeJ;
	gKeyScancodeMap[SDL_SCANCODE_U]      = kKeyCodeU;
	gKeyScancodeMap[SDL_SCANCODE_V]      = kKeyCodeAlt;
	gKeyScancodeMap[SDL_SCANCODE_B]      = kKeyCodeCtrl;
	gKeyScancodeMap[SDL_SCANCODE_H]      = kKeyCodeFarNear;
	gKeyScancodeMap[SDL_SCANCODE_K]      = kKeyCodeFarNear;
	gKeyScancodeMap[SDL_SCANCODE_PAGEUP]   = kKeyCodePageUp;
	gKeyScancodeMap[SDL_SCANCODE_PAGEDOWN] = kKeyCodePageDown;
	for (int i = 0; i < 5; ++i) {
		gKeyScancodeMap[SDL_SCANCODE_1 + i] = kKeyCode1 + i;
	}
	// joystick buttons
	memset(gJoystickMap, 0, sizeof(gJoystickMap));
	gJoystickMap[0] = kKeyCodeAlt;
	gJoystickMap[1] = kKeyCodeShift;
	gJoystickMap[2] = kKeyCodeCtrl;
	gJoystickMap[3] = kKeyCodeReturn;
	gJoystickMap[4] = kKeyCodeSpace;
	gJoystickMap[5] = kKeyCodeI;
	gJoystickMap[6] = kKeyCodeJ;
	gJoystickMap[7] = kKeyCodeU;
	// gamecontroller buttons
	memset(gGamepadMap, 0, sizeof(gGamepadMap));
	gGamepadMap[SDL_CONTROLLER_BUTTON_A] = kKeyCodeAlt;
	gGamepadMap[SDL_CONTROLLER_BUTTON_B] = kKeyCodeSpace;
	gGamepadMap[SDL_CONTROLLER_BUTTON_X] = kKeyCodeCtrl;
	gGamepadMap[SDL_CONTROLLER_BUTTON_Y] = kKeyCodeU;
	gGamepadMap[SDL_CONTROLLER_BUTTON_BACK]  = kKeyCodeEscape;
	gGamepadMap[SDL_CONTROLLER_BUTTON_GUIDE] = kKeyCodeI;
	gGamepadMap[SDL_CONTROLLER_BUTTON_START] = kKeyCodeI;
	gGamepadMap[SDL_CONTROLLER_BUTTON_LEFTSTICK]     = kKeyCodeJ;
	gGamepadMap[SDL_CONTROLLER_BUTTON_LEFTSHOULDER]  = kKeyCodeJ;
	gGamepadMap[SDL_CONTROLLER_BUTTON_RIGHTSTICK]    = kKeyCodeShift;
	gGamepadMap[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER] = kKeyCodeShift;
	gGamepadMap[SDL_CONTROLLER_BUTTON_DPAD_UP]    = kKeyCodeUp;
	gGamepadMap[SDL_CONTROLLER_BUTTON_DPAD_DOWN]  = kKeyCodeDown;
	gGamepadMap[SDL_CONTROLLER_BUTTON_DPAD_LEFT]  = kKeyCodeLeft;
	gGamepadMap[SDL_CONTROLLER_BUTTON_DPAD_RIGHT] = kKeyCodeRight;
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

static void setAspectRatio(int w, int h) {
	const float currentAspectRatio = w / (float)h;
	// pillar box
	if (currentAspectRatio > kAspectRatio) {
		const float inset = 1. - kAspectRatio / currentAspectRatio;
		_aspectRatio[0] = inset / 2;
		_aspectRatio[1] = 0.;
		_aspectRatio[2] = 1. - inset;
		_aspectRatio[3] = 1.;
		return;
	}
	// letter box
	if (currentAspectRatio < kAspectRatio) {
		const float inset = 1. - currentAspectRatio / kAspectRatio;
		_aspectRatio[0] = 0.;
		_aspectRatio[1] = inset / 2;
		_aspectRatio[2] = 1.;
		_aspectRatio[3] = 1. - inset;
		return;
	}
}

static int transformPointerX(int x) {
	return int((x - _aspectRatio[0] * gWindowW) * 320 / (_aspectRatio[2] * gWindowW));
}

static int transformPointerY(int y) {
	return int((y - _aspectRatio[1] * gWindowH) * 200 / (_aspectRatio[3] * gWindowH));
}

int main(int argc, char *argv[]) {
	GameStub *stub = GameStub_create();
	if (!stub) {
		return -1;
	}
	int ret = stub->setArgs(argc, argv);
	if (ret != 0) {
		return ret;
	}
	const int displayMode = stub->getDisplayMode();
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
	SDL_ShowCursor(stub->hasCursor() ? SDL_ENABLE : SDL_DISABLE);
	int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
	if (displayMode != kDisplayModeWindowed) {
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}
	SDL_Window *window = SDL_CreateWindow(g_caption, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, gWindowW, gWindowH, flags);
	if (!window) {
		return -1;
	}
	SDL_Surface *icon = SDL_LoadBMP(kIconBmp);
	if (icon) {
		SDL_SetWindowIcon(window, icon);
		SDL_FreeSurface(icon);
	}
	SDL_GetWindowSize(window, &gWindowW, &gWindowH);
	_aspectRatio[0] = 0.;
	_aspectRatio[1] = 0.;
	_aspectRatio[2] = 1.;
	_aspectRatio[3] = 1.;
	if (displayMode != kDisplayModeFullscreenStretch) {
		setAspectRatio(gWindowW, gWindowH);
	}
	SDL_GLContext glcontext = SDL_GL_CreateContext(window);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetSwapInterval(1);
	ret = stub->init();
	if (ret != 0) {
		return ret;
	}
	setupKeyMap();
	setupAudio(stub);
	SDL_Joystick *joystick = 0;
	SDL_GameController *controller = 0;
	if (SDL_NumJoysticks() > 0) {
		SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");
		if (SDL_IsGameController(kJoystickIndex)) {
			controller = SDL_GameControllerOpen(kJoystickIndex);
		}
		if (!controller) {
			joystick = SDL_JoystickOpen(kJoystickIndex);
		}
	}
	stub->initGL(gWindowW, gWindowH, _aspectRatio);
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
				} else {
					switch (ev.key.keysym.sym) {
					case SDLK_PAGEUP:
						if (gScale < 8) {
							++gScale;
							w = (int)(kDefaultW * gScale * .5);
							h = (int)(kDefaultH * gScale * .5);
						}
						break;
					case SDLK_PAGEDOWN:
						if (gScale > 1) {
							--gScale;
							w = (int)(kDefaultW * gScale * .5);
							h = (int)(kDefaultH * gScale * .5);
						}
						break;
					case SDLK_EXCLAIM:
						stub->queueKeyInput(kKeyCodeCheatLifeCounter, 1);
						break;
					case SDLK_s:
						stub->saveState(gSaveSlot);
						break;
					case SDLK_l:
						stub->loadState(gSaveSlot);
						break;
					case SDLK_t:
						stub->takeScreenshot();
						break;
					case SDLK_p:
					case SDLK_PLUS:
					case SDLK_KP_PLUS:
						if (gSaveSlot < 99) {
							++gSaveSlot;
						}
						break;
					case SDLK_m:
					case SDLK_MINUS:
					case SDLK_KP_MINUS:
						if (gSaveSlot > 1) {
							--gSaveSlot;
						}
						break;
					case SDLK_q:
						quitGame = true;
						break;
					}
				}
				break;
			case SDL_KEYDOWN:
				if (gKeyScancodeMap[ev.key.keysym.scancode] != 0) {
					stub->queueKeyInput(gKeyScancodeMap[ev.key.keysym.scancode], 1);
				}
				break;
			case SDL_JOYHATMOTION:
				if (joystick) {
					stub->queueKeyInput(kKeyCodeUp,    (ev.jhat.value & SDL_HAT_UP)    != 0);
					stub->queueKeyInput(kKeyCodeDown,  (ev.jhat.value & SDL_HAT_DOWN)  != 0);
					stub->queueKeyInput(kKeyCodeLeft,  (ev.jhat.value & SDL_HAT_LEFT)  != 0);
					stub->queueKeyInput(kKeyCodeRight, (ev.jhat.value & SDL_HAT_RIGHT) != 0);
				}
				break;
			case SDL_JOYAXISMOTION:
				if (joystick) {
					switch (ev.jaxis.axis) {
					case 0:
						stub->queueKeyInput(kKeyCodeLeft,  (ev.jaxis.value < -kJoystickCommitValue));
						stub->queueKeyInput(kKeyCodeRight, (ev.jaxis.value >  kJoystickCommitValue));
						break;
					case 1:
						stub->queueKeyInput(kKeyCodeUp,   (ev.jaxis.value < -kJoystickCommitValue));
						stub->queueKeyInput(kKeyCodeDown, (ev.jaxis.value >  kJoystickCommitValue));
						break;
					}
				}
				break;
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
				if (joystick) {
					if (ev.jbutton.button >= 0 && ev.jbutton.button < kJoystickMapSize) {
						if (gJoystickMap[ev.jbutton.button] != 0) {
							stub->queueKeyInput(gJoystickMap[ev.jbutton.button], ev.jbutton.state == SDL_PRESSED);
						}
					}
				}
				break;
			case SDL_CONTROLLERAXISMOTION:
				if (controller) {
					switch (ev.caxis.axis) {
					case SDL_CONTROLLER_AXIS_LEFTX:
					case SDL_CONTROLLER_AXIS_RIGHTX:
						stub->queueKeyInput(kKeyCodeLeft,  (ev.caxis.value < -kJoystickCommitValue));
						stub->queueKeyInput(kKeyCodeRight, (ev.caxis.value >  kJoystickCommitValue));
						break;
					case SDL_CONTROLLER_AXIS_LEFTY:
					case SDL_CONTROLLER_AXIS_RIGHTY:
						stub->queueKeyInput(kKeyCodeUp,   (ev.caxis.value < -kJoystickCommitValue));
						stub->queueKeyInput(kKeyCodeDown, (ev.caxis.value >  kJoystickCommitValue));
						break;
					}
				}
				break;
			case SDL_CONTROLLERBUTTONDOWN:
			case SDL_CONTROLLERBUTTONUP:
				if (controller) {
					if (gGamepadMap[ev.cbutton.button] != 0) {
						stub->queueKeyInput(gGamepadMap[ev.cbutton.button], ev.cbutton.state == SDL_PRESSED);
					}
				}
				break;
			case SDL_MOUSEMOTION:
				if (ev.motion.state & SDL_BUTTON_LMASK) {
					stub->queueTouchInput(0, transformPointerX(ev.motion.x), transformPointerY(ev.motion.y), 1);
				}
				if (ev.motion.state & SDL_BUTTON_RMASK) {
					stub->queueTouchInput(1, transformPointerX(ev.motion.x), transformPointerY(ev.motion.y), 1);
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				switch (ev.button.button) {
				case SDL_BUTTON_LEFT:
					stub->queueTouchInput(0, transformPointerX(ev.button.x), transformPointerY(ev.button.y), ev.button.state == SDL_PRESSED);
					break;
				case SDL_BUTTON_RIGHT:
					stub->queueTouchInput(1, transformPointerX(ev.button.x), transformPointerY(ev.button.y), ev.button.state == SDL_PRESSED);
					break;
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
			SDL_SetWindowSize(window, gWindowW, gWindowH);
			stub->initGL(gWindowW, gWindowH, _aspectRatio);
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
	if (controller) {
		SDL_GameControllerClose(controller);
	}
	if (joystick) {
		SDL_JoystickClose(joystick);
	}
	SDL_Quit();
	return 0;
}
