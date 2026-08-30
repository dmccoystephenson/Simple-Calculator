// Assert-based test suite for the SDL frontend's event-translation layer
// (simpleCalculator.cpp). It is the companion to testingParsing.cpp, which
// covers the shared engine: this file covers the part the engine deliberately
// knows nothing about — which SDL event maps to which button id, which button
// id maps to which engine call, where a click counts as landing on a button,
// and which texture ends up in each display slot.
//
// It runs headlessly. SDL's "dummy" video driver and software renderer are
// selected in main() before init(), so a real window, GPU, or display server is
// never needed and the suite runs in CI next to the engine tests.
//
// simpleCalculator.cpp has no header of its own — it is a single translation
// unit whose globals and functions are all non-static — so the suite includes
// that translation unit directly and renames its main() out of the way. The
// alternative, re-declaring every symbol under test as extern, would duplicate
// declarations that would then drift from the definitions; including the unit
// keeps simpleCalculator.cpp the one place each is written down.
//
// These checks characterize the frontend's *current* behavior. Where that
// behavior is a known limitation rather than a desired one (Backspace being
// unbound, a failed '=' being silent), the check pins what happens today and
// names the issue tracking the change.

// Every check here is a bare assert(), and the calls under test sit inside
// those asserts. <cassert> compiles assert() to nothing when NDEBUG is defined,
// which would strip the checks *and* the calls they exercise, leaving a binary
// that reports success without having tested anything. Refuse to build rather
// than pass vacuously (testingParsing.cpp guards itself the same way).
#ifdef NDEBUG
#error "testingFrontend.cpp relies on assert(); building with NDEBUG would make every check a no-op"
#endif

#include <SDL.h>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

// Rename the frontend's entry point so this file can supply its own, then pull
// in the translation unit under test.
#define main sdlCalculatorMain
#include "simpleCalculator.cpp"
#undef main

using namespace std;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Builds the SDL_TEXTINPUT event SDL would deliver for a typed string. The
// payload is UTF-8 and NUL-terminated, exactly as SDL fills it.
static SDL_Event textEvent(const char* text) {
	SDL_Event e;
	SDL_zero(e);
	e.type = SDL_TEXTINPUT;
	size_t length = strlen(text);
	assert(length < sizeof(e.text.text)); // a test string this long is a typo, not a case
	memcpy(e.text.text, text, length + 1);
	return e;
}

// Builds the SDL_KEYDOWN event for a key that produces no text of its own.
static SDL_Event keyEvent(SDL_Keycode key) {
	SDL_Event e;
	SDL_zero(e);
	e.type = SDL_KEYDOWN;
	e.key.keysym.sym = key;
	return e;
}

// Builds a mouse-button event at a pixel position.
static SDL_Event mouseEvent(Uint32 type, Uint8 button, int x, int y) {
	SDL_Event e;
	SDL_zero(e);
	e.type = type;
	e.button.button = button;
	e.button.x = x;
	e.button.y = y;
	return e;
}

// A left-press at the centre of a button, the ordinary way one is activated.
static SDL_Event clickCentre(const Button& button) {
	return mouseEvent(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT,
	                  button.xpos + button.width / 2, button.ypos + button.height / 2);
}

// Reads the display slots back out as a string by mapping each slot's texture
// to the character it draws ('_' for an empty slot, '?' for a texture no slot
// should ever hold). This is the render path in reverse: it states what the GUI
// would actually put on screen, rather than what the engine holds internally.
static string renderedDisplay() {
	string out;
	for (int i = 0; i < CalculatorEngine::SLOT_COUNT; i++) {
		SDL_Texture* texture = displaySlots[i].currentTexture;
		char c = '?';
		if (texture == emptyT) c = '_';
		else if (texture == plusSignT) c = '+';
		else if (texture == minusSignT) c = '-';
		else if (texture == multiplyT) c = '*';
		else if (texture == divideT) c = '/';
		else if (texture == decimalT) c = '.';
		else {
			for (int digit = 0; digit < 10; digit++) {
				if (texture == digitTextures[digit]) c = (char)('0' + digit);
			}
		}
		out += c;
	}
	return out;
}

