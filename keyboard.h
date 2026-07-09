#pragma once

#include <Keypad.h>

char keys[4][4] = {
	{'1','2','3','A'},
	{'4','5','6','B'},
	{'7','8','9','C'},
	{'*','0','#','D'}
};

char modified[10][5] = {
	{' ', '\0', '\0', '\0'},
	{'e', 'f', 'v', '('},
	{'a', '*', '#', '@'},
	{'o', 'u', 'w', ')'},
	{'t', 'd', '\'', '\"'},
	{'i', 'y', 'j', '/'},
	{'n', 'm', '!', '?'},
	{'s', 'c', 'x', 'z'},
	{'r', 'l', 'p', 'b'},
	{'h', 'g', 'k', 'q'}
};

byte rowPins[4] = {2, 3, 4, 5};
byte colPins[4] = {6, 7, 8, 9};

enum LockMode {
	None, Caps, Nums
};

class Keyboard {
	private:
		Keypad keypad;
		int modifier = 0;

	public:
		LockMode mode = LockMode::None;
		Keyboard() : keypad(makeKeymap(keys), rowPins, colPins, 4, 4) {}

		char getKey(bool raw=false) {
			char key = keypad.getKey();
			if (raw) return key;
			if (!key) return '\0';

			if (key >= 65 && key <= 67) {
				modifier = key - 64;
			} else if (key == 68) {
				if (mode == LockMode::None) {
					switch (modifier) {
						case 0:
							mode = LockMode::Caps;
							break;
						case 1:
							mode = LockMode::Nums;
							break;
					}
					modifier = 0;
				} else mode = LockMode::None;
			} else if (key >= 48 && key <= 57) {
				int number = key - 48;
				char letter = modified[number][modifier];
				modifier = 0;
				if (mode == LockMode::None) return letter;
				else if (mode == LockMode::Caps && letter >= 97 && letter <= 122) return letter - 32;
				else if (mode == LockMode::Nums) return key;
			} else if (key == '*' || key == '#') {
				int m = modifier;
				modifier = 0;
				switch (key) {
					case '*':
						return m ? ',' : '\b';
					case '#':
						return m ? '.' : '\n';
				}
			}

			return '\0';
		}
};