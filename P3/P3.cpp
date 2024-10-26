#include <iostream>
#include <cmath>
#include <SDL.h>
#include <SDL_mixer.h>
#include <vector>
#include <cstdlib>

using namespace std;

bool runNewGame = false;

struct DoublePoint {
	double x, y;
};

struct Shape {
	std::vector<DoublePoint> points;
	double range;
};

struct FloatingObject {
	double angle;
	DoublePoint pos;
	DoublePoint vel;
	Shape* shape;
};

struct Bullet {
	FloatingObject f;
	int lifetime;
};

struct Anim {
	FloatingObject f;
	int firstFrame;
	std::vector<Shape> frames;
	bool loop;
};

struct Asteroid {
	FloatingObject f;
	int level;
	double aVel;
};

Asteroid makeAsteroid(int lvl);
void freeAsteroidShape(Shape* s);

vector<Bullet> bullets;
vector<Asteroid> asteroids;
bool accelerates;
Anim flame;
Anim explosion;

bool playerAlive = true;

Uint32 globalCustomEventId = 0;

double SCREEN_WIDTH = 800;
double SCREEN_HEIGHT = 600;

int worldFrame = 0;
int nextFireFrame = -1;

DoublePoint directionVector;
double accScale = 0.05;
double bulletScale = 10;

void newGame();
void triggerFire();

typedef enum {
	Kill,
	Fire,
	Accelerate,
	ActionButtonCount

} ACTION_BUTTONS;

bool pressedButtons[ActionButtonCount] = {};

struct Player {
	FloatingObject f;
} player;

double vectorLength(DoublePoint& p) {
	return sqrt(p.x * p.x + p.y * p.y);
}

DoublePoint identityPoint(double x, double y) {
	double len = sqrt(x * x + y * y);
	return DoublePoint{ x / len, y / len };
}

DoublePoint identityPoint(DoublePoint p) {
	return identityPoint(p.x, p.y);
}

DoublePoint identityPointingTo(double x, double y) {
	return identityPoint(x- player.f.pos.x, y - player.f.pos.y);
}

double angleOfPoint(DoublePoint& p) {
	return atan2(p.y, p.x);
}

void scaleDoublePoint(DoublePoint& p, double s) {
	p.x *= s;
	p.y *= s;
}

DoublePoint addDoublePoints(DoublePoint& a, DoublePoint& b) {
	DoublePoint r;
	r.x = a.x + b.x;
	r.y = a.y + b.y;

	return r;
}

void applyMove(FloatingObject& f) {
	f.pos.x += f.vel.x;
	f.pos.y += f.vel.y;

	if (f.pos.x < 0) {
		f.pos.x += SCREEN_WIDTH;
	}
	else if (f.pos.x > SCREEN_WIDTH) {
		f.pos.x -= SCREEN_WIDTH;
	}

	if (f.pos.y < 0) {
		f.pos.y += SCREEN_HEIGHT;
	}
	else if (f.pos.y > SCREEN_HEIGHT) {
		f.pos.y -= SCREEN_HEIGHT;
	}
}

FloatingObject nextVectors[3] = {};

DoublePoint calcScreenOffsets(FloatingObject& f) {
	double xOffset = 0;
	double yOffset = 0;

	if (f.pos.x < f.shape->range) xOffset = SCREEN_WIDTH;
	else if (f.pos.x > SCREEN_WIDTH - f.shape->range) xOffset = -SCREEN_WIDTH;

	if (f.pos.y < f.shape->range) yOffset = SCREEN_HEIGHT;
	else if (f.pos.y > SCREEN_HEIGHT - f.shape->range) yOffset = -SCREEN_HEIGHT;

	DoublePoint p;
	p.x = xOffset;
	p.y = yOffset;

	return p;
}

bool floatingObjectsCollides(FloatingObject &a, FloatingObject &b) {
	DoublePoint p;
	p.x = a.pos.x - b.pos.x;
	p.y = a.pos.y - b.pos.y;

	double len = vectorLength(p);

	return len < a.shape->range || len < b.shape->range;
}

void fillWithOffsetObjects(vector<FloatingObject> &out, FloatingObject& a) {
	out.push_back(a);
	DoublePoint offset = calcScreenOffsets(a);
	FloatingObject variant;
	if (offset.x != 0) {
		variant = a;
		variant.pos.x += offset.x;

		out.push_back(variant);
	}

	if (offset.y != 0) {
		variant = a;
		variant.pos.y += offset.y;

		out.push_back(variant);
	}

	if (offset.x != 0 && offset.y != 0) {
		variant = a;
		variant.pos.x += offset.x;
		variant.pos.y += offset.y;

		out.push_back(variant);
	}
}

