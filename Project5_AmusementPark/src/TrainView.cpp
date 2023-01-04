/************************************************************************
     File:        TrainView.cpp

     Author:     
                  Michael Gleicher, gleicher@cs.wisc.edu

     Modifier
                  Yu-Chi Lai, yu-chi@cs.wisc.edu
     
     Comment:     
						The TrainView is the window that actually shows the 
						train. Its a
						GL display canvas (Fl_Gl_Window).  It is held within 
						a TrainWindow
						that is the outer window with all the widgets. 
						The TrainView needs 
						to be aware of the window - since it might need to 
						check the widgets to see how to draw

	  Note:        we need to have pointers to this, but maybe not know 
						about it (beware circular references)

     Platform:    Visio Studio.Net 2003/2005

*************************************************************************/

#include <iostream>
#include <direct.h>
#include <sstream>
#include <string.h>
#include <Fl/fl.h>

// we will need OpenGL, and OpenGL needs windows.h
#include <windows.h>
//#include "GL/gl.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>
#include <opencv2\opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <GL/glu.h>

#include "TrainView.H"
#include "TrainWindow.H"
#include "Utilities/3DUtils.H"

#define STB_IMAGE_IMPLEMENTATION
#include "stb-master/stb_image.h"



#ifdef EXAMPLE_SOLUTION
#	include "TrainExample/TrainExample.H"
#endif


//************************************************************************
//
// * Constructor to set up the GL window
//========================================================================
TrainView::
TrainView(int x, int y, int w, int h, const char* l) 
	: Fl_Gl_Window(x,y,w,h,l)
//========================================================================
{
	mode( FL_RGB|FL_ALPHA|FL_DOUBLE | FL_STENCIL );
	this->start_time = std::chrono::steady_clock::now();

	char* buffer;
	buffer = getcwd(NULL, 0);;
	std::vector<std::string> tokens = split(buffer, '\\');

	this->baseDIR = "";
	for (int i = 0; i < tokens.size(); i++) {
		if (i != tokens.size() - 1) {
			this->baseDIR += tokens[i];
		}

		if (i < tokens.size() - 2) {
			this->baseDIR += "/";
		}
	}
	//std::cout << "base directory: " << this->baseDIR << std::endl;

	resetArcball();
}

//************************************************************************
//
// * Reset the camera to look at the world
//========================================================================
void TrainView::
resetArcball()
//========================================================================
{
	// Set up the camera to look at the world
	// these parameters might seem magical, and they kindof are
	// a little trial and error goes a long way
	arcball.setup(this, 40, 250, .2f, .4f, 0);
}

//************************************************************************
//
// * FlTk Event handler for the window
//########################################################################
// TODO: 
//       if you want to make the train respond to other events 
//       (like key presses), you might want to hack this.
//########################################################################
//========================================================================
int TrainView::handle(int event)
{
	// see if the ArcBall will handle the event - if it does, 
	// then we're done
	// note: the arcball only gets the event if we're in world view
	if (tw->worldCam->value())
		if (arcball.handle(event)) 
			return 1;

	// remember what button was used
	static int last_push;

	switch(event) {
		// Mouse button being pushed event
		case FL_PUSH:
			last_push = Fl::event_button();
			// if the left button be pushed is left mouse button
			if (last_push == FL_LEFT_MOUSE  ) {
				doPick();
				damage(1);
				return 1;
			};
			break;

	   // Mouse button release event
		case FL_RELEASE: // button release
			damage(1);
			last_push = 0;
			return 1;

		// Mouse button drag event
		case FL_DRAG:

			// Compute the new control point position
			if ((last_push == FL_LEFT_MOUSE) && (selectedCube >= 0)) {
				ControlPoint* cp = &m_pTrack->points[selectedCube];

				double r1x, r1y, r1z, r2x, r2y, r2z;
				getMouseLine(r1x, r1y, r1z, r2x, r2y, r2z);

				double rx, ry, rz;
				mousePoleGo(r1x, r1y, r1z, r2x, r2y, r2z, 
								static_cast<double>(cp->pos.x), 
								static_cast<double>(cp->pos.y),
								static_cast<double>(cp->pos.z),
								rx, ry, rz,
								(Fl::event_state() & FL_CTRL) != 0);

				cp->pos.x = (float) rx;
				cp->pos.y = (float) ry;
				cp->pos.z = (float) rz;
				damage(1);
			}
			break;

		// in order to get keyboard events, we need to accept focus
		case FL_FOCUS:
			return 1;

		// every time the mouse enters this window, aggressively take focus
		case FL_ENTER:	
			focus(this);
			break;

		case FL_KEYBOARD:
		 		int k = Fl::event_key();
				int ks = Fl::event_state();
				if (k == 'p') {
					// Print out the selected control point information
					if (selectedCube >= 0) 
						printf("Selected(%d) (%g %g %g) (%g %g %g)\n",
								 selectedCube,
								 m_pTrack->points[selectedCube].pos.x,
								 m_pTrack->points[selectedCube].pos.y,
								 m_pTrack->points[selectedCube].pos.z,
								 m_pTrack->points[selectedCube].orient.x,
								 m_pTrack->points[selectedCube].orient.y,
								 m_pTrack->points[selectedCube].orient.z);
					else
						printf("Nothing Selected\n");

					return 1;
				};
				break;
	}

	return Fl_Gl_Window::handle(event);
}

void TrainView::drawFlag() {
	//bind shader
	this->flag->shader->Use();

	// model
	glm::mat4 model_matrix = glm::mat4();
	model_matrix = glm::translate(model_matrix, this->source_pos);
	model_matrix = glm::scale(model_matrix, glm::vec3(10.0f, 10.0f, 10.0f));
	glUniformMatrix4fv(glGetUniformLocation(this->flag->shader->Program, "u_model"), 1, GL_FALSE, &model_matrix[0][0]);
	glUniform1f(glGetUniformLocation(this->flag->shader->Program, "time"), this->duration_time);

	//bind VAO
	glBindVertexArray(this->flag->vao);

	// draw water surface
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LIGHTING);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, this->skybox->textures[0]);
	glUniform1i(glGetUniformLocation(this->skybox->shader->Program, "skybox"), 0);

	glDrawElements(GL_TRIANGLES, this->flag->element_amount, GL_UNSIGNED_INT, 0);

	glDisable(GL_BLEND);

	//unbind VAO
	glBindVertexArray(0);

	//unbind shader(switch to fixed pipeline)
	glUseProgram(0);
}

