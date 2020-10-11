/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#ifndef CUTSCENE_H__
#define CUTSCENE_H__

#include "file.h"
#include "util.h"

enum {
	kCutsceneScenesCount = 54,
	kCutscenePlaybackQueueSize = 4,
};

struct Game;
struct Sound;
struct Render;

struct CutscenePlayer {
	Render *_render;
	Game *_game;
	Sound *_snd;

	virtual ~CutscenePlayer() {}

	virtual bool load(int num) = 0;
	virtual void unload() = 0;
	virtual bool update(uint32_t ticks) = 0;
};

CutscenePlayer *CutscenePlayer_Dps_create();
CutscenePlayer *CutscenePlayer_Cin_create();

struct Cutscene {
	CutscenePlayer *_player;

	int _playCounter;
	int _playedTable[kCutsceneScenesCount];
	int _numToPlayCounter;
	int _numToPlay;
	bool _interrupted;
	int _playQueue[kCutscenePlaybackQueueSize];
	int _playQueueSize;

	Cutscene(Render *render, Game *g, Sound *snd);
	~Cutscene();

	bool load(int num);
	void unload();
	bool update(uint32_t ticks);
	bool isInterrupted() const;
	void queue(int num, int counter = 0);
	int dequeue();
	int changeToNext();
};

#endif // CUTSCENE_H__
