#include "CalculatorEngine.h"

#include <queue>
#include <vector>
#include <cctype>

using namespace std;

// ---------------------------------------------------------------------------
// Shunting-yard parser (https://en.wikipedia.org/wiki/Shunting-yard_algorithm)
// ---------------------------------------------------------------------------

namespace {
	// A single parsed token: either a number or a binary operator.
	struct Token {
		bool isNumber;
		int value; // valid when isNumber is true
		char op;   // valid when isNumber is false
	};

	// Operator precedence (higher binds tighter); 0 for non-operators.
	int precedence(char op) {
		if (op == '+' || op == '-') return 1;
		if (op == '*') return 2;
		return 0;
	}

	// Applies a binary operator to two operands, writing the result to out;
	// returns false for an unsupported operator.
	bool applyOperator(int a, int b, char op, int& out) {
		switch (op) {
			case '+': out = a + b; return true;
			case '-': out = a - b; return true;
			case '*': out = a * b; return true;
		}
		return false;
	}
}

bool parseEquation(const string& equation, int& result) {
	// --- shunting-yard: build the RPN output queue ---
	queue<Token> output;
	vector<char> operators; // operator stack
	int i = 0;
	int n = (int)equation.length();
	bool expectOperand = true; // operand and operator must alternate
	while (i < n) {
		char c = equation[i];
		if (isspace((unsigned char)c)) { i++; continue; }
		if (isdigit((unsigned char)c)) {
			int num = 0;
			while (i < n && isdigit((unsigned char)equation[i])) {
				num = num * 10 + (equation[i] - '0');
				i++;
			}
			output.push({true, num, 0});
			expectOperand = false;
			continue;
		}
		if (c == '+' || c == '-' || c == '*') {
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
	if (expectOperand) return false; // empty input or trailing operator
	while (!operators.empty()) {
		output.push({false, 0, operators.back()});
		operators.pop_back();
	}

	// --- evaluate the RPN output queue ---
	vector<int> values;
	while (!output.empty()) {
		Token t = output.front();
		output.pop();
		if (t.isNumber) {
			values.push_back(t.value);
		} else {
			if (values.size() < 2) return false; // malformed expression
			int b = values.back(); values.pop_back();
			int a = values.back(); values.pop_back();
			int r;
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

// Appends a character to the equation and shows it in the next display slot.
// The slot cursor saturates at the last slot (matching the original GUI, which
// kept overwriting the final slot once the display was full).
void CalculatorEngine::appendChar(char value) {
	equation += value;
	slots[nextSlot] = value;
	if (nextSlot < SLOT_COUNT - 1) {
		nextSlot++;
	}
}

// Replaces the equation/display with the given text (used to show a result).
void CalculatorEngine::setDisplay(const string& text) {
	equation = text;
	for (int i = 0; i < SLOT_COUNT; i++) {
		slots[i] = (i < (int)text.size()) ? text[i] : '\0';
	}
	nextSlot = (int)text.size();
	if (nextSlot > SLOT_COUNT - 1) {
		nextSlot = SLOT_COUNT - 1;
	}
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
	if (op == '+' || op == '-' || op == '*') {
		// an operator right after a result continues from that result
		justEvaluated = false;
		appendChar(op);
	}
}

void CalculatorEngine::clear() {
	equation.clear();
	for (int i = 0; i < SLOT_COUNT; i++) {
		slots[i] = '\0';
	}
	nextSlot = 0;
	justEvaluated = false;
}

bool CalculatorEngine::evaluate(int& result) {
	if (!parseEquation(equation, result)) {
		return false;
	}
	setDisplay(to_string(result));
	justEvaluated = true;
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