void TrainView::drawSkybox() {
	glDepthFunc(GL_LEQUAL);

	//bind shader
	this->skybox->shader->Use();

	// view matrix
	glm::mat4 view_matrix;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
	view_matrix = glm::mat4(glm::mat3(view_matrix)); // get rid of last row & col (which affect translation)
	glUniformMatrix4fv(glGetUniformLocation(this->skybox->shader->Program, "view"), 1, GL_FALSE, glm::value_ptr(view_matrix));
	
	// perspective matrix
	glm::mat4 projection_matrix;
	glGetFloatv(GL_PROJECTION_MATRIX, &projection_matrix[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(this->skybox->shader->Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection_matrix));

	//bind VAO
	glBindVertexArray(this->skybox->vao);

	// draw
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, this->skybox->textures[0]);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	//unbind VAO
	glBindVertexArray(0);

	glDepthFunc(GL_LESS);

	//unbind shader(switch to fixed pipeline)
	glUseProgram(0);
}

void TrainView::drawStreetlight(glm::vec3 scale_val, glm::vec3 trans_pos) {
	/* draw streetlight itself */ 
	drawObject(*this->streetLight, scale_val, trans_pos, false, glm::vec4(0.0, 0.0, 0.0, 1.0), this->duration_time);

	/* draw fake lighting */
	glm::vec3 scale_light(scale_val.x * 12.0, scale_val.y * 12.0, scale_val.z * 12.0);
	glm::vec4 color_light(1.0, 0.7, 0.0, 0.7);
	drawObject(*this->light, scale_light, trans_pos + glm::vec3(0, 36.0, 5.5), true, color_light, this->duration_time);
	drawObject(*this->light, scale_light, trans_pos + glm::vec3(0, 36.0, -5.5), true, color_light, this->duration_time);
}

void TrainView::drawCups(glm::vec3 scale_val) {
	glm::vec3 p0 = this->cups->cup_track[this->cups->cups_pos_index[0]];
	glm::vec3 p1 = this->cups->cup_track[this->cups->cups_pos_index[1]];
	glm::vec3 p2 = this->cups->cup_track[this->cups->cups_pos_index[2]];
	glm::vec3 p3 = this->cups->cup_track[this->cups->cups_pos_index[3]];
	glm::vec3 p4 = this->cups->cup_track[this->cups->cups_pos_index[4]];

	drawObject(*this->cups, scale_val, p0, false, glm::vec4(1.0, 0.8, 1.0, 1.0), this->duration_time);
	drawObject(*this->cups, scale_val, p1, false, glm::vec4(1.0, 1.0, 0.8, 1.0), this->duration_time + 10);
	drawObject(*this->cups, scale_val, p2, false, glm::vec4(1.0, 0.8, 1.0, 1.0), this->duration_time + 20);
	drawObject(*this->cups, scale_val, p3, false, glm::vec4(0.8, 1.0, 1.0, 1.0), this->duration_time + 30);
	drawObject(*this->cups, scale_val, p4, false, glm::vec4(1.0, 1.0, 0.8, 1.0), this->duration_time + 40);
	drawObject(*this->cups, scale_val, this->cups->cup_center, false, glm::vec4(0.8, 0.8, 1.0, 1.0), this->duration_time + 50);

	glm::vec3 stage_pos = this->cups->cup_center - glm::vec3(0, 0, 0);
	drawObject(*this->cup_stage, scale_val, stage_pos, false, glm::vec4(0.9, 0.9, 0.9, 1.0), this->duration_time);

	this->cups->cups_pos_index[0] = (this->cups->cups_pos_index[0] + 1) % this->cups->cup_track.size();
	this->cups->cups_pos_index[1] = (this->cups->cups_pos_index[1] + 1) % this->cups->cup_track.size();
	this->cups->cups_pos_index[2] = (this->cups->cups_pos_index[2] + 1) % this->cups->cup_track.size();
	this->cups->cups_pos_index[3] = (this->cups->cups_pos_index[3] + 1) % this->cups->cup_track.size();
	this->cups->cups_pos_index[4] = (this->cups->cups_pos_index[4] + 1) % this->cups->cup_track.size();
}

void TrainView::drawObject(Object obj, glm::vec3 scale_val, glm::vec3 trans_pos, bool transparency, glm::vec4 color, float time) {
	if (obj.myDraw != nullptr) {
		//bind shader
		obj.myDraw->shader->Use();

		// model
		glm::mat4 model_matrix = glm::mat4();
		model_matrix = glm::translate(model_matrix, trans_pos);
		model_matrix = glm::scale(model_matrix, scale_val);
		glUniformMatrix4fv(glGetUniformLocation(obj.myDraw->shader->Program, "u_model"), 1, GL_FALSE, &model_matrix[0][0]);

		// view matrix
		glm::mat4 view_matrix;
		glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(obj.myDraw->shader->Program, "view"), 1, GL_FALSE, &view_matrix[0][0]);

		// perspective matrix
		glm::mat4 projection_matrix;
		glGetFloatv(GL_PROJECTION_MATRIX, &projection_matrix[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(obj.myDraw->shader->Program, "projection"), 1, GL_FALSE, &projection_matrix[0][0]);

		// time
		glUniform1f(glGetUniformLocation(obj.myDraw->shader->Program, "time"), time);

		// color
		glUniform4fv(glGetUniformLocation(obj.myDraw->shader->Program, "u_color"), 1, &color[0]);

		// light position
		glUniform3fv(glGetUniformLocation(obj.myDraw->shader->Program, "light_pos"), 1, &obj.light_pos[0]);

		// texture
		this->texture->bind(0);
		glUniform1i(glGetUniformLocation(obj.myDraw->shader->Program, "u_texture"), 0);

		if (transparency) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_LIGHTING);
		}

		//bind VAO
		glBindVertexArray(obj.myDraw->vao);

		// draw
		if (obj.vertex != nullptr) {
			glBegin(GL_TRIANGLES);
			for (int i = 0; i < obj.myDraw->element_amount; i++) {
				glNormal3f(obj.normal[i * 3], obj.normal[i * 3 + 1], obj.normal[i * 3 + 2]);
				glTexCoord2f(obj.texture_coordinate[i * 2], obj.texture_coordinate[i * 2 + 1]);
				glVertex3f(obj.vertex[i * 3], obj.vertex[i * 3 + 1], obj.vertex[i * 3 + 2]);
			}
			glEnd();
		}

		if (transparency) {
			glDisable(GL_BLEND);
		}

		//unbind VAO
		glBindVertexArray(0);

		//unbind shader(switch to fixed pipeline)
		glUseProgram(0);
	}
}

