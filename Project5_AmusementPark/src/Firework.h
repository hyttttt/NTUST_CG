#ifndef FIREWORK_H
#define FIREWORK_H

#include"particle.h" 

class Firework {
public:
	emitter* e1;
	emitter* e2;
	emitter* e3;

	Firework();

	void drawScene();
};

#endif // !FIREWORK_H