// SDL/GUI frontend for the calculator. All calculation and display state lives
// in the shared CalculatorEngine; this file only renders the engine's display
// and translates SDL mouse clicks into engine input. See CalculatorEngine.h.
#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <vector>
#include "CalculatorEngine.h"

// using namespace std
using namespace std;

// functions
void changeDisplay(int id); // routes a button id to the calculator engine
SDL_Texture* textureForChar(char value); // maps a display character to its texture
void syncDisplay(); // pushes the engine's display state into the slot buttons
SDL_Texture* loadTexture(const char* path); // loads a PNG into a texture, freeing the intermediate surface
bool init(); // returns false if SDL/window/renderer setup fails
bool loadMedia(); // returns false if any texture fails to load
void close();
void handleButtonEvents(SDL_Event e);
void displayButtonTextures();

// screen dimensions
const int SCREEN_WIDTH = 750;
const int SCREEN_HEIGHT = 650;

// the window we'll be rendering to
SDL_Window* window = NULL;

// the renderer
SDL_Renderer* renderer = NULL;

// the shared, UI-agnostic calculator core
CalculatorEngine engine;

// button class
class Button {
  public:
	int xpos;
	int ypos;
	int width;
	int height;
	SDL_Texture* currentTexture;
	int id;

	Button() {
		xpos = 0;
		ypos = 0;
		width = 0;
		height = 0;
		currentTexture = NULL;
		id = 0;
	}

	void init(int x, int y, int w, int h, int num) {
		xpos = x;
		ypos = y;
		width = w;
		height = h;
		id = num;
	}

	void loadTexture(SDL_Texture* textureToLoad) {
		currentTexture = textureToLoad;
	}

	void displayTexture() {
		SDL_Rect renderQuad = {xpos, ypos, width, height};
		SDL_RenderCopy(renderer, currentTexture, NULL, &renderQuad);
	}

	void handleEvent(SDL_Event* e) {
		// only a mouse-button press does anything
		if (e->type == SDL_MOUSEBUTTONDOWN) {
			// the click event already carries the mouse position
			int x = e->button.x;
			int y = e->button.y;

			// check if the click landed inside the button
			bool inside = true;
			if (x < xpos) {
				inside = false;
			}
			else if (x > xpos + width) {
				inside = false;
			}
			else if (y < ypos) {
				inside = false;
			}
			else if (y > ypos + height) {
				inside = false;
			}

			// dispatch the click
			if (inside) {
				changeDisplay(id);
			}
		}
	}
};

// textures
SDL_Texture* emptyT = NULL;
SDL_Texture* digitTextures[10] = {NULL}; // index == digit value (0-9)
SDL_Texture* minusSignT = NULL;
SDL_Texture* plusSignT = NULL;
SDL_Texture* multiplyT = NULL;
SDL_Texture* divideT = NULL;
SDL_Texture* clearT = NULL;
SDL_Texture* equalsT = NULL;

// display slot buttons (top row, ids 13-19), indexed 0-6
Button displaySlots[CalculatorEngine::SLOT_COUNT];

// digit buttons, indexed by digit value (ids 0-9)
Button digitButtons[10];

// operator buttons
Button plusSign;
Button minusSign;
Button multiply;
Button divide;

// command buttons
Button clear;
Button equalsSign;

// every clickable button, populated once in init(); handleButtonEvents() iterates this
// instead of listing each button by name
vector<Button*> clickableButtons;

// clickableButtons plus the display slots, populated once in init();
// displayButtonTextures() iterates this instead of listing each button by name
vector<Button*> allButtons;

// Routes a clicked button id to the engine. Digit ids (0-9) and operator/command
// ids drive the calculation; display-slot ids (13-19) are inert.
void changeDisplay(int id) {
	if (id >= 0 && id <= 9) {
		engine.inputDigit(id);
	}
	else if (id == 10) {
		engine.inputOperator('-');
	}
	else if (id == 11) {
		engine.inputOperator('+');
	}
	else if (id == 12) {
		engine.inputOperator('*');
	}
	else if (id == 22) {
		engine.inputOperator('/');
	}
	else if (id == 20) {
		engine.clear();
	}
	else if (id == 21) {
		int result = 0;
		engine.evaluate(result); // the engine updates its display to show the result
	}
}

