#include "Firework.h"

particle* init_particle1()
{
	float size = rand() % 15 * 0.02f;
	unsigned char color[] = { 1.0f,0.0f,0.0f };
	particle* p = new particle(size, size, float(rand() % 10 - 4) / 80, float(rand() % 10 - 4) / 80,
		0, -4.9 / 40000, rand() % 200, 0, color, false);
	return p;
}
particle* init_particle2()
{
	float size = rand() % 15 * 0.02f;
	unsigned char color[] = { 0.0f,1.0f,0.0f };
	particle* p = new particle(size, size, float(rand() % 10 - 4) / 80, float(rand() % 10 - 4) / 80,
		0, -4.9 / 40000, rand() % 100, 0, color, false);
	return p;
}
particle* init_particle3()
{
	float size = rand() % 15 * 0.02f;
	unsigned char color[] = { 0.0f,0.0f,1.0f };
	particle* p = new particle(size, size, float(rand() % 10 - 4) / 80, float(rand() % 10 - 4) / 80,
		0, -4.9 / 40000, rand() % 80, 0, color, false);
	return p;
}

Firework::Firework() {
	this->e1 = new emitter(8000, 0, 0, 100, 100);
	this->e1->emit(init_particle1);

	this->e2 = new emitter(5000, 50, 50, 50, 50);
	this->e2->emit(init_particle2);

	this->e3 = new emitter(500, -20, -20, 70, 70);
	this->e3->emit(init_particle3);
}

void Firework::drawScene() {
	this->e1->update();
	this->e2->update();
	this->e3->update();
}