bool floatingObjectWithOffsetCollides(FloatingObject& a, FloatingObject& b) {
	vector<FloatingObject> aVariants;
	vector<FloatingObject> bVariants;
	
	fillWithOffsetObjects(aVariants, a);
	fillWithOffsetObjects(bVariants, b);

	for (auto aPtr = aVariants.begin(); aPtr < aVariants.end(); ++aPtr) {
		for (auto bPtr = bVariants.begin(); bPtr < bVariants.end(); ++bPtr) {
			if (floatingObjectsCollides(*aPtr, *bPtr)) {
				return true;
			}
		}
	}

	return false;
}

bool bulletCollides(Bullet& bullet, Asteroid& asteroid) {
	return floatingObjectWithOffsetCollides(bullet.f, asteroid.f);
}

void calculateBounceCollision(FloatingObject& a, FloatingObject& b) {
	if (floatingObjectWithOffsetCollides(a, b)) {
		// exchange velocities;
		DoublePoint v = a.vel;
		a.vel = b.vel;
		b.vel = v;
	}
}

void killPlayer() {
	playerAlive = false;
	explosion.firstFrame = worldFrame;
}

void calculateKillCollisions(FloatingObject& a, FloatingObject& b) {
	if (floatingObjectWithOffsetCollides(a, b)) {
		killPlayer();
	}
}


Uint32 tickFrame(Uint32 interval, void* param) {
	if (runNewGame) {
		newGame();
		runNewGame = false;
	}

	++worldFrame;

	SDL_Event e = {};
	e.type = globalCustomEventId;

	if (pressedButtons[Kill]) {
		killPlayer();
	}

	accelerates = pressedButtons[Accelerate];
	if (accelerates && playerAlive) {
		auto velVector = directionVector;
		scaleDoublePoint(velVector, accScale);
		player.f.vel.x += velVector.x;
		player.f.vel.y += velVector.y;
	}

	if (pressedButtons[Fire] && playerAlive && worldFrame > nextFireFrame) {
		nextFireFrame = worldFrame + 10;
		triggerFire();
	}

	applyMove(player.f);
	flame.f.pos = player.f.pos;
	flame.f.angle = player.f.angle;
	explosion.f.pos = player.f.pos;

	for (auto ptr = bullets.begin(); ptr < bullets.end(); ++ptr) {
		if (ptr->lifetime > 0) {
			--ptr->lifetime;
			applyMove(ptr->f);
		}
	}

	for (auto ptr = asteroids.begin(); ptr < asteroids.end(); ++ptr) {
		ptr->f.angle += ptr->aVel;
		applyMove(ptr->f);
	}

	auto lastToDrop = bullets.begin();

	for (; lastToDrop < bullets.end(); ++lastToDrop) {
		if (lastToDrop->lifetime > 0) {
			break;
		}
	}

	bullets.erase(bullets.begin(), lastToDrop);

	auto bPtr = bullets.begin();
	for (;bPtr<bullets.end();) {
		bool bulletHit = false;
		auto aPtr = asteroids.begin();
		for (; aPtr < asteroids.end(); ++aPtr) {
			if (bulletCollides(*bPtr, *aPtr)) {
				bulletHit = true;
				break;
			}
		}

		if (bulletHit) {
			bPtr = bullets.erase(bPtr);
			
			Asteroid old = *aPtr;
			freeAsteroidShape(aPtr->f.shape);
			asteroids.erase(aPtr);

			if (old.level > 1) {
				for (int i = 0; i < 3; i++) {
					Asteroid next = makeAsteroid(old.level - 1);
					next.f.pos = addDoublePoints(old.f.pos, nextVectors[i].pos);
					next.f.vel = addDoublePoints(old.f.vel, nextVectors[i].vel);
					asteroids.push_back(next);
				}
			}
		}
		else {
			++bPtr;
		}
	}

	if (playerAlive) {
		for (auto aPtr = asteroids.begin(); aPtr < asteroids.end(); ++aPtr) {
			calculateKillCollisions(aPtr->f, player.f);
		}
	}

	for (auto aPtr = asteroids.begin(); aPtr < asteroids.end(); ++aPtr) {
		for (auto bPtr = aPtr+1; bPtr < asteroids.end(); ++bPtr) {
			calculateBounceCollision(aPtr->f, bPtr->f);
		}
	}

	SDL_PushEvent( &e);
	return interval;
}

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

struct M33 {
	double m[3][3];
};

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

DoublePoint M33_Apply(M33& m, DoublePoint& p) {
	double nX = m.m[0][0] * p.x + m.m[0][1] * p.y + m.m[0][2];
	double nY = m.m[1][0] * p.x + m.m[1][1] * p.y + m.m[1][2];

	return DoublePoint{ nX, nY };
}