void TrainView::drawShops() {
	glm::vec4 house_color(0.5, 0.3, 0.0, 1.0);
	this->house->light_pos = glm::vec3(-5.0f, 8.0f, 0.0f);
	drawObject(*this->house, glm::vec3(70.0, 70.0, 70.0), glm::vec3(-140.0, 0.0, -100.0), false, house_color, this->duration_time);
	drawObject(*this->house, glm::vec3(70.0, 70.0, 70.0), glm::vec3(-140.0, 0.0, -30.0), false, house_color, this->duration_time);
	drawObject(*this->house, glm::vec3(70.0, 70.0, 70.0), glm::vec3(-140.0, 0.0, 40.0), false, house_color, this->duration_time);
	drawObject(*this->house, glm::vec3(70.0, 70.0, 70.0), glm::vec3(-140.0, 0.0, 110.0), false, house_color, this->duration_time);

	//drawStreetlight(glm::vec3(0.1f, 0.1f, 0.1f), glm::vec3(-110.0, 0.0, -150.0));
	drawStreetlight(glm::vec3(0.1f, 0.1f, 0.1f), glm::vec3(-110.0, 0.0, -60.0));
	drawStreetlight(glm::vec3(0.1f, 0.1f, 0.1f), glm::vec3(-110.0, 0.0, 10.0));
	drawStreetlight(glm::vec3(0.1f, 0.1f, 0.1f), glm::vec3(-110.0, 0.0, 80.0));
	drawStreetlight(glm::vec3(0.1f, 0.1f, 0.1f), glm::vec3(-110.0, 0.0, 150.0));
}

// split string
const std::vector<std::string> TrainView::split(const std::string& str, const char& delimiter) {
	std::vector<std::string> result;
	std::stringstream ss(str);
	std::string tok;

	while (std::getline(ss, tok, delimiter)) {
		result.push_back(tok);
	}
	return result;
}

// load .obj file (3d model)
// refer to: https://blog.csdn.net/MASILEJFOAISEGJIAE/article/details/85639958
bool TrainView::loadOBJ(
	const char* path,
	std::vector<glm::vec3>& out_vertices,
	std::vector<glm::vec2>& out_uvs,
	std::vector<glm::vec3>& out_normals)
{
	printf("Loading OBJ file %s...\n", path);

	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	std::vector<glm::vec3> temp_vertices;
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;

	FILE* file = fopen(path, "r");
	if (file == NULL)
	{
		printf("Impossible to open the file ! Are you in the right path ? See Tutorial 1 for details\n");
		getchar();
		return false;
	}

	while (1)
	{

		char lineHeader[128];
		// read the first word of the line
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF)
			break; // EOF = End Of File. Quit the loop.

		// else : parse lineHeader

		if (strcmp(lineHeader, "v") == 0)
		{
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}
		else if (strcmp(lineHeader, "vt") == 0)
		{
			glm::vec2 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y);
			uv.y = -uv.y; // Invert V coordinate since we will only use DDS texture, which are inverted. Remove if you want to use TGA or BMP loaders.
			temp_uvs.push_back(uv);
		}
		else if (strcmp(lineHeader, "vn") == 0)
		{
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}
		else if (strcmp(lineHeader, "f") == 0)
		{
			char charBuffer[1000];
			fgets(charBuffer, 1000, file);
			std::string line = charBuffer;
			std::vector<int> vs;
			std::vector<int> vts;
			std::vector<int> vns;
			int count = 0;

			std::vector<std::string> buf = split(line, '\n');
			line = buf[0];
			
			std::vector<std::string> groups = split(line, ' ');
			for (auto group : groups) {
				if (!group.empty()) {
					std::vector<std::string> item = split(group, '/');

					int v = stoi(item[0]);					
					int vt = item[1].empty() ? 0 : stoi(item[1]);
					int vn = item[2].empty() ? 0 : stoi(item[2]);

					vs.push_back(v);
					vts.push_back(vt);
					vns.push_back(vn);

					count++;
				}
			}

			// push new vertices when there are only 3 of them
			if (count == 3) {
				for (int i = 0; i < 3; i++) {
					vertexIndices.push_back(vs[i]);
					uvIndices.push_back(vts[i]);
					normalIndices.push_back(vns[i]);
				}
			}
			// cut the face into triangles when it is not triangle
			else {
				glm::vec2 zero2(0.0f, 0.0f);
				glm::vec3 zero3(0.0f, 0.0f, 0.0f);

				// push vertices
				for (int i = 0; i < vs.size() - 2; i++) {

					// first vertex
					// Get the attributes thanks to the index
					glm::vec3 vertex = temp_vertices[vs[0] - 1];
					glm::vec2 uv = (vts[0] - 1 < 0) ? zero2 : temp_uvs[vts[0] - 1];
					glm::vec3 normal = (vns[0] - 1 < 0) ? zero3 : temp_normals[vns[0] - 1];

					// Put the attributes in buffers
					out_vertices.push_back(vertex);
					out_uvs.push_back(uv);
					out_normals.push_back(normal);

					// second vertex
					// Get the attributes thanks to the index
					vertex = temp_vertices[vs[i + 1] - 1];
					uv = (vts[i] - 1 < 0) ? zero2 : temp_uvs[vts[i + 1] - 1];
					normal = (vns[i] - 1 < 0) ? zero3 : temp_normals[vns[i + 1] - 1];

					// Put the attributes in buffers
					out_vertices.push_back(vertex);
					out_uvs.push_back(uv);
					out_normals.push_back(normal);

					// third vertex
					// Get the attributes thanks to the index
					vertex = temp_vertices[vs[i + 2] - 1];
					uv = (vts[i] - 1 < 0) ? zero2 : temp_uvs[vts[i + 2] - 1];
					normal = (vns[i] - 1 < 0) ? zero3 : temp_normals[vns[i + 2] - 1];

					// Put the attributes in buffers
					out_vertices.push_back(vertex);
					out_uvs.push_back(uv);
					out_normals.push_back(normal);
				}
			}			
		}
		else
		{
			// Probably a comment, eat up the rest of the line
			char stupidBuffer[1000];
			fgets(stupidBuffer, 1000, file);
		}
	}
	fclose(file);

	printf("Finish loading OBJ file %s...\n", path);
	return true;
}

