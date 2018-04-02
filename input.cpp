/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include "game.h"

void Game::updateInput() {
	int currentInput = getCurrentInput();
	const uint8_t inputKey0 = _inputsTable[currentInput].inputKey0;
	const uint8_t inputKey1 = _inputsTable[currentInput].inputKey1;

	if (inp.dirMask & kInputDirRight) {
		_inputDirKeyPressed[inputKey0] |= 1;
	} else {
		_inputDirKeyPressed[inputKey0] &= ~1;
		if (inp.dirMaskPrev & kInputDirRight) {
			_inputDirKeyReleased[inputKey0] |= 1;
		}
	}
	if (inp.dirMask & kInputDirLeft) {
		_inputDirKeyPressed[inputKey0] |= 2;
	} else {
		_inputDirKeyPressed[inputKey0] &= ~2;
		if (inp.dirMaskPrev & kInputDirLeft) {
			_inputDirKeyReleased[inputKey0] |= 2;
		}
	}
	if (inp.dirMask & kInputDirDown) {
		_inputDirKeyPressed[inputKey0] |= 4;
	} else {
		_inputDirKeyPressed[inputKey0] &= ~4;
		if (inp.dirMaskPrev & kInputDirDown) {
			_inputDirKeyReleased[inputKey0] |= 4;
		}
	}
	if (inp.dirMask & kInputDirUp) {
		_inputDirKeyPressed[inputKey0] |= 8;
	} else {
		_inputDirKeyPressed[inputKey0] &= ~8;
		if (inp.dirMaskPrev & kInputDirUp) {
			_inputDirKeyReleased[inputKey0] |= 8;
		}
	}
	if (inp.altKey) {
		_inputButtonKey[inputKey0] |= 1;
	} else {
		_inputButtonKey[inputKey0] &= ~1;
	}
	if (inp.shiftKey) {
		_inputButtonKey[inputKey0] |= 2;
	} else {
		_inputButtonKey[inputKey0] &= ~2;
	}
	if (inp.enterKey) {
		_inputButtonKey[inputKey0] |= 4;
	} else {
		_inputButtonKey[inputKey0] &= ~4;
	}
	if (inp.ctrlKey) {
		_inputButtonKey[inputKey0] |= 8;
	} else {
		_inputButtonKey[inputKey0] &= ~8;
	}
	inp.dirMaskPrev = inp.dirMask;

	_inputKeyAction = inp.spaceKey;
	inp.spaceKey = false;
	_inputKeyUse = inp.useKey;
	inp.useKey = false;
	_inputKeyJump = inp.jumpKey;
	inp.jumpKey = false;

	if (inp.lookAtDir & kInputDirRight) {
		if (_level != 12) {
			_inputDirKeyPressed[inputKey1] |= 1;
		}
	} else {
		_inputDirKeyPressed[inputKey1] &= ~1;
		// _inputDirKeyReleased[inputKey1]
	}
	if (inp.lookAtDir & kInputDirLeft) {
		if (_level != 12) {
			_inputDirKeyPressed[inputKey1] |= 2;
		}
	}
	if (inp.lookAtDir & kInputDirDown) {
		if (_level != 12) {
			_inputDirKeyPressed[inputKey1] |= 4;
		}
	} else {
		_inputDirKeyPressed[inputKey1] &= ~4;
		// _inputDirKeyReleased[inputKey1]
	}
	if (inp.lookAtDir & kInputDirUp) {
		if (_level != 12) {
			_inputDirKeyPressed[inputKey1] |= 8;
		}
	} else {
		_inputDirKeyPressed[inputKey1] &= ~8;
		// _inputDirKeyReleased[inputKey1]
	}

	_inputButtonKey[0] |= _inputButtonMouse;
	_inputDirKeyPressed[0] |= _inputDirMouse;

	for (int i = 0; i < _inputsCount; ++i) {
		_inputsTable[i].sysmaskPrev = _inputsTable[i].sysmask;
		if (currentInput == i) {
			_inputsTable[i].sysmask = _inputButtonKey[0] | (_inputDirKeyPressed[0] << 4) | (_inputButtonKey[1] << 8) | (_inputDirKeyPressed[1] << 12);
			updateInputKeyMask(i);
		} else {
			_inputsTable[i].sysmask = 0;
			_inputsTable[i].keymask = 0;
			_inputsTable[i].keymaskToggle = 0;
		}
	}
	readInputEvents();
}

