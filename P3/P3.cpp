// P3.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <SDL.h>

using namespace std;

int main(int argc, char* argv[])
{
	SDL_Surface* winSurface = NULL;
	SDL_Window* window = NULL;
	SDL_Renderer* renderer = NULL;

	// Initialize SDL. SDL_Init will return -1 if it fails.
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		cout << "Error initializing SDL: " << SDL_GetError() << endl;
		system("pause");
		// End the program
		return 1;
	}

	// Create our window
	if (true) {
		window = SDL_CreateWindow("Example", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, 0);
	}
	else {
		if (SDL_CreateWindowAndRenderer(1280, 720, 0, &window, &renderer)) {
			cout << "SDL_CreateWindowAndRenderer: " << SDL_GetError() << endl;
		}
	}

	// Make sure creating the window succeeded
	if (!window) {
		cout << "Error creating window: " << SDL_GetError() << endl;
		system("pause");
		// End the program
		return 1;
	}

	if (false) {
		// Get the surface from the window
		winSurface = SDL_GetWindowSurface(window);

		// Make sure getting the surface succeeded
		if (!winSurface) {
			cout << "Error getting surface: " << SDL_GetError() << endl;
			system("pause");
			// End the program
			return 1;
		}

		// Fill the window with a white rectangle
		SDL_FillRect(winSurface, NULL, SDL_MapRGB(winSurface->format, 255, 255, 255));

		// Update the window display
		SDL_UpdateWindowSurface(window);
	}


	if (true) {
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	}
	if (nullptr == renderer) {
		cout << "Error getting renderer: " << SDL_GetError() << endl;
	}
	else {
		if (SDL_SetRenderDrawColor(renderer, 127, 127, 127, SDL_ALPHA_OPAQUE)) {
			cout << "SDL_SetRenderDrawColor: " << SDL_GetError() << endl;
		}

		if (SDL_RenderClear(renderer)) {
			cout << "SDL_RenderClear: " << SDL_GetError() << endl;
		}
		SDL_Rect rect;
		rect.x = 0;
		rect.y = 0;
		rect.w = 100;
		rect.h = 100;

		if (SDL_RenderFillRect(renderer, &rect)) {
			cout << "SDL_RenderFillRect: " << SDL_GetError() << endl;
		}
		if (SDL_RenderFlush(renderer)) {
			cout << "SDL_RenderFlush: " << SDL_GetError() << endl;
		}

		SDL_RenderPresent(renderer);
	}
	// Wait
	while (true) {
		SDL_Event e;
		if (SDL_WaitEvent(&e)) {
			if (e.type == SDL_QUIT) {
				break;
			}
		}

		cout << "Event type: " << e.type << endl;
	}

	// Destroy the window. This will also destroy the surface
	SDL_DestroyWindow(window);

	// Quit SDL
	SDL_Quit();

	// End the program
	return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
