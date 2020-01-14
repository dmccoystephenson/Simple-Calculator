// using SDL and iostream
#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <string>

// using namespace std
using namespace std;

// screen dimensions
const int SCREEN_WIDTH = 700;
const int SCREEN_HEIGHT = 700;

// screen color
int backgroundR = 255;
int backgroundG = 255;
int backgroundB = 255;

// the window we'll be rendering to
SDL_Window* window = NULL;

// the renderer
SDL_Renderer* renderer = NULL;

// textures
SDL_Texture* oneT = NULL;

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
	
	Button() {
		xpos = 0;
		ypos = 0;
		width = 0;
		height = 0;
		red = 0;
		green = 0;
		blue = 0;
	}
	
	void init(int x, int y, int w, int h, int r, int g, int b) {
		xpos = x;
		ypos = y;
		width = w;
		height = h;
		red = r;
		green = g;
		blue = b;
	}
	
	void render() {
		SDL_Rect fillRect = {xpos, ypos, width, height};
		SDL_SetRenderDrawColor(renderer, red, green, blue, 0xFF);
		SDL_RenderFillRect(renderer, &fillRect);
	}
	
	void displayTexture(SDL_Texture* texture) {
		SDL_Rect renderQuad = {xpos, ypos, width, height};
		SDL_RenderCopy(renderer, texture, NULL, &renderQuad);
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
	
	tempSurface = IMG_Load("one.png");
	
	// initialize textures
	oneT = SDL_CreateTextureFromSurface(renderer, tempSurface);
	
	// free tempSurface
	SDL_FreeSurface(tempSurface);
	
	// event handler
	SDL_Event e;
	
	Button display;
	
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
	Button plus;
	Button minus;
	Button multiply;
	
	int middleX = SCREEN_WIDTH/2 - 50;
	int middleY = SCREEN_HEIGHT/2 - 50;
	
	display.init(middleX - 250, middleY - 250, 600, 100, 0x00, 150, 0x00);
	
	one.init(middleX - 125, middleY + 125, 100, 100, 0x00, 150, 0x00);
	two.init(middleX, middleY + 125, 100, 100, 0x00, 150, 0x00);
	three.init(middleX + 125, middleY + 125, 100, 100, 0x00, 150, 0x00);
	four.init(middleX - 125, middleY, 100, 100, 0x00, 150, 0x00);
	five.init(middleX, middleY, 100, 100, 0x00, 150, 0x00);
	six.init(middleX + 125, middleY, 100, 100, 0x00, 150, 0x00);
	seven.init(middleX - 125, middleY - 125, 100, 100, 0x00, 150, 0x00);
	eight.init(middleX, middleY - 125, 100, 100, 0x00, 150, 0x00);
	nine.init(middleX + 125, middleY - 125, 100, 100, 0x00, 150, 0x00);
	zero.init(middleX, middleY + 250, 100, 100, 0x00, 150, 0x00);
	plus.init(middleX + 250, middleY + 125, 100, 100, 0x00, 150, 0x00);
	minus.init(middleX + 250, middleY, 100, 100, 0x00, 150, 0x00);
	multiply.init(middleX + 250, middleY - 125, 100, 100, 0x00, 150, 0x00);

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
			plus.handleEvent(&e);
			minus.handleEvent(&e);
			multiply.handleEvent(&e);
		}
		
		// fill the surface white
		SDL_SetRenderDrawColor(renderer, backgroundR, backgroundG, backgroundB, 0xFF);
		SDL_RenderClear(renderer);
		
		display.render();
		
		one.displayTexture(oneT);
		two.render();
		three.render();
		four.render();
		five.render();
		six.render();
		seven.render();
		eight.render();
		nine.render();
		zero.render();
		plus.render();
		minus.render();
		multiply.render();
		
		// update the surface
		SDL_RenderPresent(renderer);
	}
	
	// free resources
	SDL_DestroyTexture(oneT);
	
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);
	SDL_Quit();
	
	return 0;
}