void Game::updateMouseInput() {
	_inputButtonMouse = 0;
	_inputDirMouse = 0;
	inp.footStepKey = false;
	inp.backStepKey = false;
	for (int i = 0; i < kPlayerInputPointersCount; ++i) {
		if (!inp.pointers[i][0].down) {
			continue;
		}
		for (int j = 0; j < _iconsCount; ++j) {
			if (_iconsTable[j].isCursorOver(inp.pointers[i][0].x, inp.pointers[i][0].y)) {
				switch (_iconsTable[j].action) {
				case kIconActionRun:
					_inputDirMouse |= 8;
					break;
				case kIconActionJump:
					inp.jumpKey = true;
					break;
				case kIconActionDuck:
					_inputDirMouse |= 4;
					break;
				case kIconActionLoad:
					_inputButtonMouse |= 4;
					break;
				case kIconActionGun:
					_inputButtonMouse |= 1;
					break;
				case kIconActionHand:
					_inputButtonMouse |= 8;
					break;
				case kIconActionHandUse:
					inp.spaceKey = true;
					break;
				case kIconActionDirTurn180:
					// shift+down
					_inputButtonMouse |= 2;
					_inputDirMouse |= 4;
					break;
				case kIconActionDirUp:
					// shift+up
					_inputButtonMouse |= 2;
					_inputDirMouse |= 8;
					break;
				case kIconActionDirDown:
					_inputDirMouse |= 4;
					break;
				case kIconActionDirLeft:
					_inputDirMouse |= 2;
					break;
				case kIconActionDirRight:
					_inputDirMouse |= 1;
					break;
				case kIconActionInventory:
					inp.inventoryKey = true;
					break;
				case kIconActionOptions:
					inp.escapeKey = true;
					break;
				}
			}
		}
	}
}

