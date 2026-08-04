#include "CalculatorEngine.h"

#include <queue>
#include <vector>
#include <cctype>
#include <cmath>
#include <cstdio>

using namespace std;

// ---------------------------------------------------------------------------
// Shunting-yard parser (https://en.wikipedia.org/wiki/Shunting-yard_algorithm)
// ---------------------------------------------------------------------------

namespace {
	// A single parsed token: either a number or a binary operator.
	struct Token {
		bool isNumber;
		double value; // valid when isNumber is true
		char op;      // valid when isNumber is false
	};

	// Operator precedence (higher binds tighter); 0 for non-operators.
	int precedence(char op) {
		if (op == '+' || op == '-') return 1;
		if (op == '*' || op == '/') return 2;
		return 0;
	}

	// Applies a binary operator to two operands, writing the result to out;
	// returns false for an unsupported operator, a division by zero, or a
	// result that is not finite (e.g. overflows to infinity).
	bool applyOperator(double a, double b, char op, double& out) {
		switch (op) {
			case '+': out = a + b; break;
			case '-': out = a - b; break;
			case '*': out = a * b; break;
			// a zero divisor would produce infinity/NaN, so it is reported as a
			// malformed equation instead, matching how it was always treated
			case '/': if (b == 0) return false; out = a / b; break;
			default: return false;
		}
		return isfinite(out);
	}

	// Formats value to fit within slotCount display characters (sign and
	// decimal point both count against the budget), rounding the fractional
	// part to whatever precision remains once the integer part and sign are
	// accounted for; a whole-number result is shown with no decimal point at
	// all, and a negative magnitude small enough to round away entirely is
	// shown as a plain "0" rather than "-0". Returns false if even the rounded
	// integer part does not fit.
	bool formatForDisplay(double value, int slotCount, string& out) {
		if (!isfinite(value)) return false;

		bool negative = value < 0;
		double magnitude = negative ? -value : value;
		// nothing that wide could ever fit in slotCount<=7 anyway; bailing out
		// here keeps the long long casts below well within range
		if (magnitude >= 1e15) return false;

		int budget = slotCount - (negative ? 1 : 0);
		if (budget <= 0) return false;

		string intDigits = to_string((long long)magnitude); // truncated; refined below
		if ((int)intDigits.size() > budget) return false;

		int decimalDigits = budget - (int)intDigits.size() - 1; // -1 reserves the '.'
		string text;
		if (decimalDigits <= 0) {
			// no room for a fractional part: round to the nearest integer
			text = to_string((long long)llround(magnitude));
		} else {
			char buffer[64];
			snprintf(buffer, sizeof(buffer), "%.*f", decimalDigits, magnitude);
			text = buffer;
			// rounding can carry into an extra integer digit (e.g. "9.996" with
			// one decimal place becomes "10.0"); drop a decimal digit and retry
			// until it fits, falling back to a bare rounded integer
			while ((int)text.size() > budget && decimalDigits > 0) {
				decimalDigits--;
				if (decimalDigits == 0) {
					text = to_string((long long)llround(magnitude));
				} else {
					snprintf(buffer, sizeof(buffer), "%.*f", decimalDigits, magnitude);
					text = buffer;
				}
			}
			// strip a trailing all-zero fraction so a whole-number result
			// displays the same way integer arithmetic always has ("4", not "4.0")
			size_t dot = text.find('.');
			if (dot != string::npos) {
				size_t lastNonZero = text.find_last_not_of('0');
				if (lastNonZero == dot) lastNonZero--; // fraction was all zeros
				text.erase(lastNonZero + 1);
			}
		}
		if ((int)text.size() > budget) return false; // rounding still doesn't fit

		// a magnitude small enough to round away entirely displays as a plain
		// "0" — keeping the sign here would render a misleading "-0"
		out = (negative && text != "0") ? ("-" + text) : text;
		return true;
	}
}

