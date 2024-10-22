// P3.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <cmath>
#include <SDL.h>
#include <SDL_mixer.h>

using namespace std;

Uint32 globalCustomEventId = 0;

int worldFrame = 0;


Uint32 tickFrame(Uint32 interval, void* param) {
	++worldFrame;
	if (worldFrame % 30 == 0) {
		cout << "FRAME: " << worldFrame << endl;
	}
	
	SDL_Event e = {};
	e.type = globalCustomEventId;

	SDL_PushEvent( &e);
	return interval;
}

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

typedef struct M33 {
	double m[3][3];
} M33;

void M33_Identity(M33& m) {
	m.m[0][0] = 1; m.m[0][1] = 0; m.m[0][2] = 0;
	m.m[1][0] = 0; m.m[1][1] = 1; m.m[1][2] = 0;
	m.m[2][0] = 0; m.m[2][1] = 0; m.m[2][2] = 1;
}

void M33_Rotate(M33& m, double phi) {
	m.m[0][0] = cos(phi); m.m[0][1] = -sin(phi); m.m[0][2] = 0;
	m.m[1][0] = sin(phi); m.m[1][1] =  cos(phi); m.m[1][2] = 0;
	m.m[2][0] = 0;		  m.m[2][1] = 0;		 m.m[2][2] = 1;
}

void M33_Translate(M33& m, double x, double y) {
	m.m[0][0] = 1; m.m[0][1] = 0; m.m[0][2] = x;
	m.m[1][0] = 0; m.m[1][1] = 1; m.m[1][2] = y;
	m.m[2][0] = 0; m.m[2][1] = 0; m.m[2][2] = 1;
}

void M33_Copy(M33& s, M33& d) {
	for (int y = 0; y < 3; ++y)
		for (int x = 0; x < 3; ++x)
			d.m[y][x] = s.m[y][x];
}

void M33_Mult(M33& s, M33& o) {
	M33 r;
	for (int l = 0; l < 3; ++l) {
		r.m[l][0] = s.m[l][0] * o.m[0][0] + s.m[l][1] * o.m[1][0] + s.m[l][2] * o.m[2][0];
		r.m[l][1] = s.m[l][0] * o.m[0][1] + s.m[l][1] * o.m[1][1] + s.m[l][2] * o.m[2][1];
		r.m[l][2] = s.m[l][0] * o.m[0][2] + s.m[l][1] * o.m[1][2] + s.m[l][2] * o.m[2][2];
	}

	M33_Copy(r, s);
}


SDL_Point M33_Apply(M33& m, SDL_Point& p) {
	int nX = m.m[0][0] * p.x + m.m[0][1] * p.y + m.m[0][2];
	int nY = m.m[1][0] * p.x + m.m[1][1] * p.y + m.m[1][2];

	return SDL_Point{ nX, nY };
}

void M33_Print(M33& m) {
	cout << " [ " << m.m[0][0] << "\t" << m.m[0][1] << "\t" << m.m[0][2] << " ] " << endl;
	cout << " [ " << m.m[1][0] << "\t" << m.m[1][1] << "\t" << m.m[1][2] << " ] " << endl;
	cout << " [ " << m.m[2][0] << "\t" << m.m[2][1] << "\t" << m.m[2][2] << " ] " << endl;
}

int pX = 0;
int pY = 0;

void renderFrame() {

	if (SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE)) {
		cout << "SDL_SetRenderDrawColor: " << SDL_GetError() << endl;
	}

	if (SDL_RenderClear(renderer)) {
		cout << "SDL_RenderClear: " << SDL_GetError() << endl;
	}
	SDL_Rect rect;
	
	double angle = atan2(pY - 500, pX - 500);

	M33 base;	M33_Identity(base);
	M33 rotate; M33_Rotate(rotate, angle);
	M33 t;		M33_Translate(t, 500, 500);
	
	
	M33_Mult(base, t);
	M33_Mult(base, rotate);
	
	SDL_Point points[] = {
		{-50,-50}, {50,-5}, {50,5}, {-50,50}
	};
	
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);

	for (int i = 0; i < 4; i++) {
		SDL_Point from = M33_Apply(base, points[i]);
		SDL_Point to = M33_Apply(base, points[(i+1) % 4]);

		SDL_RenderDrawLine(renderer, from.x, from.y, to.x, to.y);
	}

	SDL_RenderPresent(renderer);
}



