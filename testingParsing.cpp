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

	// a '-' where an operand is expected is a sign, not a subtraction
	assert(parseEquation("-7", result) && result == -7);           // a bare negative literal
	// the sign binds to the literal, not to the rest of the expression:
	// negating the whole thing would give -10 here, not -4
	assert(parseEquation("-7+3", result) && result == -4);         // continuing from a negative result
	assert(parseEquation("-7-3", result) && result == -10);        // sign then a real subtraction
	assert(parseEquation("2*-3", result) && result == -6);         // sign after an operator
	assert(parseEquation("5--3", result) && result == 8);          // subtracting a negative
	assert(parseEquation("- 7", result) && result == -7);          // whitespace between sign and literal
	assert(parseEquation("-.5+1", result) && result == 0.5);       // sign on a leading-dot literal
	assert(parseEquation("-2*3", result) && result == -6);         // a signed literal obeys normal precedence
	assert(!parseEquation("--3", result));                          // a doubled sign is malformed
	assert(!parseEquation("-", result));                            // a sign with no literal at all
	assert(!parseEquation("5*-", result));                          // a dangling sign at the end
	string hugeNegative = "-" + string(400, '9');
	assert(!parseEquation(hugeNegative, result));                   // a negated literal that overflows is still rejected

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
	assert(!parseEquation("+2", result));    // leading '+' is not a sign (unlike '-')
	assert(!parseEquation("2#3", result));   // unsupported character
	assert(!parseEquation("2 3", result));   // two operands, no operator
	assert(!parseEquation("2/", result));    // trailing division operator
	assert(!parseEquation("4/0", result));   // division by zero is rejected
	assert(!parseEquation("1+4/0", result)); // division by zero anywhere fails the whole equation
}