void M33_Print(M33& m) {
	cout << " [ " << m.m[0][0] << "\t" << m.m[0][1] << "\t" << m.m[0][2] << " ] " << endl;
	cout << " [ " << m.m[1][0] << "\t" << m.m[1][1] << "\t" << m.m[1][2] << " ] " << endl;
	cout << " [ " << m.m[2][0] << "\t" << m.m[2][1] << "\t" << m.m[2][2] << " ] " << endl;
}


Shape playerShape = { { {-10,-10}, {10,-2}, {10,2}, {-10,10} } };

Shape bulletShape = { { {-4,-2}, {4,-2}, {4,2}, {-4,2} } };

void renderShape(M33& toApply, Shape& shape) {
	for (int i = 0; i < shape.points.size(); i++) {
		DoublePoint from = M33_Apply(toApply, shape.points[i]);
		DoublePoint to = M33_Apply(toApply, shape.points[(i + 1) % shape.points.size()]);

		SDL_RenderDrawLine(renderer, (int)floor(from.x), (int)floor(from.y), (int)floor(to.x), (int)floor(to.y));
	}
}

void renderShapeWithOffset(Shape& s, M33& base, double offX, double offY) {
	M33 flipOffset;
	M33_Translate(flipOffset, offX, offY);

	M33_Mult(flipOffset, base);
	renderShape(flipOffset, s);
}

void renderShapeOfFloatingObject(FloatingObject& f) {
	M33 base;	M33_Identity(base);
	M33 rotate; M33_Rotate(rotate, f.angle);
	M33 t;		M33_Translate(t, f.pos.x, f.pos.y);
	
	M33_Mult(base, t);
	M33_Mult(base, rotate);

	renderShape(base, *f.shape);

	auto offset = calcScreenOffsets(f);

	if (offset.x != 0) {
		renderShapeWithOffset(*f.shape, base, offset.x, 0);
	}

	if (offset.y!= 0) {
		renderShapeWithOffset(*f.shape, base, 0, offset.y);
	}

	if (offset.x!= 0 && offset.y!= 0) {
		renderShapeWithOffset(*f.shape, base, offset.x, offset.y);
	}

}

const int animFrameLength = 5;
void renderAnim(Anim &anim) {
	int animFrame = (worldFrame - anim.firstFrame) / animFrameLength;
	if (anim.loop) {
		animFrame = animFrame % anim.frames.size();
	}

	if (animFrame < anim.frames.size()) {
		anim.f.shape = &anim.frames[animFrame];
		renderShapeOfFloatingObject(anim.f);
	}
}


void renderFrame() {
	if (SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE)) {
		cout << "SDL_SetRenderDrawColor: " << SDL_GetError() << endl;
	}

	if (SDL_RenderClear(renderer)) {
		cout << "SDL_RenderClear: " << SDL_GetError() << endl;
	}
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);

	if (!playerAlive) {
		renderAnim(explosion);
	}
	else {
		renderShapeOfFloatingObject(player.f);
		if (accelerates) {
			renderAnim(flame);
		}
	}

	for (auto ptr = bullets.begin(); ptr < bullets.end(); ++ptr) {
		if (ptr->lifetime > 0) {
			renderShapeOfFloatingObject(ptr->f);
		}
	}

	for (auto ptr = asteroids.begin(); ptr < asteroids.end(); ++ptr) {
		renderShapeOfFloatingObject(ptr->f);
	}

	SDL_RenderPresent(renderer);
}

void triggerFire() {
	Bullet b;
	b.lifetime = 100;
	b.f = player.f;
	b.f.shape = &bulletShape;

	auto velVector = directionVector;
	scaleDoublePoint(velVector, bulletScale);

	b.f.vel.x += velVector.x;
	b.f.vel.y += velVector.y;

	bullets.push_back(b);
}

void updateShapeRange(Shape& s) {
	double maxdist = 0;
	for (int i = 0; i < s.points.size(); i++) {
		maxdist = max<double>(maxdist, vectorLength(s.points[i]));
	}

	s.range = maxdist;
}

Shape* generateAsteroidShape(int count, int baseSize, int sizeRange) {
	Shape *s = new Shape;
	
	double angle = M_PI / count * 2;
	M33 rotate; 
	
	DoublePoint p;
	double maxX = 0, minX = 0, maxY = 0, minY = 0;
	for (int i=0;i<count;++i) {
		p.y = 0;
		p.x = baseSize + (rand() % sizeRange);
		M33_Rotate(rotate, angle*i);

		s->points.push_back(M33_Apply(rotate, p));
		maxX = max<double>(maxX, s->points[i].x);
		minX = min<double>(minX, s->points[i].x);
		maxY = max<double>(maxY, s->points[i].y);
		minY = min<double>(minY, s->points[i].y);
	}

	double offCenterX = (maxX + minX) / -2;
	double offCenterY = (maxY + minY) / -2;

	for (int i = 0; i < count; ++i) {
		s->points[i].x += offCenterX;
		s->points[i].y += offCenterY;
	}

	updateShapeRange(*s);

	return s;
}

