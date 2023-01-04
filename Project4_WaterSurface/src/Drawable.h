#pragma once

#include "RenderUtilities/Shader.h"

class Drawable {
public:

	Shader* shader = nullptr;

	GLuint vao;
	GLuint vbo[4];	// position, normal, texture, color
	GLuint ebo;
	unsigned int element_amount;

	GLuint fbo;
	GLuint textures[4];	//attach to color buffer
	GLuint rbo;
};