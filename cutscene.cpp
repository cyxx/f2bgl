/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include "cutscene.h"
#include "file.h"

Cutscene::Cutscene(Render *render, Game *g, Sound *snd) {
	_playCounter = 0;
	memset(_playedTable, 0, sizeof(_playedTable));
	_numToPlayCounter = -1;
	_numToPlay = -1;
	_interrupted = false;
	for (int i = 0; i < kCutscenePlaybackQueueSize; ++i) {
		_playQueue[i] = -1;
	}
	_playQueueSize = 0;
	if (g_hasPsx) {
		_player = CutscenePlayer_Dps_create();
	} else {
		_player = CutscenePlayer_Cin_create();
	}
	_player->_render = render;
	_player->_game   = g;
	_player->_snd    = snd;
}

Cutscene::~Cutscene() {
}

bool Cutscene::load(int num) {
	debug(kDebug_CUTSCENE, "Cutscene::load() num %d", num);
	_interrupted = false;
	assert(num >= 0 && num < kCutsceneScenesCount);
	if (num != 44 && num != 45) {
		if (!_playedTable[num]) {
			_playedTable[num] = 1;
			++_playCounter;
		}
	}
	return _player->load(num);
}

bool Cutscene::update(uint32_t ticks) {
	return _player->update(ticks);
}

void Cutscene::unload() {
	_player->unload();
}

bool Cutscene::isInterrupted() const {
	return _interrupted;
}

void Cutscene::queue(int num, int counter) {
	debug(kDebug_CUTSCENE, "Cutscene::queue() num %d counter %d", num, counter);
	if (_numToPlay < 0) {
		_numToPlay = num;
		_numToPlayCounter = 0;
	} else {
		if (_playQueueSize < kCutscenePlaybackQueueSize) {
			_playQueue[_playQueueSize] = num;
			++_playQueueSize;
		} else {
			warning("Cutscene::queue() queue size %d, skipping cutscene %d", _playQueueSize, num);
		}
	}
}

int Cutscene::dequeue() {
	debug(kDebug_CUTSCENE, "Cutscene::dequeue() _playQueueSize %d", _playQueueSize);
	if (_playQueueSize > 0) {
		const int num = _playQueue[0];
		--_playQueueSize;
		for (int i = 0; i < _playQueueSize; ++i) {
			_playQueue[i] = _playQueue[i + 1];
		}
		_playQueue[_playQueueSize] = -1;
		return num;
	}
	return -1;
}

int Cutscene::changeToNext() {
	const int cutsceneNum = _numToPlay;
	if (!isInterrupted()) {
		do {
			int num = dequeue();
			if (num < 0) {
				switch (_numToPlay) {
				case 47: // logo ea
					num = 39;
					break;
				case 39: // logo dsi
					num = 13;
					break;
				case 13: // 'intro'
					num = 37;
					break;
				case 37: // opening credits - 'title'
					num = 53;
					break;
				case 53: // 'gendeb'
					num = 29;
					break;
				// game completed
				case 48: // closing credits - 'mgm'
					num = 44;
					break;
				case 44: // fade to black - 'fade1'
					num = 13;
					break;
				default:
					num = -1;
					break;
				}
			}
			_numToPlay = num;
		} while (_numToPlay >= 0 && !load(_numToPlay));
	} else {
		_numToPlay = -1;
	}
	return cutsceneNum;
}