void freeAsteroidShape(Shape* s) {
	delete s;
}

inline double drand() {
	return ((double)rand()) / RAND_MAX;
}

Asteroid makeAsteroid(int lvl) {
	int count = 7;
	int base = 5;
	int range = 5;

	if (3 == lvl) {
		count = 30;
		base = 30;
		range = 25;
	}
	else if (2 == lvl) {
		count = 20;
		base = 10;
		range = 15;
	}

	Shape* shape = generateAsteroidShape(count, base, range);

	Asteroid a;
	a.f.pos.x = drand() * SCREEN_WIDTH;
	a.f.pos.y = drand() * SCREEN_HEIGHT;
	a.f.angle = 0;
	a.f.shape = shape;
	a.f.vel.x = (0.04 * drand() - 0.02) * (4 - lvl);
	a.f.vel.y = (0.04 * drand() - 0.02) * (4 - lvl);

	a.aVel = 0.02 / lvl;
	a.level = lvl;

	return a;
}

void placeAsteroid(int lvl) {
	asteroids.push_back(makeAsteroid(lvl));
}

void initNextVectors() {
	nextVectors[0].pos = { -20,-10 }; 
	nextVectors[0].vel = { -0.8,-0.4 };

	nextVectors[1].pos = { 10,-10 };
	nextVectors[1].vel = { 0.4, -0.4 };

	nextVectors[2].pos = { 0, 15 };
	nextVectors[2].vel = { 0, 0.4 };
}

void initPlayer() {
	player.f.shape = &playerShape;
	
}

void initFlame() {
	flame.firstFrame = worldFrame;

	flame.frames.push_back({ {{ -12,-7}, {-12,7}, {-25,0} } });
	flame.frames.push_back({ {{ -12,-6}, {-12,6}, {-24,0} } });
	flame.frames.push_back({ {{ -12,-5}, {-12,5}, {-23,0} } });
	updateShapeRange(flame.frames[0]);
	updateShapeRange(flame.frames[1]);
	updateShapeRange(flame.frames[2]);

	flame.loop = true;
}

void initExplosion() {
	explosion.f.angle = 0;
	explosion.firstFrame = worldFrame;
	explosion.loop = false;

	for (int i = 0; i < 15; ++i) {
		Shape* shape = generateAsteroidShape(20, 5 + ((i % 5)*10), 20);
		explosion.frames.push_back(*shape);
		freeAsteroidShape(shape);
	}
	
}

void newGame() {
	worldFrame = 0;
	nextFireFrame = -1;
	playerAlive = true;
	player.f.pos = { 400, 300 };
	player.f.vel = { 0, 0 };
	player.f.angle = 0;

	directionVector = { 1,0 };

	for (auto ptr = asteroids.begin(); ptr < asteroids.end(); ++ptr) {
		freeAsteroidShape(ptr->f.shape);
	}

	asteroids.clear();
	bullets.clear();

	placeAsteroid(3);
	placeAsteroid(3);
	placeAsteroid(3);
	placeAsteroid(2);
	placeAsteroid(1);
}

int main(int argc, char* argv[])
{
	srand(0);
	updateShapeRange(playerShape);
	updateShapeRange(bulletShape);
	initNextVectors();

	initPlayer();

	initFlame();
	initExplosion();

	runNewGame = true;

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
	
	window = SDL_CreateWindow("Example", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, (int)SCREEN_WIDTH, (int)SCREEN_HEIGHT, 0);

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
				if (1 == mouseButtonEvent->button) {
					pressedButtons[Fire] = e.type == SDL_MOUSEBUTTONDOWN;
				}

				if (3 == mouseButtonEvent->button) {
					pressedButtons[Accelerate] = e.type == SDL_MOUSEBUTTONDOWN;
				}
			}

			else if (e.type == SDL_MOUSEMOTION) {
				SDL_MouseMotionEvent* mouseMotionEvent = (SDL_MouseMotionEvent*)(&e);

				directionVector = identityPointingTo(mouseMotionEvent->x, mouseMotionEvent->y);
				player.f.angle = angleOfPoint(directionVector);
			}
			else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
				SDL_KeyboardEvent* keyboardEvent = (SDL_KeyboardEvent*)(&e);
				auto key = keyboardEvent->keysym.scancode;
				if (key == SDL_SCANCODE_ESCAPE) {
					break;
				}
				else if (key == SDL_SCANCODE_RETURN) {
					pressedButtons[Kill] = e.type == SDL_KEYDOWN;
				}
				else if (key == SDL_SCANCODE_SPACE) {
					runNewGame = true;
				}
				
				if (false ) {
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
				
			}
			else if (e.type == SDL_QUIT) {
				break;
			}
			else if (e.type == globalCustomEventId) {
				renderFrame();
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

