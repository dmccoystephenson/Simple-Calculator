// Assert-based test suite for the text/CLI frontend's input dispatch
// (textCalculator.cpp). It is the third companion to testingParsing.cpp (the
// shared engine) and testingFrontend.cpp (the SDL frontend): this file covers
// the one translation layer neither of those reaches — which typed character
// maps to which engine call, what the frontend prints on a successful and a
// failed '=', and how the display slots are rendered as text.
//
// Before this suite existed the only automated check that reached the text
// frontend was CI's smoke test, which pipes two additions through it and greps
// the totals. That leaves every other branch of the dispatch chain unpinned,
// including the ones README.md states outright ('c' clears, 'q' quits,
// "(invalid equation)" on failure, "any other character is ignored").
//
// textCalculator.cpp has no header of its own — it is a single translation unit
// whose entry point is the thing under test — so, exactly as testingFrontend.cpp
// does with simpleCalculator.cpp, this suite includes that translation unit
// directly and renames its main() out of the way. Driving the real main() rather
// than re-implementing its loop is what makes these checks cover the dispatch as
// shipped: the frontend reads std::cin and writes std::cout, so a session is run
// by swapping string buffers into both streams and asserting on what comes back.
//
// Unlike testingFrontend, this suite needs neither SDL nor the PNG assets, so it
// can be built and run anywhere the engine can.
//
// These checks characterize the frontend's *current* behavior. Where that
// behavior is a known limitation rather than a desired one — the absence of any
// single-character delete — the check pins what happens today and names the
// issue tracking the change.

// Every check here is a bare assert(), and the calls under test sit inside those
// asserts. <cassert> compiles assert() to nothing when NDEBUG is defined, which
// would strip the checks *and* the calls they exercise, leaving a binary that
// reports success without having tested anything. Refuse to build rather than
// pass vacuously (testingParsing.cpp and testingFrontend.cpp guard themselves
// the same way).
#ifdef NDEBUG
#error "testingTextFrontend.cpp relies on assert(); building with NDEBUG would make every check a no-op"
#endif

// Pulled in before the rename below so that the frontend's own #includes are
// already satisfied by the time it is included, and no system header is ever
// seen while `main` is a macro.
#include <cassert>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Rename the frontend's entry point so this file can supply its own, then pull
// in the translation unit under test.
#define main textCalculatorMain
#include "textCalculator.cpp"
#undef main

using namespace std;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Runs one complete frontend session against the given stdin text and returns
// everything it printed. cin's buffer is swapped rather than the stream object
// because the frontend names std::cin directly; cin.clear() is required on the
// way in because a previous session ended by reading to EOF, which leaves
// eofbit/failbit set on the stream itself (the state lives on cin, not on the
// buffer being swapped, so it survives the swap and would make the very first
// getline of the next session fail).
static string runSession(const string& input) {
	istringstream in(input);
	ostringstream out;
	streambuf* previousIn = cin.rdbuf(in.rdbuf());
	streambuf* previousOut = cout.rdbuf(out.rdbuf());
	cin.clear();

	int status = textCalculatorMain();

	cin.rdbuf(previousIn);
	cout.rdbuf(previousOut);
	cin.clear();

	// the frontend has exactly one exit status: it returns 0 both when 'q' is
	// typed and when stdin runs out
	assert(status == 0);
	return out.str();
}

static bool contains(const string& haystack, const string& needle) {
	return haystack.find(needle) != string::npos;
}

// Every display snapshot the session printed, in order, with the surrounding
// "  [" / "]" stripped. printDisplay() runs once at startup and then once per
// line of input consumed, so the first entry is always the empty display and
// the count doubles as "how many input lines were actually processed, plus one".
static vector<string> displays(const string& output) {
	vector<string> found;
	const string prefix = "  [";
	size_t at = 0;
	while ((at = output.find(prefix, at)) != string::npos) {
		size_t start = at + prefix.size();
		size_t end = output.find(']', start);
		assert(end != string::npos); // a display line is always closed
		found.push_back(output.substr(start, end - start));
		at = end;
	}
	return found;
}

// The display snapshot printed at the end of the index'th input line (index 0
// being the startup one), asserting first that the session got that far.
//
// The assert is the point: indexing the vector directly throws std::out_of_range
// on a wrong expectation, and that exception names neither a file, a line, nor a
// test — it just aborts, leaving the reader to find which of the checks below
// was responsible. An assert names the expression and its location the way every
// other check in the three suites does. The failure is easy to reach by accident
// because a 'q' anywhere in a line ends the session immediately, so a test string
// with a stray 'q' before its newline produces one fewer snapshot than it reads.
static string displayAt(const string& output, size_t index) {
	vector<string> found = displays(output);
	assert(index < found.size()); // the session ended before this line was rendered
	return found[index];
}

