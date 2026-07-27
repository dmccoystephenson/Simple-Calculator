// Assert-based test suite for the shared CalculatorEngine core (parser +
// input/display state machine). Builds without SDL and runs in CI; a failing
// assertion aborts with a non-zero status and fails the job.
#include "CalculatorEngine.h"

#include <iostream>
#include <string>
#include <cassert>
#include <climits>

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

	// division: integer, truncated toward zero, same precedence as '*'
	assert(parseEquation("8/2", result) && result == 4);
	assert(parseEquation("7/2", result) && result == 3);          // truncates, does not round
	assert(parseEquation("0-7/2", result) && result == -3);       // '/' binds tighter than '-'
	assert(parseEquation("100/5/2", result) && result == 10);     // left-associative
	assert(parseEquation("2+8/4", result) && result == 4);        // '/' binds tighter than '+'
	assert(parseEquation("8/4+2", result) && result == 4);        // precedence, other order
	assert(parseEquation("2*6/4", result) && result == 3);        // equal precedence, left to right
	assert(parseEquation("6/4*2", result) && result == 2);        // (6/4)*2 == 2, not 6/(4*2)

	// values at the edge of the int range still evaluate normally
	assert(parseEquation("2147483647", result) && result == INT_MAX);       // INT_MAX literal
	assert(parseEquation("2147483646+1", result) && result == INT_MAX);     // sum lands exactly on INT_MAX
	assert(parseEquation("0-2147483647", result) && result == -INT_MAX);    // just inside the low end

	// overflow is rejected as malformed rather than wrapping (undefined behavior)
	assert(!parseEquation("2147483648", result));            // literal is INT_MAX + 1
	assert(!parseEquation("99999999999999999999", result));  // literal far past INT_MAX
	assert(!parseEquation("2147483647+1", result));          // addition overflows
	assert(!parseEquation("0-2147483647-2", result));        // subtraction underflows past INT_MIN
	assert(!parseEquation("99999*99999", result));           // product is 9,999,800,001
	assert(!parseEquation("2+99999999999", result));         // oversized literal fails the whole equation

	// malformed input / unsupported characters
	assert(!parseEquation("", result));      // empty
	assert(!parseEquation("2+", result));    // trailing operator
	assert(!parseEquation("+2", result));    // leading operator
	assert(!parseEquation("2#3", result));   // unsupported character
	assert(!parseEquation("2 3", result));   // two operands, no operator
	assert(!parseEquation("2/", result));    // trailing division operator
	assert(!parseEquation("4/0", result));   // division by zero is rejected
	assert(!parseEquation("1+4/0", result)); // division by zero anywhere fails the whole equation
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

	// division is accepted like any other operator
	engine.clear();
	engine.inputDigit(9);
	engine.inputOperator('/');
	engine.inputDigit(3);
	assert(engine.equationText() == "9/3");
	assert(displayString(engine) == "9/3____");
	assert(engine.evaluate(result) && result == 3);
	assert(displayString(engine) == "3______");

	// dividing by zero fails and leaves the equation untouched
	engine.clear();
	engine.inputDigit(6);
	engine.inputOperator('/');
	engine.inputDigit(0);
	assert(!engine.evaluate(result));
	assert(engine.equationText() == "6/0");

	// out-of-range / unsupported input is ignored
	engine.clear();
	engine.inputDigit(42);     // ignored
	engine.inputOperator('%'); // ignored (modulo is not supported)
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

	// a result that exactly fills the 7-slot display still shows in full
	engine.clear();
	engine.inputDigit(5);
	engine.inputDigit(0);
	engine.inputDigit(0);
	engine.inputDigit(0);
	engine.inputDigit(0);
	engine.inputDigit(0);
	engine.inputDigit(0);
	engine.inputOperator('+');
	engine.inputDigit(4);
	engine.inputDigit(9);
	engine.inputDigit(9);
	engine.inputDigit(9);
	engine.inputDigit(9);
	engine.inputDigit(9);
	engine.inputDigit(9);
	assert(engine.evaluate(result) && result == 9999999);
	assert(displayString(engine) == "9999999");

	// a result one digit too wide for the display is rejected rather than
	// silently truncated, leaving the equation/display untouched
	engine.clear();
	engine.inputDigit(5);
	engine.inputDigit(0);
	engine.inputDigit(0);
	engine.inputDigit(0);
	engine.inputDigit(0);
	engine.inputDigit(0);
	engine.inputDigit(0);
	engine.inputDigit(0);
	engine.inputOperator('+');
	engine.inputDigit(4);
	engine.inputDigit(9);
	engine.inputDigit(9);
	engine.inputDigit(9);
	engine.inputDigit(9);
	engine.inputDigit(9);
	engine.inputDigit(9);
	engine.inputDigit(9);
	assert(engine.equationText() == "50000000+49999999");
	assert(!engine.evaluate(result));
	assert(engine.equationText() == "50000000+49999999");
}

int main() {
	testParser();
	testEngineInput();
	cout << "All engine tests passed.\n";
	return 0;
}