void TrainView::setObject(Object& obj) {
	// set up shader
	obj.myDraw = new Drawable;
	obj.myDraw->shader = new
		Shader(
			obj.PATH_vertex_shader.c_str(),
			nullptr, nullptr, nullptr,
			obj.PATH_fragment_shader.c_str());

	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;

	// load .obj file
	loadOBJ(
		obj.PATH_obj.c_str(),
		vertices,
		uvs,
		normals
	);

	// vertex
	obj.vertex = new GLfloat[vertices.size() * 3];
	for (int i = 0; i < vertices.size(); i++) {
		obj.vertex[i * 3] = vertices[i].x;
		obj.vertex[i * 3 + 1] = vertices[i].y;
		obj.vertex[i * 3 + 2] = vertices[i].z;
	}
	obj.myDraw->element_amount = vertices.size();

	// texture coordinates
	obj.texture_coordinate = new GLfloat[uvs.size() * 2];
	for (int i = 0; i < uvs.size(); i++) {
		obj.texture_coordinate[i * 2] = uvs[i].x;
		obj.texture_coordinate[i * 2 + 1] = uvs[i].y;
	}

	// normal
	obj.normal = new GLfloat[normals.size() * 3];
	for (int i = 0; i < normals.size(); i++) {
		obj.normal[i * 3] = normals[i].x;
		obj.normal[i * 3 + 1] = normals[i].y;
		obj.normal[i * 3 + 2] = normals[i].z;
	}

	// element
	obj.element = new unsigned int[vertices.size()];
	for (int i = 0; i < vertices.size(); i++) {
		obj.element[i] = i;
	}

	// VAO
	glGenVertexArrays(1, &obj.myDraw->vao);
	glBindVertexArray(obj.myDraw->vao);
	glGenBuffers(1, &obj.myDraw->ebo);

	// VBOs
	glGenBuffers(4, obj.myDraw->vbo);

	// position
	glBindBuffer(GL_ARRAY_BUFFER, obj.myDraw->vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(obj.vertex), obj.vertex, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	// normal
	glBindBuffer(GL_ARRAY_BUFFER, obj.myDraw->vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(obj.normal), obj.normal, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(1);

	// texture Coordinate
	glBindBuffer(GL_ARRAY_BUFFER, obj.myDraw->vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(obj.texture_coordinate), obj.texture_coordinate, GL_STATIC_DRAW);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(2);

	// ebo
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.myDraw->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(obj.element), &obj.element, GL_STATIC_DRAW);

	// Unbind VAO
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}


//************************************************************************
//
// * this is the code that actually draws the window
//   it puts a lot of the work into other routines to simplify things
//========================================================================
void TrainView::draw()
{

	//*********************************************************************
	//
	// * Set up basic opengl informaiton
	//
	//**********************************************************************
	//initialized glad
	if (gladLoadGL())
	{
		// refer to: https://github.com/VictorGordan/opengl-tutorials/blob/main/YoutubeOpenGL%2019%20-%20Cubemaps%20%26%20Skyboxes/Main.cpp
		if (!this->skybox) {
			this->skybox = new Drawable;

			// shader
			this->skybox->shader = new
				Shader(
					(this->baseDIR + "/src/shaders/skybox.vert").c_str(),
					nullptr, nullptr, nullptr,
					(this->baseDIR + "/src/shaders/skybox.frag").c_str());
		
			// data
			vector<std::string> faces {
				(this->baseDIR + "/Images/skybox/1.png").c_str(),
				(this->baseDIR + "/Images/skybox/2.png").c_str(),
				(this->baseDIR + "/Images/skybox/3.png").c_str(),
				(this->baseDIR + "/Images/skybox/4.png").c_str(),
				(this->baseDIR + "/Images/skybox/5.png").c_str(),
				(this->baseDIR + "/Images/skybox/6.png").c_str()
			};

			float skyboxVertices[] = {
				//   Coordinates
				-1.0f, -1.0f,  1.0f,//        7--------6
				 1.0f, -1.0f,  1.0f,//       /|       /|
				 1.0f, -1.0f, -1.0f,//      4--------5 |
				-1.0f, -1.0f, -1.0f,//      | |      | |
				-1.0f,  1.0f,  1.0f,//      | 3------|-2
				 1.0f,  1.0f,  1.0f,//      |/       |/
				 1.0f,  1.0f, -1.0f,//      0--------1
				-1.0f,  1.0f, -1.0f
			};

			unsigned int skyboxIndices[] = {
				// Right
				1, 2, 6,
				6, 5, 1,

				// Left
				0, 4, 7,
				7, 3, 0,

				// Top
				4, 5, 6,
				6, 7, 4,

				// Bottom
				0, 3, 2,
				2, 1, 0,

				// Back
				0, 1, 5,
				5, 4, 0,

				// Front
				3, 7, 6,
				6, 2, 3
			};

			// generate buffers
			glGenVertexArrays(1, &this->skybox->vao);
			glGenBuffers(4, this->skybox->vbo);
			glGenBuffers(1, &this->skybox->ebo);

			// bind & write buffers
			glBindVertexArray(this->skybox->vao);
			glBindBuffer(GL_ARRAY_BUFFER, this->skybox->vbo[0]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->skybox->ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), &skyboxIndices, GL_STATIC_DRAW);
		
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
			glEnableVertexAttribArray(0);

			// Unbind buffers
			glBindVertexArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

			// skybox texture
			glGenTextures(1, &(this->skybox->textures[0]));
			glBindTexture(GL_TEXTURE_CUBE_MAP, this->skybox->textures[0]);

			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

			for (unsigned int i = 0; i < 6; i++)
			{
				int width, height, nrChannels;
				unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 4);
				if (data){
					stbi_set_flip_vertically_on_load(false);
					glTexImage2D
					(
						GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
						0,
						GL_RGBA,
						width,
						height,
						0,
						GL_RGBA,
						GL_UNSIGNED_BYTE,
						data
					);
					stbi_image_free(data);
				}
				else{
					std::cout << "Failed to load texture: " << faces[i] << std::endl;
					stbi_image_free(data);
				}
			}

		}

		if (!this->flag) {
			this->flag = new Drawable;

			// shader
			this->flag->shader = new
				Shader(
					PROJECT_DIR "/src/shaders/flag.vert",
					nullptr, nullptr, nullptr,
					PROJECT_DIR "/src/shaders/flag.frag");

			// VAO
			glGenVertexArrays(1, &this->flag->vao);
			glBindVertexArray(this->flag->vao);

			// VBOs
			glGenBuffers(4, this->flag->vbo);

				// Position attribute
				GLfloat  vertices[11 * 11 * 3] = { 0.0f };
				int index = 0;
				float sizeX = 1.0f;
				float sizeY = 2.0f;
				for (float x = -sizeX; x <= sizeX; x += sizeX / 5) {
					for (float y = -sizeY; y <= sizeY; y += sizeY / 5) {
						vertices[index * 3 + 0] = x;
						vertices[index * 3 + 1] = y;
						vertices[index * 3 + 2] = 0.0f;

						index++;
					}
				}

				glBindBuffer(GL_ARRAY_BUFFER, this->flag->vbo[0]);
				glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
				glEnableVertexAttribArray(0);

				// Normal attribute
				GLfloat  normal[11 * 11 * 3] = { 0.0f };
				GLfloat  mycolor[11 * 11 * 3] = { 0.0f };
				for (int i = 0; i < 363; i++) {
					if ((i + 1) % 3 == 0) {
						mycolor[i] = 1.0f;
					}
					else if ((i + 1) % 3 == 1) {
						mycolor[i] = 0.8f;
					}
					else {
						mycolor[i] = 0.8f;
						normal[i] = 1.0f;
					}
				}

				glBindBuffer(GL_ARRAY_BUFFER, this->flag->vbo[1]);
				glBufferData(GL_ARRAY_BUFFER, sizeof(normal), normal, GL_STATIC_DRAW);
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
				glEnableVertexAttribArray(1);

				// Texture Coordinate attribute
				GLfloat  texture_coordinate[11 * 11 * 2] = { 0.0f };
				index = 0;
				for (int x = 0; x < 11; x++) {
					for (int y = 0; y < 11; y++) {
						texture_coordinate[index * 2] = y / 10.0;
						texture_coordinate[index * 2 + 1] = x / 10.0;

						index++;
					}
				}

				glBindBuffer(GL_ARRAY_BUFFER, this->flag->vbo[2]);
				glBufferData(GL_ARRAY_BUFFER, sizeof(texture_coordinate), texture_coordinate, GL_STATIC_DRAW);
				glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
				glEnableVertexAttribArray(2);

				// Color attribute
				glBindBuffer(GL_ARRAY_BUFFER, this->flag->vbo[3]);
				glBufferData(GL_ARRAY_BUFFER, sizeof(mycolor), mycolor, GL_STATIC_DRAW);
				glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
				glEnableVertexAttribArray(3);

			// EBO
			glGenBuffers(1, &this->flag->ebo);
				GLuint element[10 * 10 * 6] = { 0 };
				int i = 0;
				for (int row = 0; row < 10; row++) {
					for (int col = 0; col < 10; col++) {
						int start = row * 11 + col;

						element[i + 0] = start;
						element[i + 1] = start + 1;
						element[i + 2] = start + 12;

						element[i + 3] = start;
						element[i + 4] = start + 12;
						element[i + 5] = start + 11;

						i += 6;
					}
				}

				this->flag->element_amount = sizeof(element) / sizeof(GLuint);

				//Element attribute
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->flag->ebo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(element), element, GL_STATIC_DRAW);
			
			// Unbind VAO
			glBindVertexArray(0);
		}

		if (!this->streetLight) {
			this->streetLight = new Object(
				this->baseDIR + "/src/shaders/simple.vert",
				this->baseDIR + "/src/shaders/simple.frag",
				this->baseDIR + "/Objs/Streetlight_LowRes.obj",
				""
			);
			setObject(*this->streetLight);

			this->light = new Object(
				this->baseDIR + "/src/shaders/light.vert",
				this->baseDIR + "/src/shaders/simple.frag",
				this->baseDIR + "/Objs/Crate1.obj",
				""
			);
			setObject(*this->light);
		}

		if (!this->cups) {
			this->cups = new Cups(
				this->baseDIR + "/src/shaders/cup.vert",
				this->baseDIR + "/src/shaders/phong.frag",
				this->baseDIR + "/Objs/cup.obj",
				""
			);
			setObject(*this->cups);
			this->cups->light_pos = glm::vec3(5.0f, 30.0f, 5.0f);

			this->cup_stage = new Object(
				this->baseDIR + "/src/shaders/simple.vert",
				this->baseDIR + "/src/shaders/phong.frag",
				this->baseDIR + "/Objs/stage.obj",
				""
			);
			setObject(*this->cup_stage);
			this->cup_stage->light_pos = glm::vec3(5.0f, 30.0f, 5.0f);

			this->cups->cup_center = glm::vec3(100, 4, 100);
			float radius = 25;
			const float PI = 3.14159265358979323846;
			for (float degree = 0.0; degree <= 360.0; degree += 1.0) {
				float radians = degree / (180.0 / PI);
				float x = radius * cos(radians) + this->cups->cup_center.x;
				float z = radius * sin(radians) + this->cups->cup_center.z;

				this->cups->cup_track.push_back(glm::vec3(x, this->cups->cup_center.y, z));
			}

			this->cups->cups_pos_index[0] = 0;
			this->cups->cups_pos_index[1] = this->cups->cup_track.size() / 5;
			this->cups->cups_pos_index[2] = (this->cups->cup_track.size() / 5) * 2;
			this->cups->cups_pos_index[3] = (this->cups->cup_track.size() / 5) * 3;
			this->cups->cups_pos_index[4] = (this->cups->cup_track.size() / 5) * 4;
		}

		if (!this->house) {
			this->house = new Object(
				this->baseDIR + "/src/shaders/house.vert",
				this->baseDIR + "/src/shaders/house.frag",
				this->baseDIR + "/Objs/house.obj",
				""
			);
			setObject(*this->house);
		}

		if (!this->fireworks) {
			this->fireworks = new Firework;
		}

		if (!this->commom_matrices) {
			this->commom_matrices = new UBO();
			this->commom_matrices->size = 2 * sizeof(glm::mat4);
			glGenBuffers(1, &this->commom_matrices->ubo);
			glBindBuffer(GL_UNIFORM_BUFFER, this->commom_matrices->ubo);
			glBufferData(GL_UNIFORM_BUFFER, this->commom_matrices->size, NULL, GL_STATIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
		}

		if (!this->texture)
			this->texture = new Texture2D((this->baseDIR + "/Images/flag.png").c_str());
	}
	else
		throw std::runtime_error("Could not initialize GLAD!");

	// Set up the view port
	glViewport(0,0,w(),h());

	// clear the window, be sure to clear the Z-Buffer too
	glClearColor(0,0,.3f,0);		// background should be blue

	// we need to clear out the stencil buffer since we'll use
	// it for shadows
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glEnable(GL_DEPTH);

	// Blayne prefers GL_DIFFUSE
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	// prepare for projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	setProjection();		// put the code to set up matrices here

	//######################################################################
	// TODO: 
	// you might want to set the lighting up differently. if you do, 
	// we need to set up the lights AFTER setting up the projection
	//######################################################################
	// enable the lighting
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	// top view only needs one light
	if (tw->topCam->value()) {
		glDisable(GL_LIGHT1);
		glDisable(GL_LIGHT2);
	} else {
		glEnable(GL_LIGHT1);
		glEnable(GL_LIGHT2);
	}

	//*********************************************************************
	//
	// * set the light parameters
	//
	//**********************************************************************
	GLfloat lightPosition1[]	= {0, 1, 1, 0}; // {50, 200.0, 50, 1.0};
	GLfloat lightPosition2[]	= {1, 0, 0, 0};
	GLfloat lightPosition3[]	= {0, -1, 0, 0};
	GLfloat yellowLight[]		= {0.5f, 0.5f, .1f, 1.0};
	GLfloat whiteLight[]			= {1.0f, 1.0f, 1.0f, 1.0};
	GLfloat blueLight[]			= {.1f,.1f,.3f,1.0};
	GLfloat grayLight[]			= {.3f, .3f, .3f, 1.0};

	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition1);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, whiteLight);
	glLightfv(GL_LIGHT0, GL_AMBIENT, grayLight);

	glLightfv(GL_LIGHT1, GL_POSITION, lightPosition2);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, yellowLight);

	glLightfv(GL_LIGHT2, GL_POSITION, lightPosition3);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, blueLight);

	//*********************************************************************
	// now draw the ground plane
	//*********************************************************************
	// set to opengl fixed pipeline(use opengl 1.x draw function)
	glUseProgram(0);

	setupFloor();
	glDisable(GL_LIGHTING);
	drawFloor(360,10);


	//*********************************************************************
	// now draw the object and we need to do it twice
	// once for real, and then once for shadows
	//*********************************************************************
	glEnable(GL_LIGHTING);
	setupObjects();

	drawStuff();
	// this time drawing is for shadows (except for top view)
	if (!tw->topCam->value()) {
		setupShadows();
		drawStuff(true);
		unsetupShadows();
	}

	setUBO();
	glBindBufferRange(GL_UNIFORM_BUFFER, /*binding point*/0, this->commom_matrices->ubo, 0, this->commom_matrices->size);


	drawSkybox();
	drawStreetlight(glm::vec3(0.1f, 0.1f, 0.1f), glm::vec3(50.0f, 0.0f, 70.0f));
	drawCups(glm::vec3(3.5f, 3.5f, 3.5f));
	this->fireworks->drawScene();
	drawShops();
	drawFlag();
}