int main(int argc, char* argv[])
{
	// Initialize SDL. SDL_Init will return -1 if it fails.
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		cout << "Error initializing SDL: " << SDL_GetError() << endl;
		// End the program
		return 1;
	}

	globalCustomEventId = SDL_RegisterEvents(1);
	if ( ((Uint32)-1) == globalCustomEventId ) {
		cout << "Failed to get registered event.." << endl;
		return -1;
	}

	int registeredTimer = SDL_AddTimer(15, tickFrame, nullptr);
	cout << "Registered timer at: " << registeredTimer << endl;
	
	window = SDL_CreateWindow("Example", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, 0);

	if (nullptr == window) {
		cout << "Error creating window: " << SDL_GetError() << endl;
		return 1;
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	if (nullptr == renderer) {
		cout << "Error getting renderer: " << SDL_GetError() << endl;
		return 1;
	}
	
	renderFrame();
	int audio_rate = MIX_DEFAULT_FREQUENCY;
	Uint16 audio_format = MIX_DEFAULT_FORMAT;
	int audio_channels = MIX_DEFAULT_CHANNELS;

	if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, 4096)) {
		cout << "Mix_OpenAudio: " << SDL_GetError() << endl;
	}
	
	Mix_Chunk* loadedWav = Mix_LoadWAV("C:/samples/CantinaBand60.wav");
	if (nullptr == loadedWav) {
		cout << "Mix_LoadWAV: " << SDL_GetError() << endl;
	}
	else {
		Mix_QuerySpec(&audio_rate, &audio_format, &audio_channels);
		cout << "Mix_QuerySpec: " << audio_rate <<" "<< audio_format<<" "<< audio_channels << endl;
	}

	bool musicStarted = false;
	bool paused = false;

	SDL_Event e;
	for (;;) {
		if (SDL_WaitEvent(&e)) {
			if (e.type == SDL_MOUSEMOTION) {
				SDL_MouseMotionEvent* mouseMotionEvent = (SDL_MouseMotionEvent*)(&e);
				cout << "Mx: " << mouseMotionEvent->x << " My: " << mouseMotionEvent->y << " dMx: " << mouseMotionEvent->xrel << " dMy: " << " dMx: " <<  mouseMotionEvent->yrel << endl;
				pX = mouseMotionEvent->x;
				pY = mouseMotionEvent->y;
			}
			else if (e.type == SDL_KEYDOWN /*|| e.type == SDL_KEYUP*/) {
				SDL_KeyboardEvent* keyboardEvent = (SDL_KeyboardEvent*)(&e);
				if (keyboardEvent->keysym.scancode == SDL_SCANCODE_ESCAPE) {
					break;
				}

				if (keyboardEvent->keysym.scancode == SDL_SCANCODE_RETURN) {
					cout << "Return key pressed" << endl;

					if (!musicStarted) {
						Mix_PlayChannel(0, loadedWav, 0);
						musicStarted = 1;
					}
					else {
						if (paused) {
							Mix_Resume(0);
						}
						else {
							Mix_Pause(0);
						}

						paused = !paused;
					}
				}
				renderFrame();
			}
			else if (e.type == SDL_QUIT) {
				break;
			}
			else if (e.type == globalCustomEventId) {
				renderFrame();
			}
			else {
				cout << "Not handled event type: " << e.type << endl;
			}
		}
		else {
			cout << "SDL_WaitEvent: " << SDL_GetError() << endl;
			break;
		}
		
		
	}

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
