// using SDL and iostream
#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <string>

// using namespace std
using namespace std;

void changeDisplay(int id);
int currentScreen = 0;

// screen dimensions
const int SCREEN_WIDTH = 750;
const int SCREEN_HEIGHT = 650;

// screen color
int backgroundR = 255;
int backgroundG = 255;
int backgroundB = 255;

// the window we'll be rendering to
SDL_Window* window = NULL;

// the renderer
SDL_Renderer* renderer = NULL;

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
SDL_Texture* clearT = NULL;

void changeBackgroundColor(int r, int g, int b) {
	backgroundR = r;
	backgroundG = g;
	backgroundB = b;
}

class Button {
  public:
	int xpos;
	int ypos;
	int width;
	int height;
	int red;
	int green;
	int blue;
	SDL_Texture* currentTexture;
	int id;
	int currentBroadcastingDisplay;
	
	Button() {
		xpos = 0;
		ypos = 0;
		width = 0;
		height = 0;
		red = 0;
		green = 0;
		blue = 0;
		currentTexture = NULL;
		id = 0;
	}
	
	void init(int x, int y, int w, int h, int r, int g, int b, int num) {
		xpos = x;
		ypos = y;
		width = w;
		height = h;
		red = r;
		green = g;
		blue = b;
		id = num;
	}
	
	void loadTexture(SDL_Texture* textureToLoad) {
		currentTexture = textureToLoad;
	}
	
	void render() {
		SDL_Rect fillRect = {xpos, ypos, width, height};
		SDL_SetRenderDrawColor(renderer, red, green, blue, 0xFF);
		SDL_RenderFillRect(renderer, &fillRect);
	}
	
	void displayTexture() {
		SDL_Rect renderQuad = {xpos, ypos, width, height};
		SDL_RenderCopy(renderer, currentTexture, NULL, &renderQuad);
	}
	
	void handleEvent(SDL_Event* e) {
		// if mouse event happened
		if (e->type == SDL_MOUSEMOTION || e->type == SDL_MOUSEBUTTONDOWN || e->type == SDL_MOUSEBUTTONUP) {
			// get mouse position
			int x, y;
			SDL_GetMouseState(&x, &y);
			
			// check if mouse is in button
			bool inside = true;
			
			// if mouse is left of the button
			if (x < xpos) {
				inside = false;
			}
			
			// if mouse is right of the button
			else if (x > xpos + width) {
				inside = false;
			}
			
			// if mouse is above the button
			else if (y < ypos) {
				inside = false;
			}
			
			// if mouse is below the button
			else if (y > ypos + height) {
				inside = false;
			}
			
			// mouse is outside button
			if (!inside) {
				if (red > 0) {
					red = 250;
				}
				if (green > 0) {
					green = 250;
				}
				if (blue > 0) {
					blue = 250;
				}
			}
			
			// mouse is inside button
			else {
				// set mouse over sprite
				switch (e->type) {
					case SDL_MOUSEMOTION:
						if (red > 0) {
							red = 200;
						}
						if (green > 0) {
							green = 200;
						}
						if (blue > 0) {
							blue = 200;
						}
						break;
						
					case SDL_MOUSEBUTTONDOWN:
						if (red > 0) {
							red = 255;
						}
						if (green > 0) {
							green = 255;
						}
						if (blue > 0) {
							blue = 255;
						}
						changeDisplay(id);
						break;
					
					case SDL_MOUSEBUTTONUP:
						if (red > 0) {
							red = 200;
						}
						if (green > 0) {
							green = 200;
						}
						if (blue > 0) {
							blue = 200;
						}
						break;
				}
			}
		}
	}
};

Button displayOne;
Button displayTwo;
Button displayThree;
Button displayFour;
Button displayFive;
Button displaySix;
Button displaySeven;

Button seven;
Button eight;
Button nine;
Button four;
Button five;
Button six;
Button one;
Button two;
Button three;
Button zero;
Button plusSign;
Button minusSign;
Button multiply;

Button clear;