//************************************************************************
//
// * This sets up both the Projection and the ModelView matrices
//   HOWEVER: it doesn't clear the projection first (the caller handles
//   that) - its important for picking
//========================================================================
void TrainView::
setProjection()
//========================================================================
{
	// Compute the aspect ratio (we'll need it)
	float aspect = static_cast<float>(w()) / static_cast<float>(h());

	// Check whether we use the world camp
	if (tw->worldCam->value())
		arcball.setProjection(false);
	// Or we use the top cam
	else if (tw->topCam->value()) {
		float wi, he;
		if (aspect >= 1) {
			wi = 110;
			he = wi / aspect;
		} 
		else {
			he = 110;
			wi = he * aspect;
		}

		// Set up the top camera drop mode to be orthogonal and set
		// up proper projection matrix
		glMatrixMode(GL_PROJECTION);
		glOrtho(-wi, wi, -he, he, 200, -200);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef(-90,1,0,0);
	} 
	// Or do the train view or other view here
	//####################################################################
	// TODO: 
	// put code for train view projection here!	
	//####################################################################
	else {
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(100, aspect, 0.1, 200);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		Pnt3f train_center = train_toward + train_pos;
		gluLookAt(
			train_pos.x, train_pos.y, train_pos.z,
			train_center.x, train_center.y, train_center.z,
			dir_up.x, dir_up.y, dir_up.z
		);
	}
}