void Game::updateGameInput() {
	for (int i = 0; i < _inputsCount; ++i) {
		_inputDirKeyReleased[_inputsTable[i].inputKey0] = 0;
		_inputDirKeyReleased[_inputsTable[i].inputKey1] = 0;
	}
	if (_res._demoInputDataSize != 0 && _demoInput < _res._demoInputDataSize) {
		while (_ticks == _res._demoInputData[_demoInput].ticks) {
			const ResDemoInput *input = &_res._demoInputData[_demoInput];
			switch (input->key) {
			case 0:
				break;
			case 5:
				inp.shiftKey = input->pressed;
				break;
			case 7: // escape
				break;
			case 22:
				inp.numKeys[4] = true;
				break;
			case 23:
				inp.numKeys[5] = true;
				break;
			case 33:
				inp.useKey = true;
				break;
			case 34:
				inp.jumpKey = true;
				break;
			case 51:
				inp.enterKey = input->pressed;
				break;
			case 62: // del
				break;
			case 64:
				if (input->pressed) {
					inp.lookAtDir |= kInputDirDown;
				} else {
					inp.lookAtDir &= ~kInputDirDown;
				}
				break;
			case 70:
			case 75:
				if (input->pressed) {
					inp.lookAtDir |= kInputDirRight;
				} else {
					inp.lookAtDir &= ~kInputDirRight;
				}
				break;
			case 73:
				if (input->pressed) {
					inp.dirMask |= kInputDirLeft;
				} else {
					inp.dirMask &= ~kInputDirLeft;
				}
				break;
			case 74:
				if (input->pressed) {
					inp.dirMask |= kInputDirDown;
				} else {
					inp.dirMask &= ~kInputDirDown;
				}
				break;
			case 80:
				if (input->pressed) {
					inp.dirMask |= kInputDirRight;
				} else {
					inp.dirMask &= ~kInputDirRight;
				}
				break;
			case 81:
				if (input->pressed) {
					inp.dirMask |= kInputDirUp;
				} else {
					inp.dirMask &= ~kInputDirUp;
				}
				break;
			case 83:
				inp.spaceKey = input->pressed;
				break;
			case 144:
				inp.altKey = input->pressed;
				break;
			case 145:
				inp.ctrlKey = input->pressed;
				break;
			case 154:
				inp.farNear = true;
				break;
			case 163:
				inp.footStepKey = input->pressed;
				break;
			case 164:
				inp.backStepKey = input->pressed;
				break;
			default:
				warning("Game::updateGameInput() unhandled demo input key %d", input->key);
				break;
			}
			++_demoInput;
			if (_demoInput >= _res._demoInputDataSize) {
				break;
			}
		}
	}
	if (_params.mouseMode || _params.touchMode) {
		updateMouseInput();
	}
	bool ctrlKey = inp.ctrlKey;
	if ((_cheats & kCheatActivateButtonToShoot) != 0) {
		if (isConradInShootingPos()) {
			if (inp.spaceKey) {
				inp.ctrlKey = true;
				inp.spaceKey = false;
			}
		}
	}
	if ((_cheats & kCheatShootButtonToStep) != 0) {
		if (!isConradInShootingPos()) {
			if (inp.ctrlKey && (inp.dirMask & (kInputDirLeft | kInputDirRight)) == 0) {
				inp.footStepKey = (inp.dirMask & kInputDirUp) != 0;
				inp.dirMask &= ~kInputDirUp;
				inp.backStepKey = (inp.dirMask & kInputDirDown) != 0;
				inp.dirMask &= ~kInputDirDown;
				inp.ctrlKey = false;
			}
		}
	}
	bool enterKey = inp.enterKey;
	if ((_cheats & kCheatAutoReloadGun) != 0) {
		if (_objectsPtrTable[kObjPtrGun]->customData[0] == 0) {
			if (inp.ctrlKey) {
				inp.enterKey = true;
				inp.ctrlKey = false;
			}
		}
	}
	bool shiftKey = inp.shiftKey;
	if ((_cheats & kCheatStepWithUpDownInShooting) != 0) {
		if (isConradInShootingPos()) {
			if (inp.shiftKey && (inp.dirMask & (kInputDirLeft | kInputDirRight)) == 0) {
				inp.footStepKey = (inp.dirMask & kInputDirUp) != 0;
				inp.dirMask &= ~kInputDirUp;
				inp.backStepKey = (inp.dirMask & kInputDirDown) != 0;
				inp.dirMask &= ~kInputDirDown;
				inp.shiftKey = false;
			}
		}
	}
	updateInput();
	inp.ctrlKey = ctrlKey;
	inp.enterKey = enterKey;
	inp.shiftKey = shiftKey;
}

bool Game::testInputKeyMask(int num, int dir, int button, int index) const {
	GameInput *inp = &_inputsTable[index];
	if (num == 0) {
		return (((inp->sysmask >> 4) & dir) == dir) && ((inp->sysmask & button) == button);
	} else if (num == 1) {
		return (((inp->sysmask >> 12) & dir) == dir) && (((inp->sysmask >> 8) & button) == button);
	}
	return false;
}

bool Game::testInputKeyMaskEq(int num, int dir, int button, int index) const {
	const uint32_t mask = (dir << 4) | button;
	GameInput *inp = &_inputsTable[index];
	if (num == 0) {
		return (inp->sysmask & 255) == mask;
	} else if (num == 1) {
		return ((inp->sysmask >> 8) & 255) == mask;
	}
	return false;
}

bool Game::testInputKeyMaskPrev(int num, int dir, int button, int index) const {
	GameInput *inp = &_inputsTable[index];
	if (num == 0) {
		return (((inp->sysmaskPrev >> 4) & dir) == dir) && ((inp->sysmaskPrev & button) == button);
	} else if (num == 1) {
		return (((inp->sysmaskPrev >> 12) & dir) == dir) && (((inp->sysmaskPrev >> 8) & button) == button);
	}
	return false;
}

