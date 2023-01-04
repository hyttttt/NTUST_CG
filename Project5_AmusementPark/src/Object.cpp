#include "Object.H"

Object::Object(std::string vert_path, std::string frag_path, std::string obj_path, std::string mtl_path) {
	this->PATH_vertex_shader = vert_path;
	this->PATH_fragment_shader = frag_path;
	this->PATH_obj = obj_path;
	this->PATH_mtl = mtl_path;
}