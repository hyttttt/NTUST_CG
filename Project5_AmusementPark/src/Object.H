#ifndef OBJECT_H
#define OBJECT_H

#include "Drawable.h"
#include <glm/vec3.hpp>

class Object {
public:
	Drawable* myDraw = nullptr;
	GLfloat* vertex;
	GLfloat* texture_coordinate;
	GLfloat* normal;
	unsigned int* element;
	glm::vec3 light_pos;

	std::string PATH_vertex_shader;
	std::string PATH_fragment_shader;
	std::string PATH_obj;
	std::string PATH_mtl;

	Object(std::string vert_path, std::string frag_path, std::string obj_path, std::string mtl_path);
};

#endif // !OBJECT_H