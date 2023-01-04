#ifndef CUPS_H
#define CUPS_H

#include "Object.H"
#include <glm/vec3.hpp>

class Cups : public Object {
public:
	std::vector<glm::vec3> cup_track;
	int cups_pos_index[5];
	glm::vec3 cup_center;

	Cups(std::string vert_path, std::string frag_path, std::string obj_path, std::string mtl_path);
};

#endif // !CUPS_H