void changeDisplay(int id) {
	switch(id) {
		// if user clicks on clear button
		case 20:
			displayOne.loadTexture(emptyT);
			displayTwo.loadTexture(emptyT);
			displayThree.loadTexture(emptyT);
			displayFour.loadTexture(emptyT);
			displayFive.loadTexture(emptyT);
			displaySix.loadTexture(emptyT);
			displaySeven.loadTexture(emptyT);
			currentScreen = 0;
			break;

		// if user clicks on plus button
		case 11:
			switch(currentScreen) {
				case 0:
					displayOne.loadTexture(plusSignT);
					if (currentScreen < 6) {
						currentScreen++;
					}
					break;
				case 1:
					displayTwo.loadTexture(plusSignT);
					if (currentScreen < 6) {
						currentScreen++;
					}
					break;
				case 2:
					displayThree.loadTexture(plusSignT);
					if (currentScreen < 6) {
						currentScreen++;
					}
					break;
				case 3:
					displayFour.loadTexture(plusSignT);
					if (currentScreen < 6) {
						currentScreen++;
					}
					break;
				case 4:
					displayFive.loadTexture(plusSignT);
					if (currentScreen < 6) {
						currentScreen++;
					}
					break;
				case 5:
					displaySix.loadTexture(plusSignT);
					if (currentScreen < 6) {
						currentScreen++;
					}
					break;
				case 6:
					displaySeven.loadTexture(plusSignT);
					if (currentScreen < 6) {
						currentScreen++;
					}
					break;
			}
			break;

		// if user clicks on zero button
		case 0:
			switch(currentScreen) {
				case 0:
					displayOne.loadTexture(zeroT);
					if (currentScreen < 6) {
						currentScreen++;
					}
					break;
				case 1:
					displayTwo.loadTexture(zeroT);
					if (currentScreen < 6) {
						currentScreen++;
					}
					break;
				case 2:
					displayThree.loadTexture(zeroT);
					if (currentScreen < 6) {
						currentScreen++;
					}
					break;
				case 3:
					displayFour.loadTexture(zeroT);
					if (currentScreen < 6) {
						currentScreen++;
					}
					break;
				case 4:
					displayFive.loadTexture(zeroT);
					if (currentScreen < 6) {
						currentScreen++;
					}
					break;
				case 5:
					displaySix.loadTexture(zeroT);
					if (currentScreen < 6) {
						currentScreen++;
					}
					break;
				case 6:
					displaySeven.loadTexture(zeroT);
					if (currentScreen < 6) {
						currentScreen++;
					}
					break;
			}
			break;
		// if user clicks on one button
		case 1:
			switch(currentScreen) {
				case 0:
					displayOne.loadTexture(oneT);
					if (currentScreen < 6) {
						currentScreen++;
					}
					break;
				case 1:
					displayTwo.loadTexture(oneT);
					if (currentScreen < 6) {
						currentScreen++;
					}
					break;
				case 2:
					displayThree.loadTexture(oneT);
					if (currentScreen < 6) {
						currentScreen++;
					}
					break;
				case 3:
					displayFour.loadTexture(oneT);
					if (currentScreen < 6) {
						currentScreen++;
					}
					break;
				case 4:
					displayFive.loadTexture(oneT);
					if (currentScreen < 6) {
						currentScreen++;
					}
					break;
				case 5:
					displaySix.loadTexture(oneT);
					if (currentScreen < 6) {
						currentScreen++;
					}
					break;
				case 6:
					displaySeven.loadTexture(oneT);
					if (currentScreen < 6) {
						currentScreen++;
					}
					break;
			}
			break;
		// if user clicks on two button
		case 2:
			switch(currentScreen) {
				case 0:
					displayOne.loadTexture(twoT);
					if (currentScreen < 6) {
						currentScreen++;
					}
					break;
				case 1:
					displayTwo.loadTexture(twoT);
					if (currentScreen < 6) {
						currentScreen++;
					}
					break;
				case 2:
					displayThree.loadTexture(twoT);
					if (currentScreen < 6) {
						currentScreen++;
					}
					break;
				case 3:
					displayFour.loadTexture(twoT);
					if (currentScreen < 6) {
						currentScreen++;
					}
					break;
				case 4:
					displayFive.loadTexture(twoT);
					if (currentScreen < 6) {
						currentScreen++;
					}
					break;
				case 5:
					displaySix.loadTexture(twoT);
					if (currentScreen < 6) {
						currentScreen++;
					}
					break;
				case 6:
					displaySeven.loadTexture(twoT);
					if (currentScreen < 6) {
						currentScreen++;
					}
					break;
			}
	}
}

