/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#define LOG_TAG "F2bJni"

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/time.h>
#include <jni.h>
#include <android/keycodes.h>
#include <android/log.h>
#include <dlfcn.h>
#include "stub.h"

static const int kHz = 25;
static const int kSaveGameSlot = 0;

static GameStub *g_stub;
static uint32_t g_timeStamp;
static char g_dataPath[MAXPATHLEN];
static char g_savePath[MAXPATHLEN];

extern "C" {

static uint32_t getTimeMs() {
	struct timeval tv;
	if (gettimeofday(&tv, 0) == 0) {
		return tv.tv_sec * 1000 + tv.tv_usec / 1000;
	}
	return 0;
}

JNIEXPORT void JNICALL Java_org_cyxdown_f2b_F2bJni_drawFrame(JNIEnv *env, jclass c, jint ticks) {
	if (g_stub) {
		const int t = getTimeMs() - g_timeStamp;
		if (t >= 1000 / kHz) {
			g_stub->doTick(ticks);
			g_timeStamp = getTimeMs();
		}
		g_stub->drawGL();
	}
}

JNIEXPORT void JNICALL Java_org_cyxdown_f2b_F2bJni_setDataDirectory(JNIEnv *env, jclass c, jstring jpath) {
	if (jpath) {
		const char *path = env->GetStringUTFChars(jpath, 0);
		snprintf(g_dataPath, sizeof(g_dataPath), "--datapath=%s", path);
		env->ReleaseStringUTFChars(jpath, path);
	}
}

JNIEXPORT void JNICALL Java_org_cyxdown_f2b_F2bJni_setSaveDirectory(JNIEnv *env, jclass c, jstring jpath) {
	if (jpath) {
		const char *path = env->GetStringUTFChars(jpath, 0);
		snprintf(g_dataPath, sizeof(g_dataPath), "--savepath=%s", path);
		env->ReleaseStringUTFChars(jpath, path);
	}
}

JNIEXPORT void JNICALL Java_org_cyxdown_f2b_F2bJni_initGame(JNIEnv *env, jclass c, jstring jpath, jint w, jint h) {
	g_stub = GameStub_create();
	if (g_stub) {
		const char *dataPath = env->GetStringUTFChars(jpath, 0);
		char optPath[MAXPATHLEN];
		snprintf(optPath, sizeof(optPath), "--datapath=%s", dataPath);
		char *argv[] = { 0, optPath, 0 };
		const int ret = g_stub->init(2, argv);
		env->ReleaseStringUTFChars(jpath, dataPath);
		if (ret != 0) {
			__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "F2bJni.init() failed ret %d", ret);
			g_stub = 0;
			return;
		}
		g_stub->initGL(w, h);
		g_timeStamp = getTimeMs();
	}
}

JNIEXPORT void JNICALL Java_org_cyxdown_f2b_F2bJni_quitGame(JNIEnv *env, jclass c) {
	if (g_stub) {
		g_stub->quit();
		g_stub = 0;
	}
}

JNIEXPORT void JNICALL Java_org_cyxdown_f2b_F2bJni_saveGame(JNIEnv *env, jclass c) {
	if (g_stub) {
		g_stub->saveState(kSaveGameSlot);
	}
}

JNIEXPORT void JNICALL Java_org_cyxdown_f2b_F2bJni_queueKeyEvent(JNIEnv *env, jclass c, jint keycode, jint pressed) {
	if (g_stub) {
//		__android_log_print(ANDROID_LOG_INFO, LOG_TAG, "keyEvent %d pressed %d", keycode, pressed);
		int key = -1;
		switch (keycode) {
		case AKEYCODE_SPACE:
			key = kKeyCodeSpace;
			break;
		case AKEYCODE_ALT_LEFT:
		case AKEYCODE_ALT_RIGHT:
			key = kKeyCodeAlt;
			break;
		case AKEYCODE_SHIFT_LEFT:
		case AKEYCODE_SHIFT_RIGHT:
			key = kKeyCodeShift;
			break;
		case AKEYCODE_SOFT_LEFT:
		case AKEYCODE_SOFT_RIGHT:
			key = kKeyCodeCtrl;
			break;
		case AKEYCODE_TAB:
			key = kKeyCodeTab;
			break;
		case AKEYCODE_ENTER:
		case AKEYCODE_DPAD_CENTER:
			key = kKeyCodeReturn;
			break;
		case AKEYCODE_DPAD_UP:
			key = kKeyCodeUp;
			break;
		case AKEYCODE_DPAD_DOWN:
			key = kKeyCodeDown;
			break;
		case AKEYCODE_DPAD_LEFT:
			key = kKeyCodeLeft;
			break;
		case AKEYCODE_DPAD_RIGHT:
			key = kKeyCodeRight;
			break;
		case AKEYCODE_I:
			key = kKeyCodeI;
			break;
		case AKEYCODE_J:
			key = kKeyCodeJ;
			break;
		case AKEYCODE_U:
			key = kKeyCodeU;
			break;
		case AKEYCODE_1:
			key = kKeyCode1;
			break;
		case AKEYCODE_2:
			key = kKeyCode2;
			break;
		case AKEYCODE_3:
			key = kKeyCode3;
			break;
		case AKEYCODE_4:
			key = kKeyCode4;
			break;
		case AKEYCODE_5:
			key = kKeyCode5;
			break;
		}
		if (key != -1) {
			g_stub->queueKeyInput(key, pressed);
		}
	}
}

}