// Maps a display character produced by the engine to the texture that draws it.
SDL_Texture* textureForChar(char value) {
	if (value >= '0' && value <= '9') {
		return digitTextures[value - '0'];
	}
	switch (value) {
		case '+': return plusSignT;
		case '-': return minusSignT;
		case '*': return multiplyT;
		case '/': return divideT;
		default:  return emptyT; // '\0' (empty slot) or anything unrenderable
	}
}

// Refreshes the seven display-slot buttons from the engine's current display.
void syncDisplay() {
	for (int i = 0; i < engine.slotCount(); i++) {
		displaySlots[i].loadTexture(textureForChar(engine.slotChar(i)));
	}
}

bool init() {
	// initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		cerr << "SDL_Init failed: " << SDL_GetError() << endl;
		return false;
	}

	// create window
	window = SDL_CreateWindow("Simple Calculator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	if (window == NULL) {
		cerr << "SDL_CreateWindow failed: " << SDL_GetError() << endl;
		return false;
	}

	// get window surface
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (renderer == NULL) {
		cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << endl;
		return false;
	}

	// initialize renderer color
	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

	// initialize PNG loading
	int imgFlags = IMG_INIT_PNG;
	if ((IMG_Init(imgFlags) & imgFlags) != imgFlags) {
		cerr << "IMG_Init failed: " << IMG_GetError() << endl;
		return false;
	}

	// set middle button x and y
	int middleX = SCREEN_WIDTH/2 - 50;
	int middleY = SCREEN_HEIGHT/2 - 50;

	// initialize buttons
	digitButtons[0].init(middleX, middleY + 250, 100, 100, 0);
	digitButtons[1].init(middleX - 125, middleY + 125, 100, 100, 1);
	digitButtons[2].init(middleX, middleY + 125, 100, 100, 2);
	digitButtons[3].init(middleX + 125, middleY + 125, 100, 100, 3);
	digitButtons[4].init(middleX - 125, middleY, 100, 100, 4);
	digitButtons[5].init(middleX, middleY, 100, 100, 5);
	digitButtons[6].init(middleX + 125, middleY, 100, 100, 6);
	digitButtons[7].init(middleX - 125, middleY - 125, 100, 100, 7);
	digitButtons[8].init(middleX, middleY - 125, 100, 100, 8);
	digitButtons[9].init(middleX + 125, middleY - 125, 100, 100, 9);

	minusSign.init(middleX + 250, middleY, 100, 100, 10);
	plusSign.init(middleX + 250, middleY + 125, 100, 100, 11);
	multiply.init(middleX + 250, middleY - 125, 100, 100, 12);
	divide.init(middleX - 250, middleY, 100, 100, 22); // left column, below clear

	displaySlots[0].init(middleX - 300, middleY - 250, 100, 100, 13);
	displaySlots[1].init(middleX - 200, middleY - 250, 100, 100, 14);
	displaySlots[2].init(middleX - 100, middleY - 250, 100, 100, 15);
	displaySlots[3].init(middleX, middleY - 250, 100, 100, 16);
	displaySlots[4].init(middleX + 100, middleY - 250, 100, 100, 17);
	displaySlots[5].init(middleX + 200, middleY - 250, 100, 100, 18);
	displaySlots[6].init(middleX + 300, middleY - 250, 100, 100, 19);

	clear.init(middleX - 250, middleY - 125, 100, 100, 20);
	equalsSign.init(middleX + 250, middleY + 250, 100, 100, 21);

	// populate the button containers now that every button has its position/id
	clickableButtons.clear();
	for (int i = 0; i < 10; i++) {
		clickableButtons.push_back(&digitButtons[i]);
	}
	clickableButtons.push_back(&minusSign);
	clickableButtons.push_back(&plusSign);
	clickableButtons.push_back(&multiply);
	clickableButtons.push_back(&divide);
	clickableButtons.push_back(&clear);
	clickableButtons.push_back(&equalsSign);

	allButtons = clickableButtons;
	for (int i = 0; i < CalculatorEngine::SLOT_COUNT; i++) {
		allButtons.push_back(&displaySlots[i]);
	}

	return true;
}