// refer to: https://stackoverflow.com/questions/41423203/b-spline-curve-in-c
void bSpline(float x[], float y[], float z[], vector<Pnt3f>& point_list) {
	for (float t = 0; t <= 1; t += 0.01) {
		const double t2 = t * t;
		const double t3 = t2 * t;
		const double mt = 1.0 - t;
		const double mt3 = mt * mt * mt;

		const double bi3 = mt3;
		const double bi2 = 3 * t3 - 6 * t2 + 4;
		const double bi1 = -3 * t3 + 3 * t2 + 3 * t + 1;
		const double bi = t3;

		float xt = x[0] * bi3
			+ x[1] * bi2
			+ x[2] * bi1
			+ x[3] * bi;
		xt /= 6.0;

		float yt = y[0] * bi3
			+ y[1] * bi2
			+ y[2] * bi1
			+ y[3] * bi;
		yt /= 6.0;

		float zt = z[0] * bi3
			+ z[1] * bi2
			+ z[2] * bi1
			+ z[3] * bi;
		zt /= 6.0;

		point_list.push_back(Pnt3f(xt, yt, zt));
	}
}

void cardinalCurve(float tension, float x[], float y[], float z[], vector<Pnt3f>& point_list) {
	for (float t = 0; t <= 1; t += 0.01) {
		float xt = (-x[0] + (2 / tension - 1) * x[1] + (1 - 2 / tension) * x[2] + x[3]) * t * t * t
			+ (2 * x[0] + (1 - 3 / tension) * x[1] + (3 / tension - 2) * x[2] - x[3]) * t * t
			+ (-x[0] + x[2]) * t
			+ (1 / tension) * x[1];
		xt *= tension;

		float yt = (-y[0] + (2 / tension - 1) * y[1] + (1 - 2 / tension) * y[2] + y[3]) * t * t * t
			+ (2 * y[0] + (1 - 3 / tension) * y[1] + (3 / tension - 2) * y[2] - y[3]) * t * t
			+ (-y[0] + y[2]) * t
			+ (1 / tension) * y[1];
		yt *= tension;

		float zt = (-z[0] + (2 / tension - 1) * z[1] + (1 - 2 / tension) * z[2] + z[3]) * t * t * t
			+ (2 * z[0] + (1 - 3 / tension) * z[1] + (3 / tension - 2) * z[2] - z[3]) * t * t
			+ (-z[0] + z[2]) * t
			+ (1 / tension) * z[1];
		zt *= tension;

		point_list.push_back(Pnt3f(xt, yt, zt));
	}
}

