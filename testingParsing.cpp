// Assert-based test suite for the shared CalculatorEngine core (parser +
// input/display state machine). Builds without SDL and runs in CI; a failing
// assertion aborts with a non-zero status and fails the job.
#include "CalculatorEngine.h"

#include <iostream>
#include <string>
#include <cassert>
#include <cmath>

using namespace std;

// Floating-point results from different operation orders (e.g. "0.1+0.2")
// are not always bit-exact, so equality checks against a computed result use
// a small epsilon instead of ==.
static bool nearlyEqual(double a, double b) {
	return fabs(a - b) < 1e-9;
}

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
	double result = 0;

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

	// division: true (floating-point) division, same precedence as '*'
	assert(parseEquation("8/2", result) && result == 4);
	assert(parseEquation("7/2", result) && result == 3.5);        // no longer truncated
	assert(parseEquation("0-7/2", result) && result == -3.5);     // '/' binds tighter than '-'
	assert(parseEquation("100/5/2", result) && result == 10);     // left-associative
	assert(parseEquation("2+8/4", result) && result == 4);        // '/' binds tighter than '+'
	assert(parseEquation("8/4+2", result) && result == 4);        // precedence, other order
	assert(parseEquation("2*6/4", result) && result == 3);        // equal precedence, left to right
	assert(parseEquation("6/4*2", result) && result == 3);        // (6/4)*2 == 3.0, not 6/(4*2)

	// decimal literals: at most one '.' per number, in any position
	assert(parseEquation("2.5+1", result) && result == 3.5);        // digits on both sides of '.'
	assert(parseEquation(".5+1", result) && result == 1.5);         // leading '.', no integer part
	assert(parseEquation("2.+3", result) && result == 5);           // trailing '.', no fractional part
	assert(parseEquation("2.5*2", result) && result == 5);
	assert(parseEquation("1/4", result) && result == 0.25);         // division can produce a fraction
	assert(parseEquation("0.1+0.2", result) && nearlyEqual(result, 0.3)); // binary float rounding tolerated

	// malformed decimal literals
	assert(!parseEquation("2.3.4", result));  // a second '.' within the same number
	assert(!parseEquation(".", result));      // a lone '.' with no digits at all
	assert(!parseEquation("2+.", result));    // '.' with no digits after an operator

	// overflow to a non-finite value is rejected rather than wrapping
	string hugeDigits(400, '9');
	assert(!parseEquation(hugeDigits, result));                       // literal overflows to infinity
	assert(!parseEquation(hugeDigits + "*" + hugeDigits, result));    // product overflows to infinity

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
	double result = 0;
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

	// a decimal point feeds the display and the eventual parse like any digit
	engine.clear();
	engine.inputDigit(2);
	engine.inputDecimalPoint();
	engine.inputDigit(5);
	assert(engine.equationText() == "2.5");
	assert(displayString(engine) == "2.5____");
	assert(engine.evaluate(result) && result == 2.5);
	assert(displayString(engine) == "2.5____");

	// a second '.' within the same number is ignored
	engine.clear();
	engine.inputDigit(1);
	engine.inputDecimalPoint();
	engine.inputDecimalPoint(); // ignored: "1." already has a '.'
	engine.inputDigit(2);
	assert(engine.equationText() == "1.2");

	// a '.' right after an operator starts a new number, so it is allowed again
	engine.clear();
	engine.inputDigit(1);
	engine.inputDecimalPoint();
	engine.inputDigit(2);
	engine.inputOperator('+');
	engine.inputDecimalPoint();
	engine.inputDigit(5);
	assert(engine.equationText() == "1.2+.5");
	assert(engine.evaluate(result) && result == 1.7);

	// a division whose true quotient is fractional now displays the fraction
	// instead of being truncated to an integer
	engine.clear();
	engine.inputDigit(1);
	engine.inputOperator('/');
	engine.inputDigit(4);
	assert(engine.evaluate(result) && result == 0.25);
	assert(displayString(engine) == "0.25___");

	// a '.' typed right after a result starts a brand-new calculation, just
	// like a digit does
	engine.clear();
	engine.inputDigit(5);
	assert(engine.evaluate(result) && result == 5);
	engine.inputDecimalPoint();
	engine.inputDigit(5);
	assert(engine.equationText() == ".5");
}

int main() {
	testParser();
	testEngineInput();
	cout << "All engine tests passed.\n";
	return 0;
}
