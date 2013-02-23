/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#define LOG_TAG "F2bJni"

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <jni.h>
#include <android/log.h>
#include <dlfcn.h>
#include "stub.h"

static GameStub *g_stub;

extern "C" {

JNIEXPORT void JNICALL Java_org_cyxdown_f2b_F2bJni_drawFrame(JNIEnv *env, jclass c, jint ticks) {
	if (g_stub) {
		g_stub->doTick(ticks);
		g_stub->drawGL();
	}
}

JNIEXPORT void JNICALL Java_org_cyxdown_f2b_F2bJni_initGame(JNIEnv *env, jclass c, jstring jpath, jint w, jint h) {
	g_stub = GameStub_create();
	if (g_stub) {
		const char *dataPath = env->GetStringUTFChars(jpath, 0);
		char optPath[MAXPATHLEN];
		snprintf(optPath, sizeof(optPath), "--datapath=%s", dataPath);
		char *argv[] = { optPath, 0 };
		g_stub->init(1, argv);
		env->ReleaseStringUTFChars(jpath, dataPath);
		g_stub->initGL(w, h);
	}
}

JNIEXPORT void JNICALL Java_org_cyxdown_f2b_F2bJni_quitGame(JNIEnv *env, jclass c) {
	if (g_stub) {
		g_stub->quit();
		g_stub = 0;
	}
}

JNIEXPORT void JNICALL Java_org_cyxdown_f2b_F2bJni_queueKeyEvent(JNIEnv *env, jclass c, jint keycode, jint pressed) {
	if (g_stub) {
		__android_log_print(ANDROID_LOG_INFO, LOG_TAG, "keyEvent %d pressed %d", keycode, pressed);
		g_stub->queueKeyInput(keycode, pressed);
	}
}

}
