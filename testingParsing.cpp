// Assert-based test suite for the shared CalculatorEngine core (parser +
// input/display state machine). Builds without SDL and runs in CI; a failing
// assertion aborts with a non-zero status and fails the job.
#include "CalculatorEngine.h"

#include <iostream>
#include <string>
#include <cassert>

using namespace std;

// Renders the engine's display slots as a string ('_' for an empty slot).
static string displayString(const CalculatorEngine& engine) {
	string out;
	for (int i = 0; i < engine.slotCount(); i++) {
		char c = engine.slotChar(i);
		out += (c == '\0') ? '_' : c;
	}
	return out;
}

static void testParser() {
	int result = 0;

	// valid expressions
	assert(parseEquation("2+2", result) && result == 4);
	assert(parseEquation("42", result) && result == 42);          // bare number
	assert(parseEquation("10-3", result) && result == 7);
	assert(parseEquation("2*3*4", result) && result == 24);
	assert(parseEquation("1+2+3", result) && result == 6);
	assert(parseEquation("2+3*4", result) && result == 14);       // precedence
	assert(parseEquation("2*3+4", result) && result == 10);       // precedence
	assert(parseEquation("7-2-1", result) && result == 4);        // left-associative
	assert(parseEquation(" 5 + 6 ", result) && result == 11);     // whitespace tolerated

	// malformed input / unsupported characters
	assert(!parseEquation("", result));      // empty
	assert(!parseEquation("2+", result));    // trailing operator
	assert(!parseEquation("+2", result));    // leading operator
	assert(!parseEquation("2#3", result));   // unsupported character
	assert(!parseEquation("2 3", result));   // two operands, no operator
}

static void testEngineInput() {
	CalculatorEngine engine;

	// a fresh engine has an empty equation and empty display
	assert(engine.equationText() == "");
	assert(displayString(engine) == "_______");

	// building an equation feeds both the equation text and the display slots
	engine.inputDigit(1);
	engine.inputDigit(2);
	engine.inputOperator('+');
	engine.inputDigit(3);
	assert(engine.equationText() == "12+3");
	assert(displayString(engine) == "12+3___");

	// evaluating shows the result on the display
	int result = 0;
	assert(engine.evaluate(result) && result == 15);
	assert(engine.equationText() == "15");
	assert(displayString(engine) == "15_____");

	// a digit right after a result starts a fresh calculation
	engine.inputDigit(2);
	assert(engine.equationText() == "2");

	// an operator right after a result continues from the result
	engine.clear();
	engine.inputDigit(5);
	assert(engine.evaluate(result) && result == 5);
	engine.inputOperator('+');
	engine.inputDigit(4);
	assert(engine.equationText() == "5+4");
	assert(engine.evaluate(result) && result == 9);

	// out-of-range / unsupported input is ignored
	engine.clear();
	engine.inputDigit(42);     // ignored
	engine.inputOperator('/'); // ignored (division is not supported)
	assert(engine.equationText() == "");

	// clear resets everything
	engine.inputDigit(9);
	engine.clear();
	assert(engine.equationText() == "");
	assert(displayString(engine) == "_______");

	// evaluating an invalid equation fails and leaves the display untouched
	engine.inputDigit(2);
	engine.inputOperator('+');
	assert(!engine.evaluate(result));
	assert(engine.equationText() == "2+");
}

int main() {
	testParser();
	testEngineInput();
	cout << "All engine tests passed.\n";
	return 0;
}