bool parseEquation(const string& equation, double& result) {
	// --- shunting-yard: build the RPN output queue ---
	queue<Token> output;
	vector<char> operators; // operator stack
	int i = 0;
	int n = (int)equation.length();
	bool expectOperand = true; // operand and operator must alternate
	bool negatePending = false; // a unary '-' waiting for the literal it negates
	while (i < n) {
		char c = equation[i];
		if (isspace((unsigned char)c)) { i++; continue; }
		// a '-' where an operand is expected is a sign, not a subtraction: it
		// negates the literal that follows. This is what lets a calculation
		// continue from a negative result ("-7+3") and lets a negative number
		// be entered at all ("-7"). Only one sign is allowed per operand
		// position: a second consecutive one ("--3") falls through to the
		// operator branch below and is rejected as malformed, the same way any
		// other doubled operator ("5+*3") is.
		if (c == '-' && expectOperand && !negatePending) {
			negatePending = true;
			i++;
			continue;
		}
		if (isdigit((unsigned char)c) || c == '.') {
			// a number literal: digits with at most one decimal point
			// ("2", ".5", "2.", and "2.5" are all valid)
			double intPart = 0;
			double fracPart = 0;
			double fracScale = 1;
			bool sawDot = false;
			bool sawDigit = false;
			while (i < n && (isdigit((unsigned char)equation[i]) || (equation[i] == '.' && !sawDot))) {
				if (equation[i] == '.') {
					sawDot = true;
					i++;
					continue;
				}
				int digit = equation[i] - '0';
				if (sawDot) {
					fracScale *= 10;
					fracPart += digit / fracScale;
				} else {
					intPart = intPart * 10 + digit;
				}
				sawDigit = true;
				i++;
			}
			if (!sawDigit) return false; // a lone '.' with no digits at all
			double value = intPart + fracPart;
			if (negatePending) {
				value = -value;
				negatePending = false;
			}
			if (!isfinite(value)) return false; // literal too large to represent
			output.push({true, value, 0});
			expectOperand = false;
			continue;
		}
		if (c == '+' || c == '-' || c == '*' || c == '/') {
			if (expectOperand) return false; // operator with no preceding operand
			// pop operators of higher-or-equal precedence (left-associative)
			while (!operators.empty() && precedence(operators.back()) >= precedence(c)) {
				output.push({false, 0, operators.back()});
				operators.pop_back();
			}
			operators.push_back(c);
			expectOperand = true;
			i++;
			continue;
		}
		return false; // unsupported character
	}
	// a dangling sign ("5*-") also leaves expectOperand set, since the literal
	// it was waiting for never arrived
	if (expectOperand) return false; // empty input or trailing operator
	while (!operators.empty()) {
		output.push({false, 0, operators.back()});
		operators.pop_back();
	}

	// --- evaluate the RPN output queue ---
	vector<double> values;
	while (!output.empty()) {
		Token t = output.front();
		output.pop();
		if (t.isNumber) {
			values.push_back(t.value);
		} else {
			if (values.size() < 2) return false; // malformed expression
			double b = values.back(); values.pop_back();
			double a = values.back(); values.pop_back();
			double r;
			if (!applyOperator(a, b, t.op, r)) return false;
			values.push_back(r);
		}
	}
	if (values.size() != 1) return false;
	result = values.back();
	return true;
}

// ---------------------------------------------------------------------------
// CalculatorEngine
// ---------------------------------------------------------------------------

CalculatorEngine::CalculatorEngine() {
	clear();
}

// Redraws the slots as a window onto the last SLOT_COUNT characters of the
// equation, left-aligned and padded with empty slots while it is shorter. Once
// the equation outgrows the display the window scrolls rather than overwriting
// the final slot, so what is on screen is always a truthful suffix of what will
// be evaluated (the earlier behavior dropped characters in the middle: "1"-"9"
// showed "1234569", and continuing from a 7-character result such as "0.33333"
// showed "0.3333+" for the equation "0.33333+").
void CalculatorEngine::refreshSlots() {
	int length = (int)equation.size();
	int start = (length > SLOT_COUNT) ? (length - SLOT_COUNT) : 0;
	for (int i = 0; i < SLOT_COUNT; i++) {
		slots[i] = (start + i < length) ? equation[start + i] : '\0';
	}
}

// Appends a character to the equation and redraws the display around it.
void CalculatorEngine::appendChar(char value) {
	equation += value;
	refreshSlots();
}

// Replaces the equation/display with the given text (used to show a result).
void CalculatorEngine::setDisplay(const string& text) {
	equation = text;
	refreshSlots();
}

void CalculatorEngine::inputDigit(int digit) {
	if (digit < 0 || digit > 9) return;
	// a digit typed right after a result starts a brand-new calculation
	if (justEvaluated) {
		clear();
	}
	appendChar((char)('0' + digit));
}

void CalculatorEngine::inputOperator(char op) {
	if (op == '+' || op == '-' || op == '*' || op == '/') {
		// an operator right after a result continues from that result
		justEvaluated = false;
		appendChar(op);
	}
}

void CalculatorEngine::inputDecimalPoint() {
	// a decimal point typed right after a result starts a brand-new calculation
	if (justEvaluated) {
		clear();
	}
	// the number currently being typed is whatever follows the last operator
	// (or the whole equation, if none); reject a second '.' within it
	size_t lastOperator = equation.find_last_of("+-*/");
	string currentNumber = (lastOperator == string::npos) ? equation : equation.substr(lastOperator + 1);
	if (currentNumber.find('.') != string::npos) return;
	appendChar('.');
}

void CalculatorEngine::clear() {
	equation.clear();
	refreshSlots();
	justEvaluated = false;
}

bool CalculatorEngine::evaluate(double& result) {
	double value;
	if (!parseEquation(equation, value)) {
		return false;
	}
	// A result that does not fit in the fixed-width display cannot be shown
	// truthfully, so it is rejected the same way a malformed equation is
	// (equation/display left untouched) rather than being silently truncated.
	string text;
	if (!formatForDisplay(value, SLOT_COUNT, text)) {
		return false;
	}
	setDisplay(text);
	justEvaluated = true;
	result = value;
	return true;
}

const string& CalculatorEngine::equationText() const {
	return equation;
}

int CalculatorEngine::slotCount() const {
	return SLOT_COUNT;
}

char CalculatorEngine::slotChar(int index) const {
	if (index < 0 || index >= SLOT_COUNT) return '\0';
	return slots[index];
}
