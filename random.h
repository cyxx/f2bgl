/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#ifndef RANDOM_H__
#define RANDOM_H__

struct Random {
	int _randSeed;

	Random()
		: _randSeed(10001) {
	}

	int getRandomNumber() {
		_randSeed = (3125 * _randSeed) % 8192;
		return _randSeed;
	}
	int getRandomNumberShift(int shift) {
		return (getRandomNumber() - 4096) << shift;
	}
};

#endif // RANDOM_H__