int main(int argc, char* args[]) {
	// initialize SDL
	SDL_Init(SDL_INIT_VIDEO);
	
	// create window
	window = SDL_CreateWindow("SDL Practice - First Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	
	// get window surface
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	
	// initialize renderer color
	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	
	// initialize PNG loading
	int imgFlags = IMG_INIT_PNG;
	IMG_Init(imgFlags);
	
	// create tempSurface
	SDL_Surface* tempSurface;
	
	// initialize textures
	tempSurface = IMG_Load("empty.png");
	emptyT = SDL_CreateTextureFromSurface(renderer, tempSurface);
	tempSurface = IMG_Load("one.png");
	oneT = SDL_CreateTextureFromSurface(renderer, tempSurface);
	tempSurface = IMG_Load("two.png");
	twoT = SDL_CreateTextureFromSurface(renderer, tempSurface);
	tempSurface = IMG_Load("three.png");
	threeT = SDL_CreateTextureFromSurface(renderer, tempSurface);
	tempSurface = IMG_Load("four.png");
	fourT = SDL_CreateTextureFromSurface(renderer, tempSurface);
	tempSurface = IMG_Load("five.png");
	fiveT = SDL_CreateTextureFromSurface(renderer, tempSurface);
	tempSurface = IMG_Load("six.png");
	sixT = SDL_CreateTextureFromSurface(renderer, tempSurface);
	tempSurface = IMG_Load("seven.png");
	sevenT = SDL_CreateTextureFromSurface(renderer, tempSurface);
	tempSurface = IMG_Load("eight.png");
	eightT = SDL_CreateTextureFromSurface(renderer, tempSurface);
	tempSurface = IMG_Load("nine.png");
	nineT = SDL_CreateTextureFromSurface(renderer, tempSurface);
	tempSurface = IMG_Load("zero.png");
	zeroT = SDL_CreateTextureFromSurface(renderer, tempSurface);
	tempSurface = IMG_Load("minus.png");
	minusSignT = SDL_CreateTextureFromSurface(renderer, tempSurface);
	tempSurface = IMG_Load("plus.png");
	plusSignT = SDL_CreateTextureFromSurface(renderer, tempSurface);
	tempSurface = IMG_Load("multiply.png");
	multiplyT = SDL_CreateTextureFromSurface(renderer, tempSurface);
	tempSurface = IMG_Load("clear.png");
	clearT = SDL_CreateTextureFromSurface(renderer, tempSurface);
	
	// free tempSurface
	SDL_FreeSurface(tempSurface);
	
	// event handler
	SDL_Event e;
	
	int middleX = SCREEN_WIDTH/2 - 50;
	int middleY = SCREEN_HEIGHT/2 - 50;
	
	zero.init(middleX, middleY + 250, 100, 100, 0x00, 150, 0x00, 0);
	one.init(middleX - 125, middleY + 125, 100, 100, 0x00, 150, 0x00, 1);
	two.init(middleX, middleY + 125, 100, 100, 0x00, 150, 0x00, 2);
	three.init(middleX + 125, middleY + 125, 100, 100, 0x00, 150, 0x00, 3);
	four.init(middleX - 125, middleY, 100, 100, 0x00, 150, 0x00, 4);
	five.init(middleX, middleY, 100, 100, 0x00, 150, 0x00, 5);
	six.init(middleX + 125, middleY, 100, 100, 0x00, 150, 0x00, 6);
	seven.init(middleX - 125, middleY - 125, 100, 100, 0x00, 150, 0x00, 7);
	eight.init(middleX, middleY - 125, 100, 100, 0x00, 150, 0x00, 8);
	nine.init(middleX + 125, middleY - 125, 100, 100, 0x00, 150, 0x00, 9);

	minusSign.init(middleX + 250, middleY, 100, 100, 0x00, 150, 0x00, 10);
	plusSign.init(middleX + 250, middleY + 125, 100, 100, 0x00, 150, 0x00, 11);
	multiply.init(middleX + 250, middleY - 125, 100, 100, 0x00, 150, 0x00, 12);

	displayOne.init(middleX - 300, middleY - 250, 100, 100, 0x00, 150, 0x00, 13);
	displayTwo.init(middleX - 200, middleY - 250, 100, 100, 0x00, 150, 0x00, 14);
	displayThree.init(middleX - 100, middleY - 250, 100, 100, 0x00, 150, 0x00, 15);
	displayFour.init(middleX, middleY - 250, 100, 100, 0x00, 150, 0x00, 16);
	displayFive.init(middleX + 100, middleY - 250, 100, 100, 0x00, 150, 0x00, 17);
	displaySix.init(middleX + 200, middleY - 250, 100, 100, 0x00, 150, 0x00, 18);
	displaySeven.init(middleX + 300, middleY - 250, 100, 100, 0x00, 150, 0x00, 19);
	
	clear.init(middleX - 250, middleY - 125, 100, 100, 0x00, 150, 0x00, 20);

	changeBackgroundColor(200, 200, 200);

	// load initial display textures
	displayOne.loadTexture(emptyT);
	displayTwo.loadTexture(emptyT);
	displayThree.loadTexture(emptyT);
	displayFour.loadTexture(emptyT);
	displayFive.loadTexture(emptyT);
	displaySix.loadTexture(emptyT);
	displaySeven.loadTexture(emptyT);

	bool running = true;
	
	// while app is running
	while (running) {
		// handle events on queue
		while (SDL_PollEvent(&e) != 0) {
			// if user requests quit
			if (e.type == SDL_QUIT) {
				running = false;
			}
			
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
			clear.handleEvent(&e);
		}
		
		// fill the surface white
		SDL_SetRenderDrawColor(renderer, backgroundR, backgroundG, backgroundB, 0xFF);
		SDL_RenderClear(renderer);
		
		// load button textures
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
		clear.loadTexture(clearT);
		
		// display textures
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
		zero.displayTexture();;
		plusSign.displayTexture();
		minusSign.displayTexture();
		multiply.displayTexture();
		clear.displayTexture();
		
		// update the surface
		SDL_RenderPresent(renderer);
	}
	
	// free resources
	SDL_DestroyTexture(oneT);
	SDL_DestroyTexture(twoT);
	SDL_DestroyTexture(threeT);
	SDL_DestroyTexture(fourT);
	// TODO: free rest
	
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);
	SDL_Quit();
	
	return 0;
}