void Game::updateInputKeyMask(int inputIndex) {
	_inputsTable[inputIndex].keymaskPrev = _inputsTable[inputIndex].keymask;

	uint32_t keymaskToggle = _inputsTable[inputIndex].keymaskToggle;
	uint32_t keymask = 0;
	const int num0 = 0;
	const int num1 = 1;

	if (testInputKeyMask(num0, 2, 0, inputIndex)) {
		keymask |= 0x1000000;
	}
	if (testInputKeyMask(num0, 1, 0, inputIndex)) {
		keymask |= 0x2000000;
	}
	if (testInputKeyMask(num0, 8, 0, inputIndex)) {
		keymask |= 0x8000000;
	}
	if (testInputKeyMask(num0, 4, 0, inputIndex)) {
		keymask |= 0x10000000;
	}
	if (testInputKeyMask(num0, 0, 1, inputIndex)) {
		keymask |= 0x4000000;
	}
	if (testInputKeyMask(num0, 0, 2, inputIndex)) {
		keymask |= 0x20000000;
	}
	if (inp.footStepKey) {
		keymask |= 1;
	}
	if (testInputKeyMaskEq(num0, 8, 2, inputIndex) || testInputKeyMaskEq(num0, 8 | 2, 2, inputIndex) || testInputKeyMaskEq(num0, 8 | 1, 2, inputIndex)) {
		keymask |= 2;
	}
	if (testInputKeyMask(num0, 2, 0, inputIndex)) {
		keymask |= 0x40;
	}
	if (testInputKeyMask(num0, 1, 0, inputIndex)) {
		keymask |= 0x20;
	}
	if (_inputKeyJump) {
		keymask |= 8;
	}
	if (testInputKeyMaskEq(num0, 8, 0, inputIndex) || testInputKeyMaskEq(num0, 8 | 2, 0, inputIndex) || testInputKeyMaskEq(num0, 8 | 1, 0, inputIndex)) {
		keymask |= 4;
	}
	if (testInputKeyMask(num0, 4, 0, inputIndex)) {
		keymask |= 0x10;
	}
	if (testInputKeyMask(num0, 4, 2, inputIndex)) {
		keymask |= 0x800000;
	}
	if (inp.backStepKey) {
		keymask |= 0x400;
	}
	if (!testInputKeyMaskPrev(num0, 0, 8, inputIndex) && testInputKeyMask(num0, 0, 8, inputIndex)) {
		keymask |= 0x200;
	}
	if (testInputKeyMask(0, 0, 8, inputIndex)) {
		keymask |= 0x40000000;
	}
	if (testInputKeyMask(num0, 0, 8, inputIndex) && _objectsPtrTable[kObjPtrProj] && _objectsPtrTable[kObjPtrProj]->customData[0] > 0) {
		keymask |= 0x80000;
	}
	if (testInputKeyMask(num0, 0, 1, inputIndex)) {
		keymask |= 0x80;
	} else if ((_inputsTable[inputIndex].keymaskPrev & 0x80) == 0x80) {
		keymaskToggle &= ~0x80;
	}
	if (testInputKeyMask(num1, 2, 0, inputIndex)) {
		keymask |= 0x4000;
	}
	if (testInputKeyMask(num1, 1, 0, inputIndex)) {
		keymask |= 0x8000;
	}
	if (testInputKeyMask(num1, 4, 0, inputIndex)) {
		keymask |= 0x10000;
	}
	if (testInputKeyMask(num1, 8, 0, inputIndex)) {
		keymask |= 0x20000;
	}
	if (_cameraDefaultDist) {
		keymask |= 0x40000;
	}
	if (testInputKeyMask(num0, 0, 4, inputIndex)) {
		keymask |= 0x100000;
	}
	if (_inputKeyAction) {
		keymask |= 0x800;
	}
	if (_inputKeyUse) {
		keymask |= 0x80000000;
	}
	if (!testInputKeyMask(num0, 8, 0, inputIndex)) {
		if (testInputKeyMask(num0, 1, 2, inputIndex)) {
			keymask |= 0x200000;
		}
		if (testInputKeyMask(num0, 2, 2, inputIndex)) {
			keymask |= 0x400000;
		}
	}

	_inputsTable[inputIndex].keymask = keymask;
	_inputsTable[inputIndex].keymaskToggle = keymaskToggle;
}