void TrainView::drawTrack(bool doingShadows) {
	vector<Pnt3f> leftPoints;
	vector<Pnt3f> rightPoints;
	tw->arcLength_float = 0;
	point_list.clear();

	// Linear track
	if (tw->splineBrowser->value() == 1) {
		for (size_t i = 0; i < m_pTrack->points.size(); ++i) {
			// pos
			Pnt3f qt0 = m_pTrack->points[i].pos;
			Pnt3f qt1 = m_pTrack->points[(i + 1) % m_pTrack->points.size()].pos;

			// total length of track
			tw->arcLength_float += (qt1 - qt0).length();

			// cross
			Pnt3f toward = qt1 - qt0;
			Pnt3f dir_left = toward * dir_up;
			dir_left.normalize();
			dir_left = dir_left * 2.5f;

			// draw track
			glLineWidth(3);
			glBegin(GL_LINES);
			if (!doingShadows)
				glColor3ub(72, 81, 84);
			glVertex3f(qt0.x + dir_left.x, qt0.y + dir_left.y, qt0.z + dir_left.z);
			glVertex3f(qt1.x + dir_left.x, qt1.y + dir_left.y, qt1.z + dir_left.z);
			glVertex3f(qt0.x - dir_left.x, qt0.y - dir_left.y, qt0.z - dir_left.z);
			glVertex3f(qt1.x - dir_left.x, qt1.y - dir_left.y, qt1.z - dir_left.z);
			glEnd();

			// make point list
			for (float t = 0; t <= 1; t += 0.01) {
				point_list.push_back(qt0 + toward * t);
			}
		}
	}

	// curve track
	else if (tw->splineBrowser->value() == 2 || tw->splineBrowser->value() == 3) {
		for (size_t i = 0; i < m_pTrack->points.size(); ++i) {
			// pos
			Pnt3f p1_pos = m_pTrack->points[i].pos;
			Pnt3f p2_pos = m_pTrack->points[(i + 1) % m_pTrack->points.size()].pos;
			Pnt3f p3_pos = m_pTrack->points[(i + 2) % m_pTrack->points.size()].pos;
			Pnt3f p4_pos = m_pTrack->points[(i + 3) % m_pTrack->points.size()].pos;

			float x[4] = {
				p1_pos.x,
				p2_pos.x,
				p3_pos.x,
				p4_pos.x
			};

			float y[4] = {
				p1_pos.y,
				p2_pos.y,
				p3_pos.y,
				p4_pos.y
			};

			float z[4] = {
				p1_pos.z,
				p2_pos.z,
				p3_pos.z,
				p4_pos.z
			};


			// find whcih curve
			if (tw->splineBrowser->value() == 2)
				cardinalCurve(tw->tension->value(), x, y, z, point_list);
			else
				bSpline(x, y, z, point_list);

			// cross
			for (int j = 0; j < point_list.size(); j++) {
				Pnt3f qt0(point_list[j].x, point_list[j].y, point_list[j].z);
				Pnt3f qt1(point_list[(j + 1) % point_list.size()].x, point_list[(j + 1) % point_list.size()].y, point_list[(j + 1) % point_list.size()].z);
				Pnt3f toward = qt1 - qt0;

				Pnt3f dir_left = toward * dir_up;
				dir_left.normalize();

				// record left and right track points
				leftPoints.push_back(qt0 + dir_left * 2.5f);
				rightPoints.push_back(qt0 - dir_left * 2.5f);

				// total length of track
				tw->arcLength_float += (qt1 - qt0).length();
			}

			// draw left track
			glLineWidth(3);
			glBegin(GL_LINE_STRIP);
			if (!doingShadows)
				glColor3ub(72, 81, 84);
			for (auto p : leftPoints)
				glVertex3f(p.x, p.y, p.z);
			glEnd();

			// draw right track
			glLineWidth(3);
			glBegin(GL_LINE_STRIP);
			if (!doingShadows)
				glColor3ub(72, 81, 84);
			for (auto p : rightPoints)
				glVertex3f(p.x, p.y, p.z);
			glEnd();

			// draw middle track
			/*glLineWidth(3);
			glBegin(GL_POINTS);
				if (!doingShadows)
					glColor3ub(72, 81, 84);
				for (auto p : point_list)
					glVertex3f(p.x, p.y, p.z);
			glEnd();*/

			leftPoints.clear();
			rightPoints.clear();
		}
	}

	// draw track tiles
	// refer to: https://github.com/okh8609/CG_Project3_RollerCoaster/blob/master/Roller%20Coaster/Src/TrainView.cpp
	float curr_offset = 0;
	float tie_width = 4;
	float tie_length = tie_width * 3;
	float tie_thickness = tie_width / 2;

	for (int i = 0; i < point_list.size(); i++) {
		Pnt3f curr_pos = point_list[i];
		Pnt3f next_pos = point_list[(i + 1) % point_list.size()];
		float distance = (next_pos - curr_pos).length();

		// no need to draw tile, move forward
		if (curr_offset < tie_width * 2) {
			curr_offset += distance;
		}
		// draw tile
		else {
			curr_offset = 0;
			Pnt3f dir_front = next_pos - curr_pos;
			dir_front.normalize();
			Pnt3f dir_left = dir_front * dir_up;
			dir_left.normalize();

			// define nodes on the tie
			Pnt3f p0 = curr_pos
				+ dir_front * tie_width
				- dir_left * 0.5 * tie_length
				+ dir_up * tie_thickness;

			Pnt3f p1 = curr_pos
				+ dir_front * tie_width
				- dir_left * 0.5 * tie_length;

			Pnt3f p2 = curr_pos
				+ dir_front * tie_width
				+ dir_left * 0.5 * tie_length;

			Pnt3f p3 = curr_pos
				+ dir_front * tie_width
				+ dir_left * 0.5 * tie_length
				+ dir_up * tie_thickness;

			Pnt3f p4 = curr_pos
				+ dir_left * 0.5 * tie_length
				+ dir_up * tie_thickness;

			Pnt3f p5 = curr_pos
				- dir_left * 0.5 * tie_length
				+ dir_up * tie_thickness;

			Pnt3f p6 = curr_pos
				- dir_left * 0.5 * tie_length;

			Pnt3f p7 = curr_pos
				+ dir_left * 0.5 * tie_length;

			// color of track ties
			if (!doingShadows)
				glColor3ub(128, 87, 41);

			glBegin(GL_QUADS);
			// front surface
			glVertex3f(p0.x, p0.y, p0.z);
			glVertex3f(p1.x, p1.y, p1.z);
			glVertex3f(p2.x, p2.y, p2.z);
			glVertex3f(p3.x, p3.y, p3.z);

			// back surface
			glVertex3f(p4.x, p4.y, p4.z);
			glVertex3f(p5.x, p5.y, p5.z);
			glVertex3f(p6.x, p6.y, p6.z);
			glVertex3f(p7.x, p7.y, p7.z);

			// top surface
			glVertex3f(p0.x, p0.y, p0.z);
			glVertex3f(p3.x, p3.y, p3.z);
			glVertex3f(p4.x, p4.y, p4.z);
			glVertex3f(p5.x, p5.y, p5.z);


			// bottom surface
			glVertex3f(p1.x, p1.y, p1.z);
			glVertex3f(p2.x, p2.y, p2.z);
			glVertex3f(p7.x, p7.y, p7.z);
			glVertex3f(p6.x, p6.y, p6.z);

			// left surface
			glVertex3f(p2.x, p2.y, p2.z);
			glVertex3f(p3.x, p3.y, p3.z);
			glVertex3f(p4.x, p4.y, p4.z);
			glVertex3f(p7.x, p7.y, p7.z);

			// right surface
			glVertex3f(p0.x, p0.y, p0.z);
			glVertex3f(p1.x, p1.y, p1.z);
			glVertex3f(p6.x, p6.y, p6.z);
			glVertex3f(p5.x, p5.y, p5.z);
			glEnd();
		}
	}
}