// ---------------------------------------------------------------------------
// Checks
// ---------------------------------------------------------------------------

// The banner is the frontend's only usage text, so it is the only place a user
// learns the key bindings; it is asserted here so that dropping a binding from
// the dispatch and from the banner together still fails a test.
static void testStartupBanner() {
	string out = runSession("q\n");
	assert(contains(out, "Simple Calculator (text mode)"));
	assert(contains(out, "Type digits, '.', and + - * / to build an equation"));
	assert(contains(out, "'='  evaluate"));
	assert(contains(out, "'c'  clear"));
	assert(contains(out, "'q'  quit"));

	// startup prints the empty display before reading anything, and an empty
	// slot renders as '_' — this is the text frontend's whole rendering model
	vector<string> shown = displays(out);
	assert(shown.size() == 1); // the 'q' quit before any line-end display
	assert(shown[0] == "_______");
	assert((int)shown[0].size() == CalculatorEngine::SLOT_COUNT);
}

// Digits, operators, and the decimal point each reach their own engine input
// method; a character bound to none of them is dropped silently.
static void testCharacterDispatch() {
	// digits accumulate left to right and pad the remaining slots
	assert(displayAt(runSession("123\nq\n"), 1) == "123____");

	// each operator is dispatched, not just '+'
	assert(displayAt(runSession("1+2-3*4/5\nq\n"), 1) == "2-3*4/5"); // window scrolled

	// '.' reaches inputDecimalPoint rather than falling into the ignored bucket
	assert(displayAt(runSession("5.5\nq\n"), 1) == "5.5____");

	// spaces, tabs, and letters that bind to nothing are ignored, and — this is
	// the part worth pinning — they are ignored *silently*: the session prints
	// byte-for-byte what it would have printed had they never been typed, so
	// they leave no trace in the display and produce no diagnostic of their own
	string withIgnored = runSession("1 2\tx3\nq\n");
	string withoutIgnored = runSession("123\nq\n");
	assert(withIgnored == withoutIgnored);
	assert(displayAt(withIgnored, 1) == "123____");
}

// A successful '=' prints the result and leaves it on the display.
static void testEvaluateSuccess() {
	string out = runSession("12+3=\nq\n");
	assert(contains(out, "= 15\n"));
	assert(displayAt(out, 1) == "15_____");

	// true (floating-point) division, matching what README.md promises
	assert(contains(runSession("7/2=\nq\n"), "= 3.5\n"));

	// the decimal point actually participates in the arithmetic it was
	// dispatched into: 5.5*2 is 11, and the whole-number result sheds the
	// fractional part entirely
	assert(contains(runSession("5.5*2=\nq\n"), "= 11\n"));
}

// The frontend prints engine.equationText() — the formatted *display* text —
// rather than formatting the double out-param itself. That is what makes the
// README's "the GUI and text frontend ... always agree on what a result looks
// like" true, and it is invisible to CI's smoke test because 15 and 10 format
// identically either way.
static void testPrintsDisplayTextNotRawDouble() {
	// 1/3 fills the display's five available fractional digits as "0.33333";
	// streaming the double instead would print ostream's six significant
	// digits, "0.333333". The trailing newline in the needle is what
	// distinguishes them, since "0.33333" is a prefix of "0.333333".
	string out = runSession("1/3=\nq\n");
	assert(contains(out, "= 0.33333\n"));
	assert(!contains(out, "0.333333"));
	assert(displayAt(out, 1) == "0.33333");

	// negative zero is the sharper case: the engine deliberately displays it as
	// a plain "0", while streaming the double would print "-0"
	string negativeZero = runSession("0*-3=\nq\n");
	assert(contains(negativeZero, "= 0\n"));
	assert(!contains(negativeZero, "-0"));
	assert(displayAt(negativeZero, 1) == "0______");
}