static void testEngineInput() {
	CalculatorEngine engine;
	double result = 0;

	// a fresh engine has an empty equation and empty display
	assert(engine.equationText() == "");
	assert(displayString(engine) == "_______");

	// evaluating a still-empty equation fails and leaves the display untouched,
	// the same way parseEquation("") does at the parser level
	assert(!engine.evaluate(result));
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

	// unlike inputDecimalPoint(), inputOperator() does not look at what
	// precedes it: an operator typed as the very first input (no operand
	// yet) is still appended to the equation/display, and only rejected
	// later at evaluate() time, same as parseEquation("+2") is
	engine.clear();
	engine.inputOperator('+');
	assert(engine.equationText() == "+");
	assert(displayString(engine) == "+______");
	assert(!engine.evaluate(result));
	assert(engine.equationText() == "+");

	// likewise, two operators typed back to back are both appended (each is
	// individually valid to inputOperator) and only rejected together at
	// evaluate() time, same as parseEquation("5+*3") is
	engine.clear();
	engine.inputDigit(5);
	engine.inputOperator('+');
	engine.inputOperator('*');
	engine.inputDigit(3);
	assert(engine.equationText() == "5+*3");
	assert(!engine.evaluate(result));
	assert(engine.equationText() == "5+*3");

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

	// a decimal point as the very first input on a fresh equation (not mid-number,
	// not right after a result) starts a leading-dot literal, same as ".5+1" at
	// the parser level
	engine.clear();
	engine.inputDecimalPoint();
	engine.inputDigit(5);
	assert(engine.equationText() == ".5");
	assert(engine.evaluate(result) && result == 0.5);

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

	// a negative result displays with a leading '-' (the sign counts against
	// the 7-slot budget, same as formatForDisplay documents)
	engine.clear();
	engine.inputDigit(3);
	engine.inputOperator('-');
	engine.inputDigit(1);
	engine.inputDigit(0);
	assert(engine.evaluate(result) && result == -7);
	assert(engine.equationText() == "-7");
	assert(displayString(engine) == "-7_____");

	// a negative result with a fractional part still fits the sign, integer
	// part, '.', and fraction within the 7-slot budget
	engine.clear();
	engine.inputDigit(1);
	engine.inputOperator('-');
	engine.inputDigit(3);
	engine.inputDecimalPoint();
	engine.inputDigit(5);
	assert(engine.evaluate(result) && result == -2.5);
	assert(displayString(engine) == "-2.5___");

	// a calculation continues from a negative result: the leading '-' of the
	// displayed result is re-read as a sign, not as a dangling operator
	engine.clear();
	engine.inputDigit(3);
	engine.inputOperator('-');
	engine.inputDigit(1);
	engine.inputDigit(0);
	assert(engine.evaluate(result) && result == -7);
	assert(engine.equationText() == "-7");
	engine.inputOperator('+');
	engine.inputDigit(3);
	assert(engine.equationText() == "-7+3");
	assert(engine.evaluate(result) && result == -4);
	assert(displayString(engine) == "-4_____");

	// the same works for an operator that isn't '+'
	engine.clear();
	engine.inputDigit(3);
	engine.inputOperator('-');
	engine.inputDigit(1);
	engine.inputDigit(0);
	assert(engine.evaluate(result) && result == -7);
	engine.inputOperator('*');
	engine.inputDigit(2);
	assert(engine.equationText() == "-7*2");
	assert(engine.evaluate(result) && result == -14);

	// '-' as the very first input enters a negative number directly
	engine.clear();
	engine.inputOperator('-');
	engine.inputDigit(7);
	assert(engine.equationText() == "-7");
	assert(displayString(engine) == "-7_____");
	assert(engine.evaluate(result) && result == -7);

	// a decimal point after a leading sign belongs to the signed literal
	engine.clear();
	engine.inputOperator('-');
	engine.inputDigit(2);
	engine.inputDecimalPoint();
	engine.inputDigit(5);
	assert(engine.equationText() == "-2.5");
	assert(engine.evaluate(result) && result == -2.5);

	// "5--3" is now valid (subtracting a negative), but only one sign may
	// follow an operator: a third consecutive '-' has no operand position
	// left to be a sign in, so it is rejected at evaluate() time
	engine.clear();
	engine.inputDigit(5);
	engine.inputOperator('-');
	engine.inputOperator('-');
	engine.inputOperator('-');
	engine.inputDigit(3);
	assert(engine.equationText() == "5---3");
	assert(!engine.evaluate(result));

	// a negative magnitude too small to survive rounding displays as a plain
	// "0", not a misleading "-0" (the out-param keeps the true value)
	engine.clear();
	engine.inputDigit(1);
	engine.inputOperator('-');
	engine.inputDigit(1);
	engine.inputDecimalPoint();
	engine.inputDigit(0);
	engine.inputDigit(0);
	engine.inputDigit(0);
	engine.inputDigit(0);
	engine.inputDigit(1);
	assert(engine.equationText() == "1-1.00001");
	assert(engine.evaluate(result) && nearlyEqual(result, -0.00001));
	assert(engine.equationText() == "0");
	assert(displayString(engine) == "0______");

	// a result that is IEEE negative zero displays as a plain "0" too. It is
	// not caught by the rounding case above: -0.0 is not < 0, so the sign is
	// carried by the value's sign bit rather than by its magnitude, and it
	// used to reach the display via snprintf ("%.5f" of -0.0 is "-0.00000",
	// which the retry loop and trailing-zero strip reduced to "-0")
	engine.clear();
	engine.inputOperator('-');
	engine.inputDigit(0);
	assert(engine.equationText() == "-0");
	assert(engine.evaluate(result) && result == 0);
	assert(engine.equationText() == "0");
	assert(displayString(engine) == "0______");

	// negative zero is equally reachable as the product or quotient of zero and
	// a negative operand, not just as a signed zero literal
	engine.clear();
	engine.inputDigit(0);
	engine.inputOperator('*');
	engine.inputOperator('-');
	engine.inputDigit(3);
	assert(engine.equationText() == "0*-3");
	assert(engine.evaluate(result) && result == 0);
	assert(engine.equationText() == "0");

	engine.clear();
	engine.inputDigit(0);
	engine.inputOperator('/');
	engine.inputOperator('-');
	engine.inputDigit(5);
	assert(engine.equationText() == "0/-5");
	assert(engine.evaluate(result) && result == 0);
	assert(engine.equationText() == "0");

	// a non-terminating fraction is rounded to fill the display's whole
	// fractional-digit budget, not truncated to a fixed few digits
	engine.clear();
	engine.inputDigit(1);
	engine.inputOperator('/');
	engine.inputDigit(3);
	assert(engine.evaluate(result) && nearlyEqual(result, 1.0 / 3.0));
	assert(engine.equationText() == "0.33333");
	assert(displayString(engine) == "0.33333");

	// rounding that carries into an extra integer digit is handled by
	// formatForDisplay's retry loop (see its comment on "9.996" -> "10.0"):
	// the initial 5-decimal attempt rounds up to "10.00000" (8 chars, over
	// budget), the loop retries at 4 decimals ("10.0000", fits), then the
	// all-zero fraction is stripped down to a bare "10"
	engine.clear();
	engine.inputDigit(9);
	engine.inputDecimalPoint();
	engine.inputDigit(9);
	engine.inputDigit(9);
	engine.inputDigit(9);
	engine.inputDigit(9);
	engine.inputDigit(9);
	engine.inputDigit(9);
	engine.inputOperator('+');
	engine.inputDigit(0);
	assert(engine.equationText() == "9.999999+0");
	assert(engine.evaluate(result) && nearlyEqual(result, 9.999999));
	assert(engine.equationText() == "10");
	assert(displayString(engine) == "10_____");

	// the display is a window onto the end of the equation: once the equation
	// outgrows the seven slots the window scrolls, so what is shown stays a
	// truthful suffix of what will be evaluated (it used to overwrite the
	// final slot, showing "1234569" here and dropping the 7 and 8 entirely)
	engine.clear();
	for (int d = 1; d <= 9; d++) {
		engine.inputDigit(d);
	}
	assert(engine.equationText() == "123456789");
	assert(displayString(engine) == "3456789");

	// the window scrolls one character at a time, and an equation of exactly
	// SLOT_COUNT characters is still shown in full before it starts moving
	engine.clear();
	for (int d = 1; d <= 7; d++) {
		engine.inputDigit(d);
	}
	assert(displayString(engine) == "1234567");
	engine.inputDigit(8);
	assert(displayString(engine) == "2345678");

	// continuing from a result that already fills the display: the operator no
	// longer overwrites the result's last digit (which showed "0.3333+" for the
	// equation "0.33333+", misrepresenting the value being carried forward)
	engine.clear();
	engine.inputDigit(1);
	engine.inputOperator('/');
	engine.inputDigit(3);
	assert(engine.evaluate(result) && nearlyEqual(result, 1.0 / 3.0));
	assert(displayString(engine) == "0.33333");
	engine.inputOperator('+');
	assert(engine.equationText() == "0.33333+");
	assert(displayString(engine) == ".33333+");
	engine.inputDigit(1);
	assert(engine.equationText() == "0.33333+1");
	// the carried-forward operand is the displayed (rounded) "0.33333", not the
	// full 1/3, so the sum is exactly 1.33333
	assert(engine.evaluate(result) && nearlyEqual(result, 1.33333));
	// evaluating shrinks the equation back to the result, so the window stops
	// scrolling and the display is left-aligned again
	assert(displayString(engine) == "1.33333");
}

int main() {
	testParser();
	testEngineInput();
	cout << "All engine tests passed.\n";
	return 0;
}