// Loads a PNG at path into a texture. The intermediate surface is always
// freed here so callers never have to manage it (fixes the leak where a
// single reused tempSurface was only freed once, after the last of 16 loads).
// Returns NULL and logs to cerr if the load or texture creation fails.
SDL_Texture* loadTexture(const char* path) {
	SDL_Surface* surface = IMG_Load(path);
	if (surface == NULL) {
		cerr << "IMG_Load failed for " << path << ": " << IMG_GetError() << endl;
		return NULL;
	}

	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (texture == NULL) {
		cerr << "SDL_CreateTextureFromSurface failed for " << path << ": " << SDL_GetError() << endl;
	}

	SDL_FreeSurface(surface);
	return texture;
}

bool loadMedia() {
	// initialize textures
	static const char* digitPaths[10] = {
		"zero.png", "one.png", "two.png", "three.png", "four.png",
		"five.png", "six.png", "seven.png", "eight.png", "nine.png"
	};
	emptyT = loadTexture("empty.png");
	for (int i = 0; i < 10; i++) {
		digitTextures[i] = loadTexture(digitPaths[i]);
	}
	minusSignT = loadTexture("minus.png");
	plusSignT = loadTexture("plus.png");
	multiplyT = loadTexture("multiply.png");
	divideT = loadTexture("divide.png");
	clearT = loadTexture("clear.png");
	equalsT = loadTexture("equals.png");

	if (emptyT == NULL || minusSignT == NULL || plusSignT == NULL ||
		multiplyT == NULL || divideT == NULL || clearT == NULL || equalsT == NULL) {
		return false;
	}
	for (int i = 0; i < 10; i++) {
		if (digitTextures[i] == NULL) {
			return false;
		}
	}

	// load button textures (display slots are filled each frame by syncDisplay)
	for (int i = 0; i < 10; i++) {
		digitButtons[i].loadTexture(digitTextures[i]);
	}
	plusSign.loadTexture(plusSignT);
	minusSign.loadTexture(minusSignT);
	multiply.loadTexture(multiplyT);
	divide.loadTexture(divideT);
	clear.loadTexture(clearT);
	equalsSign.loadTexture(equalsT);

	return true;
}

void close() {
	// free resources
	SDL_DestroyTexture(emptyT);
	for (int i = 0; i < 10; i++) {
		SDL_DestroyTexture(digitTextures[i]);
	}
	SDL_DestroyTexture(plusSignT);
	SDL_DestroyTexture(minusSignT);
	SDL_DestroyTexture(multiplyT);
	SDL_DestroyTexture(divideT);
	SDL_DestroyTexture(clearT);
	SDL_DestroyTexture(equalsT);

	// destroy renderer and window (renderer first, per SDL convention)
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	// exit SDL subystems
	IMG_Quit();
	SDL_Quit();
}

void handleButtonEvents(SDL_Event e) {
	for (Button* button : clickableButtons) {
		button->handleEvent(&e);
	}
}

void displayButtonTextures() {
	// refresh the display slots from the engine, then draw every button
	syncDisplay();

	for (Button* button : allButtons) {
		button->displayTexture();
	}
}

// argc/argv are unnamed: SDL requires this signature, but the app uses neither
int main(int, char*[]) {
	if (!init()) {
		cerr << "Initialization failed, exiting." << endl;
		close(); // tear down whatever init() managed to set up before it failed
		return 1;
	}

	if (!loadMedia()) {
		cerr << "Failed to load media, exiting." << endl;
		close();
		return 1;
	}

	// event handler
	SDL_Event e;

	bool running = true;

	// while app is running
	while (running) {
		// handle events on queue
		while (SDL_PollEvent(&e) != 0) {
			// if user requests quit
			if (e.type == SDL_QUIT) {
				running = false;
			}

			handleButtonEvents(e);
		}

		// fill the surface with the background color
		SDL_SetRenderDrawColor(renderer, 200, 200, 200, 0xFF);
		SDL_RenderClear(renderer);

		// display textures
		displayButtonTextures();

		// update the surface
		SDL_RenderPresent(renderer);
	}

	close();

	return 0;
}
