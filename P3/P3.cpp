// P3.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <cmath>
#include <SDL.h>
#include <SDL_mixer.h>
#include <vector>

using namespace std;

struct Shape {
	SDL_Point *points;
	int pointsCount;
};

struct DoublePoint {
	double x,y;
};

struct FloatingObject {
	double angle;
	struct DoublePoint pos;
	struct DoublePoint vel;
};

struct Bullet {
	struct FloatingObject f;
	int lifetime;
};

vector<struct Bullet> bullets;

Uint32 globalCustomEventId = 0;

int worldFrame = 0;

double cursorX = 0;
double cursorY = 0;
double accScale = 0.05;
double bulletScale = 10;

typedef enum {
	MoveLeft = 0,
	MoveRight,
	MoveUp,
	MoveDown,
	Accelerate,
	ActionButtonCount

} ACTION_BUTTONS;

bool pressedButtons[ActionButtonCount] = {};

struct Player {
	struct FloatingObject f;
} player;



DoublePoint identityPoint(double x, double y) {
	double len = sqrt(x * x + y * y);
	return DoublePoint{ x / len, y / len };
}


DoublePoint identityPoint(DoublePoint p) {
	return identityPoint(p.x, p.y);
}

DoublePoint identityPointingToCursor() {
	return identityPoint(cursorX - player.f.pos.x, cursorY - player.f.pos.y);
}

double angleOfPoint(DoublePoint& p) {
	return atan2(p.y, p.x);
}

void scaleDoublePoint(DoublePoint& p, double s) {
	p.x *= s;
	p.y *= s;
}

void applyMove(FloatingObject& f) {
	f.pos.x += f.vel.x;
	f.pos.y += f.vel.y;
}

Uint32 tickFrame(Uint32 interval, void* param) {
	++worldFrame;
	if (worldFrame % 30 == 0) {
		cout << "FRAME: " << worldFrame << endl;
	}
	
	SDL_Event e = {};
	e.type = globalCustomEventId;

	if (pressedButtons[Accelerate]) {
		auto velVector = identityPointingToCursor();
		scaleDoublePoint(velVector, accScale);
		player.f.vel.x += velVector.x;
		player.f.vel.y += velVector.y;
	}

	applyMove(player.f);

	for (auto ptr = bullets.begin(); ptr < bullets.end(); ++ptr) {
		if (ptr->lifetime > 0) {
			--ptr->lifetime;
			applyMove(ptr->f);
		}
	}

	auto lastToDrop = bullets.begin();

	for (; lastToDrop < bullets.end(); ++lastToDrop) {
		if (lastToDrop->lifetime > 0) {
			break;
		}
	}

	bullets.erase(bullets.begin(), lastToDrop);


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

SDL_Point playerShapePoints[] = {
	{-10,-10}, {10,-2}, {10,2}, {-10,10}
};

struct Shape playerShape = {
	playerShapePoints, 4
};

SDL_Point bulletShapePoints[] = {
	{-4,-2}, {4,-2}, {4,2}, {-4,2}
};

struct Shape bulletShape = {
	bulletShapePoints, 4
};

void renderShape(M33& toApply, struct Shape& shape) {
	for (int i = 0; i < shape.pointsCount; i++) {
		SDL_Point from = M33_Apply(toApply, shape.points[i]);
		SDL_Point to = M33_Apply(toApply, shape.points[(i + 1) % shape.pointsCount]);

		SDL_RenderDrawLine(renderer, from.x, from.y, to.x, to.y);
	}
}

void renderShapeOfFloatingObject(struct FloatingObject& f, struct Shape& shape) {
	M33 base;	M33_Identity(base);
	M33 rotate; M33_Rotate(rotate, f.angle);
	M33 t;		M33_Translate(t, f.pos.x, f.pos.y);

	M33_Mult(base, t);
	M33_Mult(base, rotate);

	renderShape(base, shape);
}

void renderFrame() {
	if (SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE)) {
		cout << "SDL_SetRenderDrawColor: " << SDL_GetError() << endl;
	}

	if (SDL_RenderClear(renderer)) {
		cout << "SDL_RenderClear: " << SDL_GetError() << endl;
	}
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);

	auto directionVector = identityPointingToCursor();
	player.f.angle = angleOfPoint(directionVector);

	renderShapeOfFloatingObject(player.f, playerShape);

	for (auto ptr = bullets.begin(); ptr < bullets.end(); ++ptr) {
		if (ptr->lifetime > 0) {
			renderShapeOfFloatingObject(ptr->f, bulletShape);
		}
	}

	SDL_RenderPresent(renderer);
}

void triggerFire() {
	struct Bullet b;
	b.lifetime = 200;
	b.f = player.f;
	
	auto directionVector = identityPointingToCursor();
	scaleDoublePoint(directionVector, bulletScale);

	b.f.vel.x += directionVector.x;
	b.f.vel.y += directionVector.y;


	bullets.push_back(b);
}

int main(int argc, char* argv[])
{
	player.f.pos.x = 500;
	player.f.pos.y = 500;

	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		cout << "Error initializing SDL: " << SDL_GetError() << endl;
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
			if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
				SDL_MouseButtonEvent* mouseButtonEvent= (SDL_MouseButtonEvent*)(&e);
				if (1 == mouseButtonEvent->button && e.type == SDL_MOUSEBUTTONDOWN) {
					triggerFire();
				}

				if (3 == mouseButtonEvent->button) {
					pressedButtons[Accelerate] = e.type == SDL_MOUSEBUTTONDOWN;
				}
			}

			else if (e.type == SDL_MOUSEMOTION) {
				SDL_MouseMotionEvent* mouseMotionEvent = (SDL_MouseMotionEvent*)(&e);
				cout << "Mx: " << mouseMotionEvent->x << " My: " << mouseMotionEvent->y << " dMx: " << mouseMotionEvent->xrel << " dMy: " << " dMx: " <<  mouseMotionEvent->yrel << endl;
				cursorX = mouseMotionEvent->x;
				cursorY = mouseMotionEvent->y;
			}
			else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
				SDL_KeyboardEvent* keyboardEvent = (SDL_KeyboardEvent*)(&e);
				auto key = keyboardEvent->keysym.scancode;
				if (key == SDL_SCANCODE_ESCAPE) {
					break;
				}

				if (key == SDL_SCANCODE_RETURN && e.type == SDL_KEYDOWN) {
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
				else if (key == SDL_SCANCODE_LEFT) {
					pressedButtons[MoveLeft] = e.type == SDL_KEYDOWN;
				}
				else if (key == SDL_SCANCODE_RIGHT) {
					pressedButtons[MoveRight] = e.type == SDL_KEYDOWN;
				}
				else if (key == SDL_SCANCODE_UP) {
					pressedButtons[MoveUp] = e.type == SDL_KEYDOWN;
				}
				else if (key == SDL_SCANCODE_DOWN) {
					pressedButtons[MoveDown] = e.type == SDL_KEYDOWN;
				}
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
	SDL_Quit();

	return 0;
}