// Every way evaluate() can fail surfaces as the same one-line diagnostic, and
// none of them disturbs the equation being built.
static void testEvaluateFailurePrintsDiagnostic() {
	// a trailing operator
	string trailing = runSession("2+=\nq\n");
	assert(contains(trailing, "(invalid equation)\n"));
	assert(!contains(trailing, "= ")); // no result line was printed
	assert(displayAt(trailing, 1) == "2+_____"); // equation left untouched

	// division by zero is reported the same way as a malformed equation, which
	// is the behavior README.md documents rather than a separate error
	string byZero = runSession("6/0=\nq\n");
	assert(contains(byZero, "(invalid equation)\n"));
	assert(displayAt(byZero, 1) == "6/0____");

	// a result too wide for the seven slots is rejected rather than truncated,
	// so it reaches the user through this same message; the display keeps
	// showing the window onto the untouched equation
	string tooWide = runSession("50000000+49999999=\nq\n");
	assert(contains(tooWide, "(invalid equation)\n"));
	assert(displayAt(tooWide, 1) == "9999999");

	// a doubled sign has no operand position left to be a sign in
	assert(contains(runSession("5---3=\nq\n"), "(invalid equation)\n"));
}

// 'c' resets the equation; the uppercase form is bound too, and neither prints
// anything of its own.
static void testClear() {
	string out = runSession("12+3c\nq\n");
	assert(displayAt(out, 1) == "_______");
	assert(!contains(out, "(invalid equation)"));

	assert(displayAt(runSession("12+3C\nq\n"), 1) == "_______");

	// clearing mid-line and then typing again builds a fresh equation, so the
	// clear takes effect immediately rather than at the end of the line
	assert(displayAt(runSession("99c7\nq\n"), 1) == "7______");
}

// 'q' returns from main() the moment it is read, so it ends the session from
// the middle of a line — the rest of that line and every later line go
// unprocessed, and the end-of-line display is never printed.
static void testQuit() {
	string out = runSession("5q9\n7\nq\n");
	vector<string> shown = displays(out);
	assert(shown.size() == 1);       // only the startup display
	assert(shown[0] == "_______");   // the '5' never reached an end-of-line render

	// the uppercase form quits too
	assert(displays(runSession("Q\n")).size() == 1);

	// quitting from a later line keeps everything the earlier lines built
	vector<string> laterQuit = displays(runSession("12\nq\n"));
	assert(laterQuit.size() == 2);
	assert(laterQuit.at(1) == "12_____");
}

// Running out of input ends the session just as 'q' does, which is what lets
// the frontend be driven from a pipe or a here-doc the way CI's smoke test does.
static void testEndOfInputTerminates() {
	vector<string> shown = displays(runSession("5\n"));
	assert(shown.size() == 2); // startup, then the one line consumed
	assert(shown.at(1) == "5______");

	// a final line with no trailing newline is still consumed by getline
	assert(displayAt(runSession("5+5="), 1) == "10_____");

	// empty input is a session that reads nothing and renders once
	assert(displays(runSession("")).size() == 1);
}

// Engine state belongs to the session, not to the line, so an equation can be
// built across several lines and evaluated on a later one.
static void testStateSpansLines() {
	string out = runSession("12\n+3\n=\nq\n");
	vector<string> shown = displays(out);
	assert(shown.at(1) == "12_____");
	assert(shown.at(2) == "12+3___");
	assert(shown.at(3) == "15_____");
	assert(contains(out, "= 15\n"));

	// and each session starts from a fresh engine: the "15" above is gone
	assert(displayAt(runSession("+3=\nq\n"), 1) == "+3_____");
}

// The display is a window onto the *end* of the equation, so a long equation
// scrolls rather than dropping characters in the middle. The engine owns this
// rule; the check here is that the text frontend renders the window it is given
// instead of its own truncation.
static void testDisplayScrolls() {
	assert(displayAt(runSession("1234567\nq\n"), 1) == "1234567"); // exactly full
	assert(displayAt(runSession("12345678\nq\n"), 1) == "2345678"); // scrolled by one
	assert(displayAt(runSession("1234567890\nq\n"), 1) == "4567890");
}

// Known limitation, pinned so that fixing it has to update this suite: there is
// no way to delete a single character, so a mistyped digit costs the whole
// equation. Tracked by issue #48.
static void testNoSingleCharacterDelete() {
	// backspace is not a bound character; it is ignored like any other, so it
	// leaves the digit it was meant to remove on the display
	assert(displayAt(runSession("123\b\nq\n"), 1) == "123____");
	// the only erase available is the whole-equation clear
	assert(displayAt(runSession("123c\nq\n"), 1) == "_______");
}

int main() {
	testStartupBanner();
	testCharacterDispatch();
	testEvaluateSuccess();
	testPrintsDisplayTextNotRawDouble();
	testEvaluateFailurePrintsDiagnostic();
	testClear();
	testQuit();
	testEndOfInputTerminates();
	testStateSpansLines();
	testDisplayScrolls();
	testNoSingleCharacterDelete();

	cout << "All text frontend tests passed." << endl;
	return 0;
}