void drawCart(Pnt3f train_pos, Pnt3f train_toward, Pnt3f train_left, Pnt3f dir_up, float train_length, float train_width, float train_height, bool doingShadows) {
	// define nodes on the train
	Pnt3f p0 = train_pos
		+ train_toward * 0.5 * train_length
		- train_left * 0.5 * train_width
		+ dir_up * 0.5 * train_height;

	Pnt3f p1 = train_pos
		+ train_toward * 0.5 * train_length
		- train_left * 0.5 * train_width
		- dir_up * 0.5 * train_height;

	Pnt3f p2 = train_pos
		+ train_toward * 0.5 * train_length
		+ train_left * 0.5 * train_width
		- dir_up * 0.5 * train_height;

	Pnt3f p3 = train_pos
		+ train_toward * 0.5 * train_length
		+ train_left * 0.5 * train_width
		+ dir_up * 0.5 * train_height;

	Pnt3f p4 = train_pos
		- train_toward * 0.5 * train_length
		+ train_left * 0.5 * train_width
		+ dir_up * 0.5 * train_height;

	Pnt3f p5 = train_pos
		- train_toward * 0.5 * train_length
		- train_left * 0.5 * train_width
		+ dir_up * 0.5 * train_height;

	Pnt3f p6 = train_pos
		- train_toward * 0.5 * train_length
		- train_left * 0.5 * train_width
		- dir_up * 0.5 * train_height;

	Pnt3f p7 = train_pos
		- train_toward * 0.5 * train_length
		+ train_left * 0.5 * train_width
		- dir_up * 0.5 * train_height;

	// color of train
	if (!doingShadows)
		glColor3ub(61, 82, 89);

	glBegin(GL_QUADS);
	// front surface
	glVertex3f(p0.x, p0.y, p0.z);
	glVertex3f(p1.x, p1.y, p1.z);
	glVertex3f(p2.x, p2.y, p2.z);
	glVertex3f(p3.x, p3.y, p3.z);

	// back surface
	glVertex3f(p4.x, p4.y, p4.z);
	glVertex3f(p5.x, p5.y, p5.z);
	glVertex3f(p6.x, p6.y, p6.z);
	glVertex3f(p7.x, p7.y, p7.z);

	// top surface
	glVertex3f(p0.x, p0.y, p0.z);
	glVertex3f(p3.x, p3.y, p3.z);
	glVertex3f(p4.x, p4.y, p4.z);
	glVertex3f(p5.x, p5.y, p5.z);


	// bottom surface
	glVertex3f(p1.x, p1.y, p1.z);
	glVertex3f(p2.x, p2.y, p2.z);
	glVertex3f(p7.x, p7.y, p7.z);
	glVertex3f(p6.x, p6.y, p6.z);

	// left surface
	glVertex3f(p2.x, p2.y, p2.z);
	glVertex3f(p3.x, p3.y, p3.z);
	glVertex3f(p4.x, p4.y, p4.z);
	glVertex3f(p7.x, p7.y, p7.z);

	// right surface
	glVertex3f(p0.x, p0.y, p0.z);
	glVertex3f(p1.x, p1.y, p1.z);
	glVertex3f(p6.x, p6.y, p6.z);
	glVertex3f(p5.x, p5.y, p5.z);
	glEnd();
}

void TrainView::drawTrain(bool doingShadows) {
	float train_width = 10;
	float train_length = train_width * 2;
	float train_height = train_width;

	if (!train_setup) {
		train_pos = point_list[0] + Pnt3f(0, train_height / 2 + 2, 0);
		train_toward = point_list[1] - point_list[0];
		train_toward.normalize();

		train_pos2 = point_list[35] + Pnt3f(0, train_height / 2 + 2, 0);
		train_toward2 = point_list[36] - point_list[35];
		train_toward2.normalize();

		train_setup = true;
	}

	train_left = train_toward * dir_up;
	train_left2 = train_toward2 * dir_up;

	drawCart(train_pos, train_toward, train_left, dir_up, train_length, train_width, train_height, doingShadows);
	drawCart(train_pos2, train_toward2, train_left2, dir_up, train_length, train_width, train_height, doingShadows);

	//glm::vec3 tp(this->train_pos.x, this->train_pos.y, this->train_pos.z);
	//drawObject(*this->cart, glm::vec3(3.0f, 3.0f, 3.0f), tp, false, glm::vec4(1.0, 0.5, 0, 1.0));
}

//************************************************************************
//
// * this draws all of the stuff in the world
//
//	NOTE: if you're drawing shadows, DO NOT set colors (otherwise, you get 
//       colored shadows). this gets called twice per draw 
//       -- once for the objects, once for the shadows
//########################################################################
// TODO: 
// if you have other objects in the world, make sure to draw them
//########################################################################
//========================================================================
void TrainView::drawStuff(bool doingShadows)
{
	// Draw the control points
	// don't draw the control points if you're driving 
	// (otherwise you get sea-sick as you drive through them)
	if (!tw->trainCam->value()) {
		for(size_t i=0; i<m_pTrack->points.size(); ++i) {
			if (!doingShadows) {
				if ( ((int) i) != selectedCube)
					glColor3ub(240, 60, 60);
				else
					glColor3ub(240, 240, 30);
			}
			m_pTrack->points[i].draw();
		}
	}
	// draw the track
	//####################################################################
	// TODO: 
	// call your own track drawing code
	//####################################################################
	drawTrack(doingShadows);

	// draw the train
	//####################################################################
	// TODO: 
	//	call your own train drawing code
	//####################################################################
	if (!tw->trainCam->value())
		drawTrain(doingShadows);
}

// 
//************************************************************************
//
// * this tries to see which control point is under the mouse
//	  (for when the mouse is clicked)
//		it uses OpenGL picking - which is always a trick
//########################################################################
// TODO: 
//		if you want to pick things other than control points, or you
//		changed how control points are drawn, you might need to change this
//########################################################################
//========================================================================
void TrainView::
doPick()
//========================================================================
{
	// since we'll need to do some GL stuff so we make this window as 
	// active window
	make_current();		

	// where is the mouse?
	int mx = Fl::event_x(); 
	int my = Fl::event_y();

	// get the viewport - most reliable way to turn mouse coords into GL coords
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	// Set up the pick matrix on the stack - remember, FlTk is
	// upside down!
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity ();
	gluPickMatrix((double)mx, (double)(viewport[3]-my), 
						5, 5, viewport);

	// now set up the projection
	setProjection();

	// now draw the objects - but really only see what we hit
	GLuint buf[100];
	glSelectBuffer(100,buf);
	glRenderMode(GL_SELECT);
	glInitNames();
	glPushName(0);

	// draw the cubes, loading the names as we go
	for(size_t i=0; i<m_pTrack->points.size(); ++i) {
		glLoadName((GLuint) (i+1));
		m_pTrack->points[i].draw();
	}

	// go back to drawing mode, and see how picking did
	int hits = glRenderMode(GL_RENDER);
	if (hits) {
		// warning; this just grabs the first object hit - if there
		// are multiple objects, you really want to pick the closest
		// one - see the OpenGL manual 
		// remember: we load names that are one more than the index
		selectedCube = buf[3]-1;
	} else // nothing hit, nothing selected
		selectedCube = -1;

	printf("Selected Cube %d\n",selectedCube);
}

void TrainView::setUBO()
{
	float wdt = this->pixel_w();
	float hgt = this->pixel_h();

	glm::mat4 view_matrix;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
	//HMatrix view_matrix; 
	//this->arcball.getMatrix(view_matrix);

	glm::mat4 projection_matrix;
	glGetFloatv(GL_PROJECTION_MATRIX, &projection_matrix[0][0]);
	//projection_matrix = glm::perspective(glm::radians(this->arcball.getFoV()), (GLfloat)wdt / (GLfloat)hgt, 0.01f, 1000.0f);

	glBindBuffer(GL_UNIFORM_BUFFER, this->commom_matrices->ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &projection_matrix[0][0]);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), &view_matrix[0][0]);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}