// Types a string of characters through the keyboard path, the way a user would.
static void type(const char* characters) {
	for (const char* c = characters; *c != '\0'; c++) {
		char single[2] = {*c, '\0'};
		handleKeyboardEvents(textEvent(single));
	}
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

// init() is the one place a button id is chosen; every other mapping is read
// off the buttons themselves, so pinning the ids here is what makes the rest of
// the suite meaningful rather than circular.
static void testButtonIdsAssignedByInit() {
	for (int digit = 0; digit < 10; digit++) {
		assert(digitButtons[digit].id == digit);
	}
	assert(minusSign.id == 10);
	assert(plusSign.id == 11);
	assert(multiply.id == 12);
	assert(divide.id == 22);
	assert(decimalPoint.id == 23);
	assert(clear.id == 20);
	assert(equalsSign.id == 21);
	for (int i = 0; i < CalculatorEngine::SLOT_COUNT; i++) {
		assert(displaySlots[i].id == 13 + i);
	}

	// every button that does something is clickable; the display slots are not,
	// which is what makes them inert rather than relying on changeDisplay() to
	// ignore their ids
	assert(clickableButtons.size() == 17); // 10 digits + 5 operators + clear + equals
	assert(allButtons.size() == clickableButtons.size() + CalculatorEngine::SLOT_COUNT);
	for (int i = 0; i < CalculatorEngine::SLOT_COUNT; i++) {
		for (Button* button : clickableButtons) {
			assert(button != &displaySlots[i]);
		}
	}

	// no two buttons share an id, so a dispatched id is never ambiguous
	for (size_t a = 0; a < allButtons.size(); a++) {
		for (size_t b = a + 1; b < allButtons.size(); b++) {
			assert(allButtons[a]->id != allButtons[b]->id);
		}
	}
}

// loadMedia() reads every button glyph from a PNG beside the executable. A
// missing or renamed asset is a runtime-only failure the compiler cannot catch,
// so loading them all is itself the check.
static void testAllAssetsLoad() {
	assert(emptyT != NULL);
	for (int digit = 0; digit < 10; digit++) {
		assert(digitTextures[digit] != NULL);
	}
	assert(minusSignT != NULL);
	assert(plusSignT != NULL);
	assert(multiplyT != NULL);
	assert(divideT != NULL);
	assert(clearT != NULL);
	assert(equalsT != NULL);
	assert(decimalT != NULL);

	// the non-slot buttons are given their fixed texture once, by loadMedia()
	for (int digit = 0; digit < 10; digit++) {
		assert(digitButtons[digit].currentTexture == digitTextures[digit]);
	}
	assert(plusSign.currentTexture == plusSignT);
	assert(minusSign.currentTexture == minusSignT);
	assert(multiply.currentTexture == multiplyT);
	assert(divide.currentTexture == divideT);
	assert(decimalPoint.currentTexture == decimalT);
	assert(clear.currentTexture == clearT);
	assert(equalsSign.currentTexture == equalsT);
}

// The keyboard reaches the engine by first resolving a character to a button
// id, so a key press and a click on the same button run the identical dispatch.
static void testButtonIdForChar() {
	for (char digit = '0'; digit <= '9'; digit++) {
		assert(buttonIdForChar(digit) == digitButtons[digit - '0'].id);
	}
	assert(buttonIdForChar('-') == minusSign.id);
	assert(buttonIdForChar('+') == plusSign.id);
	assert(buttonIdForChar('*') == multiply.id);
	assert(buttonIdForChar('/') == divide.id);
	assert(buttonIdForChar('.') == decimalPoint.id);
	assert(buttonIdForChar('=') == equalsSign.id);
	assert(buttonIdForChar('c') == clear.id);
	assert(buttonIdForChar('C') == clear.id); // the shifted key clears too

	// characters no button is bound to
	assert(buttonIdForChar('a') == -1);
	assert(buttonIdForChar('Q') == -1);
	assert(buttonIdForChar(' ') == -1);
	assert(buttonIdForChar('%') == -1); // modulo is not an operation the engine has
	assert(buttonIdForChar('\n') == -1); // Enter arrives as a key, not as text
	assert(buttonIdForChar('\0') == -1);
}

// changeDisplay() is the single dispatch both input paths funnel into.
static void testChangeDisplayDispatch() {
	changeDisplay(clear.id);
	assert(engine.equationText() == "");

	for (int digit = 0; digit <= 9; digit++) {
		changeDisplay(clear.id);
		changeDisplay(digit);
		assert(engine.equationText() == string(1, (char)('0' + digit)));
	}

	changeDisplay(clear.id);
	changeDisplay(7);
	changeDisplay(minusSign.id);
	changeDisplay(3);
	assert(engine.equationText() == "7-3");
	changeDisplay(equalsSign.id);
	assert(engine.equationText() == "4");

	changeDisplay(clear.id);
	changeDisplay(6);
	changeDisplay(plusSign.id);
	changeDisplay(2);
	changeDisplay(equalsSign.id);
	assert(engine.equationText() == "8");

	changeDisplay(clear.id);
	changeDisplay(6);
	changeDisplay(multiply.id);
	changeDisplay(2);
	changeDisplay(equalsSign.id);
	assert(engine.equationText() == "12");

	changeDisplay(clear.id);
	changeDisplay(6);
	changeDisplay(divide.id);
	changeDisplay(2);
	changeDisplay(equalsSign.id);
	assert(engine.equationText() == "3");

	changeDisplay(clear.id);
	changeDisplay(2);
	changeDisplay(decimalPoint.id);
	changeDisplay(5);
	assert(engine.equationText() == "2.5");

	// the display-slot ids fall through every branch, so even a dispatch of one
	// leaves the equation alone
	for (int i = 0; i < CalculatorEngine::SLOT_COUNT; i++) {
		changeDisplay(displaySlots[i].id);
		assert(engine.equationText() == "2.5");
	}

	// so does an id no button was ever given
	changeDisplay(99);
	changeDisplay(-1);
	assert(engine.equationText() == "2.5");
}

// Typed characters arrive as SDL_TEXTINPUT, which reports what the layout and
// modifiers actually produced rather than which physical key moved.
static void testTextInputEvents() {
	changeDisplay(clear.id);
	type("12+3");
	assert(engine.equationText() == "12+3");
	type("=");
	assert(engine.equationText() == "15");

	// the shifted operators are the reason text input is used at all: on a US
	// layout SDL_KEYDOWN would report '+' as '=' and '*' as '8'
	changeDisplay(clear.id);
	type("2*3=");
	assert(engine.equationText() == "6");

	changeDisplay(clear.id);
	type("7/2=");
	assert(engine.equationText() == "3.5");

	changeDisplay(clear.id);
	type("1.5+1=");
	assert(engine.equationText() == "2.5");

	// 'c' and 'C' both clear, the same as the on-screen button
	changeDisplay(clear.id);
	type("42c");
	assert(engine.equationText() == "");
	type("42C");
	assert(engine.equationText() == "");

	// an unbound character is dropped rather than reaching the engine
	changeDisplay(clear.id);
	type("4a2 %z");
	assert(engine.equationText() == "42");

	// a multi-byte character is ignored: every character a button is bound to is
	// a single ASCII byte, so anything wider cannot be one of them
	changeDisplay(clear.id);
	handleKeyboardEvents(textEvent("\xc3\xa9")); // U+00E9, two UTF-8 bytes
	handleKeyboardEvents(textEvent("12"));       // two bound characters at once
	assert(engine.equationText() == "");

	// an empty payload is ignored rather than read past its terminator
	handleKeyboardEvents(textEvent(""));
	assert(engine.equationText() == "");
}

// The keys that produce no character of their own are handled as SDL_KEYDOWN.
static void testKeyDownEvents() {
	// all three Enter keycodes evaluate
	const SDL_Keycode enterKeys[] = {SDLK_RETURN, SDLK_RETURN2, SDLK_KP_ENTER};
	for (SDL_Keycode key : enterKeys) {
		changeDisplay(clear.id);
		type("9+1");
		handleKeyboardEvents(keyEvent(key));
		assert(engine.equationText() == "10");
	}

	// Escape and Delete clear, matching the on-screen clear button
	const SDL_Keycode clearKeys[] = {SDLK_ESCAPE, SDLK_DELETE};
	for (SDL_Keycode key : clearKeys) {
		changeDisplay(clear.id);
		type("123");
		handleKeyboardEvents(keyEvent(key));
		assert(engine.equationText() == "");
	}

	// a key that produces text of its own is ignored by the SDL_KEYDOWN branch,
	// so pressing '5' enters one digit rather than two: SDL delivers both a
	// keydown and a text-input event for it, and only the latter is bound
	changeDisplay(clear.id);
	handleKeyboardEvents(keyEvent(SDLK_5));
	assert(engine.equationText() == "");
	handleKeyboardEvents(textEvent("5"));
	assert(engine.equationText() == "5");

	// Backspace is unbound, so a mistyped character still costs the whole
	// equation (issue #48 tracks adding a single-character delete)
	changeDisplay(clear.id);
	type("129");
	handleKeyboardEvents(keyEvent(SDLK_BACKSPACE));
	assert(engine.equationText() == "129");

	// so is every other key with no character of its own
	const SDL_Keycode unboundKeys[] = {SDLK_TAB, SDLK_UP, SDLK_F1, SDLK_LSHIFT, SDLK_HOME};
	for (SDL_Keycode key : unboundKeys) {
		handleKeyboardEvents(keyEvent(key));
		assert(engine.equationText() == "129");
	}
}

// A click is dispatched by hit-testing every clickable button against the
// position the event carries.
static void testClickDispatch() {
	changeDisplay(clear.id);
	handleButtonEvents(clickCentre(digitButtons[4]));
	handleButtonEvents(clickCentre(plusSign));
	handleButtonEvents(clickCentre(digitButtons[5]));
	assert(engine.equationText() == "4+5");
	handleButtonEvents(clickCentre(equalsSign));
	assert(engine.equationText() == "9");
	handleButtonEvents(clickCentre(clear));
	assert(engine.equationText() == "");

	// the hit box matches the rectangle displayTexture() draws: the near edges
	// are inclusive and the far edges exclusive, so the clickable area is
	// exactly the pixels the button covers
	const Button& b = digitButtons[8];
	changeDisplay(clear.id);
	handleButtonEvents(mouseEvent(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, b.xpos, b.ypos));
	assert(engine.equationText() == "8"); // top-left corner is inside

	changeDisplay(clear.id);
	handleButtonEvents(mouseEvent(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT,
	                              b.xpos + b.width - 1, b.ypos + b.height - 1));
	assert(engine.equationText() == "8"); // last drawn pixel is inside

	changeDisplay(clear.id);
	handleButtonEvents(mouseEvent(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, b.xpos - 1, b.ypos));
	handleButtonEvents(mouseEvent(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, b.xpos, b.ypos - 1));
	handleButtonEvents(mouseEvent(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, b.xpos + b.width, b.ypos));
	handleButtonEvents(mouseEvent(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, b.xpos, b.ypos + b.height));
	assert(engine.equationText() == ""); // one pixel past any edge is outside

	// only a left press activates a button: a right-click is a context-menu
	// gesture and a middle-click a paste/scroll one
	handleButtonEvents(mouseEvent(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_RIGHT,
	                              b.xpos + 5, b.ypos + 5));
	handleButtonEvents(mouseEvent(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_MIDDLE,
	                              b.xpos + 5, b.ypos + 5));
	assert(engine.equationText() == "");

	// and only the press, not the release, so a click enters one character
	handleButtonEvents(mouseEvent(SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT,
	                              b.xpos + 5, b.ypos + 5));
	handleButtonEvents(mouseEvent(SDL_MOUSEMOTION, SDL_BUTTON_LEFT,
	                              b.xpos + 5, b.ypos + 5));
	assert(engine.equationText() == "");

	// the gap between two buttons belongs to neither
	handleButtonEvents(mouseEvent(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT,
	                              digitButtons[5].xpos + digitButtons[5].width + 5,
	                              digitButtons[5].ypos + 5));
	assert(engine.equationText() == "");

	// a click on a display slot does nothing: the slots are output, and
	// handleButtonEvents() only offers the event to the clickable buttons
	for (int i = 0; i < CalculatorEngine::SLOT_COUNT; i++) {
		handleButtonEvents(clickCentre(displaySlots[i]));
	}
	assert(engine.equationText() == "");
}

// The rendering side: the engine's display characters become slot textures.
static void testDisplayRendering() {
	assert(textureForChar('0') == digitTextures[0]);
	assert(textureForChar('9') == digitTextures[9]);
	assert(textureForChar('+') == plusSignT);
	assert(textureForChar('-') == minusSignT);
	assert(textureForChar('*') == multiplyT);
	assert(textureForChar('/') == divideT);
	assert(textureForChar('.') == decimalT);
	assert(textureForChar('\0') == emptyT); // an empty slot
	assert(textureForChar('x') == emptyT);  // nothing the engine can produce

	// a fresh equation renders as seven empty slots
	changeDisplay(clear.id);
	syncDisplay();
	assert(renderedDisplay() == "_______");

	// every glyph the engine can put on screen, drawn at once
	changeDisplay(clear.id);
	type("1+2*3/4");
	syncDisplay();
	assert(renderedDisplay() == "1+2*3/4");

	// a decimal point and the trailing empty slots
	changeDisplay(clear.id);
	type("2.5");
	syncDisplay();
	assert(renderedDisplay() == "2.5____");

	// a negative result draws its sign with the same texture a subtraction uses
	changeDisplay(clear.id);
	type("3-10=");
	assert(engine.equationText() == "-7");
	syncDisplay();
	assert(renderedDisplay() == "-7_____");

	// the display is a window onto the end of a long equation, and the frontend
	// renders that window rather than the whole equation
	changeDisplay(clear.id);
	type("123456789");
	syncDisplay();
	assert(renderedDisplay() == "3456789");

	// displayButtonTextures() refreshes the slots itself before drawing, so the
	// rendered frame never lags the engine by an input
	changeDisplay(clear.id);
	type("42");
	displayButtonTextures();
	assert(renderedDisplay() == "42_____");

	// a failed '=' leaves the drawn display exactly as it was, which is why the
	// button looks like it did nothing (issue #43 tracks surfacing the failure)
	changeDisplay(clear.id);
	type("6/0");
	displayButtonTextures();
	assert(renderedDisplay() == "6/0____");
	type("=");
	displayButtonTextures();
	assert(renderedDisplay() == "6/0____");
}

int main() {
	// Select the headless drivers before init(). Setting them in-process rather
	// than expecting them in the environment keeps the suite runnable by a bare
	// `./testingFrontend` and independent of whether a display server exists.
	setenv("SDL_VIDEODRIVER", "dummy", 1);
	setenv("SDL_RENDER_DRIVER", "software", 1);

	if (!init()) {
		cerr << "testingFrontend: init() failed, cannot run frontend tests" << endl;
		close();
		return 1;
	}
	if (!loadMedia()) {
		cerr << "testingFrontend: loadMedia() failed — run this from the repository "
		        "directory, where the PNG assets live" << endl;
		close();
		return 1;
	}

	testButtonIdsAssignedByInit();
	testAllAssetsLoad();
	testButtonIdForChar();
	testChangeDisplayDispatch();
	testTextInputEvents();
	testKeyDownEvents();
	testClickDispatch();
	testDisplayRendering();

	close();
	cout << "All frontend tests passed.\n";
	return 0;
}
