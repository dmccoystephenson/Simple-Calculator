// CalculatorEngine — the UI-agnostic calculator core shared by every frontend.
//
// It owns all interaction state (the equation being built and the fixed-size
// display) and the evaluation logic; it knows nothing about SDL, a terminal,
// or any other presentation layer. A frontend translates its own input events
// into the input methods below and renders whatever the display queries
// expose. This is the seam that lets the SDL GUI, the text/CLI frontend, and
// any future port (e.g. a pygame binding or a WebAssembly build) all drive the
// same engine.
#ifndef CALCULATOR_ENGINE_H
#define CALCULATOR_ENGINE_H

#include <string>

// Evaluates a floating-point arithmetic expression using the shunting-yard
// algorithm (supports +, -, *, / with standard precedence and
// left-associativity). A number literal may include at most one decimal
// point (".5", "2.", and "2.5" are all valid; "2.3.4" is not). Returns true
// and writes the value to result on success; returns false on malformed/empty
// input, an unsupported character, division by zero, or a literal/
// intermediate/final value that is not finite (e.g. overflows to infinity).
bool parseEquation(const std::string& equation, double& result);

class CalculatorEngine {
  public:
	// Number of display slots a frontend can render.
	static const int SLOT_COUNT = 7;

	CalculatorEngine();

	// Input — a frontend maps its own buttons/keys onto these.
	void inputDigit(int digit);     // digit in [0, 9]; out-of-range is ignored
	void inputOperator(char op);    // op in {'+', '-', '*', '/'}; others are ignored
	void inputDecimalPoint();       // appends '.'; ignored if the number being typed
	                                 // (the digits since the last operator) already has one
	void clear();                   // reset to the empty state
	bool evaluate(double& result);  // evaluate; fails (display untouched) if the equation is
	                                 // malformed or the result doesn't fit in SLOT_COUNT chars

	// Presentation queries — a frontend reads these to render the display.
	const std::string& equationText() const; // the full equation built so far
	int slotCount() const;                    // == SLOT_COUNT
	char slotChar(int index) const;           // char shown in a slot, or '\0' if empty

  private:
	void appendChar(char value);
	void setDisplay(const std::string& text);

	std::string equation;
	char slots[SLOT_COUNT];
	int nextSlot;
	bool justEvaluated; // true right after evaluate(): the next digit starts fresh
};

#endif
