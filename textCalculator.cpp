// Text/CLI frontend for the calculator. It shares the exact same
// CalculatorEngine as the SDL GUI — this file contains no calculation or
// display logic of its own, only the translation between terminal I/O and the
// engine. It is the simplest demonstration that the engine is UI-agnostic.
#include "CalculatorEngine.h"

#include <iostream>
#include <string>
#include <cctype>

using namespace std;

// Renders the engine's display slots as a single line ('_' for an empty slot).
static void printDisplay(const CalculatorEngine& engine) {
	cout << "  [";
	for (int i = 0; i < engine.slotCount(); i++) {
		char c = engine.slotChar(i);
		cout << (c == '\0' ? '_' : c);
	}
	cout << "]\n";
}

int main() {
	CalculatorEngine engine;
	cout << "Simple Calculator (text mode)\n";
	cout << "Type digits and + - * to build an equation, then:\n";
	cout << "  '='  evaluate    'c'  clear    'q'  quit\n";
	printDisplay(engine);

	string line;
	while (getline(cin, line)) {
		for (char c : line) {
			if (isdigit((unsigned char)c)) {
				engine.inputDigit(c - '0');
			} else if (c == '+' || c == '-' || c == '*') {
				engine.inputOperator(c);
			} else if (c == 'c' || c == 'C') {
				engine.clear();
			} else if (c == 'q' || c == 'Q') {
				return 0;
			} else if (c == '=') {
				int result = 0;
				if (engine.evaluate(result)) {
					cout << "= " << result << "\n";
				} else {
					cout << "(invalid equation)\n";
				}
			}
			// any other character (spaces, etc.) is ignored
		}
		printDisplay(engine);
	}
	return 0;
}
