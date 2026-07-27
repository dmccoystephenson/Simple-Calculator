// SDL/GUI frontend for the calculator. All calculation and display state lives
// in the shared CalculatorEngine; this file only renders the engine's display
// and translates SDL mouse clicks into engine input. See CalculatorEngine.h.
#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
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
SDL_Texture* oneT = NULL;
SDL_Texture* twoT = NULL;
SDL_Texture* threeT = NULL;
SDL_Texture* fourT = NULL;
SDL_Texture* fiveT = NULL;
SDL_Texture* sixT = NULL;
SDL_Texture* sevenT = NULL;
SDL_Texture* eightT = NULL;
SDL_Texture* nineT = NULL;
SDL_Texture* zeroT = NULL;
SDL_Texture* minusSignT = NULL;
SDL_Texture* plusSignT = NULL;
SDL_Texture* multiplyT = NULL;
SDL_Texture* divideT = NULL;
SDL_Texture* clearT = NULL;
SDL_Texture* equalsT = NULL;

// display slot buttons (top row, ids 13-19)
Button displayOne;
Button displayTwo;
Button displayThree;
Button displayFour;
Button displayFive;
Button displaySix;
Button displaySeven;

// digit buttons (ids 0-9)
Button one;
Button two;
Button three;
Button four;
Button five;
Button six;
Button seven;
Button eight;
Button nine;
Button zero;

// operator buttons
Button plusSign;
Button minusSign;
Button multiply;
Button divide;

// command buttons
Button clear;
Button equalsSign;

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
	switch (value) {
		case '0': return zeroT;
		case '1': return oneT;
		case '2': return twoT;
		case '3': return threeT;
		case '4': return fourT;
		case '5': return fiveT;
		case '6': return sixT;
		case '7': return sevenT;
		case '8': return eightT;
		case '9': return nineT;
		case '+': return plusSignT;
		case '-': return minusSignT;
		case '*': return multiplyT;
		case '/': return divideT;
		default:  return emptyT; // '\0' (empty slot) or anything unrenderable
	}
}

// Refreshes the seven display-slot buttons from the engine's current display.
void syncDisplay() {
	Button* displaySlots[CalculatorEngine::SLOT_COUNT] = {
		&displayOne, &displayTwo, &displayThree, &displayFour,
		&displayFive, &displaySix, &displaySeven
	};
	for (int i = 0; i < engine.slotCount(); i++) {
		displaySlots[i]->loadTexture(textureForChar(engine.slotChar(i)));
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
	zero.init(middleX, middleY + 250, 100, 100, 0);
	one.init(middleX - 125, middleY + 125, 100, 100, 1);
	two.init(middleX, middleY + 125, 100, 100, 2);
	three.init(middleX + 125, middleY + 125, 100, 100, 3);
	four.init(middleX - 125, middleY, 100, 100, 4);
	five.init(middleX, middleY, 100, 100, 5);
	six.init(middleX + 125, middleY, 100, 100, 6);
	seven.init(middleX - 125, middleY - 125, 100, 100, 7);
	eight.init(middleX, middleY - 125, 100, 100, 8);
	nine.init(middleX + 125, middleY - 125, 100, 100, 9);

	minusSign.init(middleX + 250, middleY, 100, 100, 10);
	plusSign.init(middleX + 250, middleY + 125, 100, 100, 11);
	multiply.init(middleX + 250, middleY - 125, 100, 100, 12);
	divide.init(middleX - 250, middleY, 100, 100, 22); // left column, below clear

	displayOne.init(middleX - 300, middleY - 250, 100, 100, 13);
	displayTwo.init(middleX - 200, middleY - 250, 100, 100, 14);
	displayThree.init(middleX - 100, middleY - 250, 100, 100, 15);
	displayFour.init(middleX, middleY - 250, 100, 100, 16);
	displayFive.init(middleX + 100, middleY - 250, 100, 100, 17);
	displaySix.init(middleX + 200, middleY - 250, 100, 100, 18);
	displaySeven.init(middleX + 300, middleY - 250, 100, 100, 19);

	clear.init(middleX - 250, middleY - 125, 100, 100, 20);
	equalsSign.init(middleX + 250, middleY + 250, 100, 100, 21);

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
	emptyT = loadTexture("empty.png");
	oneT = loadTexture("one.png");
	twoT = loadTexture("two.png");
	threeT = loadTexture("three.png");
	fourT = loadTexture("four.png");
	fiveT = loadTexture("five.png");
	sixT = loadTexture("six.png");
	sevenT = loadTexture("seven.png");
	eightT = loadTexture("eight.png");
	nineT = loadTexture("nine.png");
	zeroT = loadTexture("zero.png");
	minusSignT = loadTexture("minus.png");
	plusSignT = loadTexture("plus.png");
	multiplyT = loadTexture("multiply.png");
	divideT = loadTexture("divide.png");
	clearT = loadTexture("clear.png");
	equalsT = loadTexture("equals.png");

	if (emptyT == NULL || oneT == NULL || twoT == NULL || threeT == NULL ||
		fourT == NULL || fiveT == NULL || sixT == NULL || sevenT == NULL ||
		eightT == NULL || nineT == NULL || zeroT == NULL || minusSignT == NULL ||
		plusSignT == NULL || multiplyT == NULL || divideT == NULL ||
		clearT == NULL || equalsT == NULL) {
		return false;
	}

	// load button textures (display slots are filled each frame by syncDisplay)
	one.loadTexture(oneT);
	two.loadTexture(twoT);
	three.loadTexture(threeT);
	four.loadTexture(fourT);
	five.loadTexture(fiveT);
	six.loadTexture(sixT);
	seven.loadTexture(sevenT);
	eight.loadTexture(eightT);
	nine.loadTexture(nineT);
	zero.loadTexture(zeroT);
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
	SDL_DestroyTexture(oneT);
	SDL_DestroyTexture(twoT);
	SDL_DestroyTexture(threeT);
	SDL_DestroyTexture(fourT);
	SDL_DestroyTexture(fiveT);
	SDL_DestroyTexture(sixT);
	SDL_DestroyTexture(sevenT);
	SDL_DestroyTexture(eightT);
	SDL_DestroyTexture(nineT);
	SDL_DestroyTexture(zeroT);
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
	one.handleEvent(&e);
	two.handleEvent(&e);
	three.handleEvent(&e);
	four.handleEvent(&e);
	five.handleEvent(&e);
	six.handleEvent(&e);
	seven.handleEvent(&e);
	eight.handleEvent(&e);
	nine.handleEvent(&e);
	zero.handleEvent(&e);
	plusSign.handleEvent(&e);
	minusSign.handleEvent(&e);
	multiply.handleEvent(&e);
	divide.handleEvent(&e);
	clear.handleEvent(&e);
	equalsSign.handleEvent(&e);
}

void displayButtonTextures() {
	// refresh the display slots from the engine, then draw every button
	syncDisplay();

	displayOne.displayTexture();
	displayTwo.displayTexture();
	displayThree.displayTexture();
	displayFour.displayTexture();
	displayFive.displayTexture();
	displaySix.displayTexture();
	displaySeven.displayTexture();

	one.displayTexture();
	two.displayTexture();
	three.displayTexture();
	four.displayTexture();
	five.displayTexture();
	six.displayTexture();
	seven.displayTexture();
	eight.displayTexture();
	nine.displayTexture();
	zero.displayTexture();
	plusSign.displayTexture();
	minusSign.displayTexture();
	multiply.displayTexture();
	divide.displayTexture();
	clear.displayTexture();
	equalsSign.displayTexture();
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
