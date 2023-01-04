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
	start_time = std::chrono::steady_clock::now();
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

void TrainView::drawSineWave() {
	//bind shader
	this->sineWave->shader->Use();

	glm::mat4 model_matrix = glm::mat4();
	model_matrix = glm::translate(model_matrix, this->source_pos);
	model_matrix = glm::scale(model_matrix, glm::vec3(10.0f, 10.0f, 10.0f));
	glUniformMatrix4fv(
		glGetUniformLocation(this->sineWave->shader->Program, "u_model"), 1, GL_FALSE, &model_matrix[0][0]);
	glUniform1f(glGetUniformLocation(this->sineWave->shader->Program, "time"), this->duration_time);

	//bind VAO
	glBindVertexArray(this->sineWave->vao);

	// draw water surface
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LIGHTING);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, this->skybox->textures[0]);
	glUniform1i(glGetUniformLocation(this->skybox->shader->Program, "skybox"), 0);

	/*glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, this->tub->textures[0]);
	glUniform1i(glGetUniformLocation(this->skybox->shader->Program, "tub"), this->tub->textures[0]);*/

	glDrawElements(GL_TRIANGLES, this->sineWave->element_amount, GL_UNSIGNED_INT, 0);

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

void TrainView::drawTub() {
	//bind shader
	this->tub->shader->Use();

	glm::mat4 model_matrix = glm::mat4();
	model_matrix = glm::translate(model_matrix, this->source_pos);
	model_matrix = glm::scale(model_matrix, glm::vec3(10.0f, 10.0f, 10.0f));
	glUniformMatrix4fv(glGetUniformLocation(this->tub->shader->Program, "u_model"), 1, GL_FALSE, &model_matrix[0][0]);

	//bind VAO
	glBindVertexArray(this->tub->vao);

	// draw
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, this->tub->textures[0]);
	glDrawElements(GL_TRIANGLES, this->tub->element_amount, GL_UNSIGNED_INT, 0);

	//unbind VAO
	glBindVertexArray(0);

	//unbind shader(switch to fixed pipeline)
	glUseProgram(0);
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
					PROJECT_DIR "/src/shaders/skybox.vert",
					nullptr, nullptr, nullptr,
					PROJECT_DIR "/src/shaders/skybox.frag");
		
			// data
			vector<std::string> faces {
				PROJECT_DIR"/Images/skybox1/1.png",
				PROJECT_DIR"/Images/skybox1/2.png",
				PROJECT_DIR"/Images/skybox1/3.png",
				PROJECT_DIR"/Images/skybox1/4.png",
				PROJECT_DIR"/Images/skybox1/5.png",
				PROJECT_DIR"/Images/skybox1/6.png"
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

		if (!this->sineWave) {
			this->sineWave = new Drawable;

			// shader
			this->sineWave->shader = new
				Shader(
					PROJECT_DIR "/src/shaders/sineWave.vert",
					nullptr, nullptr, nullptr,
					PROJECT_DIR "/src/shaders/sineWave.frag");

			/*GLfloat  texture_coordinate[] = {
				0.0f, 0.0f,
				1.0f, 0.0f,
				1.0f, 1.0f,
				0.0f, 1.0f };*/

			// VAO
			glGenVertexArrays(1, &this->sineWave->vao);
			glBindVertexArray(this->sineWave->vao);

			// VBOs
			glGenBuffers(4, this->sineWave->vbo);

				// Position attribute
				GLfloat  vertices[11 * 11 * 3] = { 0.0f };
				int index = 0;
				for (float x = -5.0f; x <= 5.0f; x += 1.0f) {
					for (float z = -5.0f; z <= 5.0f; z += 1.0f) {
						vertices[index * 3 + 0] = x;
						vertices[index * 3 + 1] = 0.0f;
						vertices[index * 3 + 2] = z;

						index++;
					}
				}

				glBindBuffer(GL_ARRAY_BUFFER, this->sineWave->vbo[0]);
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

				glBindBuffer(GL_ARRAY_BUFFER, this->sineWave->vbo[1]);
				glBufferData(GL_ARRAY_BUFFER, sizeof(normal), normal, GL_STATIC_DRAW);
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
				glEnableVertexAttribArray(1);

				//// Texture Coordinate attribute
				//glBindBuffer(GL_ARRAY_BUFFER, this->sineWave->vbo[2]);
				//glBufferData(GL_ARRAY_BUFFER, sizeof(texture_coordinate), texture_coordinate, GL_STATIC_DRAW);
				//glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
				//glEnableVertexAttribArray(2);

				// Color attribute
				glBindBuffer(GL_ARRAY_BUFFER, this->sineWave->vbo[3]);
				glBufferData(GL_ARRAY_BUFFER, sizeof(mycolor), mycolor, GL_STATIC_DRAW);
				glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
				glEnableVertexAttribArray(3);

			// EBO
			glGenBuffers(1, &this->sineWave->ebo);
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

				this->sineWave->element_amount = sizeof(element) / sizeof(GLuint);

				//Element attribute
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->sineWave->ebo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(element), element, GL_STATIC_DRAW);
			
			// Unbind VAO
			glBindVertexArray(0);
		}

		if (!this->tub) {
			this->tub = new Drawable;

			// shader
			this->tub->shader = new
				Shader(
					PROJECT_DIR "/src/shaders/simple.vert",
					nullptr, nullptr, nullptr,
					PROJECT_DIR "/src/shaders/simple.frag");

			float tubVertices[] = {
				//   Coordinates
				-5.0f, -5.0f,  5.0f,//        7--------6
				 5.0f, -5.0f,  5.0f,//       /|       /|
				 5.0f, -5.0f, -5.0f,//      4--------5 |
				-5.0f, -5.0f, -5.0f,//      | |      | |
				-5.0f,  5.0f,  5.0f,//      | 3------|-2
				 5.0f,  5.0f,  5.0f,//      |/       |/
				 5.0f,  5.0f, -5.0f,//      0--------1
				-5.0f,  5.0f, -5.0f
			};

			float texture_coordinate[] = {
				//     0,1--------1,1
				//      |          |
				//      |          |
				//      |          |
				//     0,0--------1,0

				//// Bottom // 0, 3, 2,
				0.0f, 0.0f,
				0.0f, 1.0f,
				1.0f, 1.0f,
				
				// Bottom // 2, 1, 0,
				1.0f, 1.0f,
				1.0f, 0.0f,
				0.0f, 0.0f,

				// Front // 3, 7, 6,
				0.0f, 0.0f,
				0.0f, 1.0f,
				1.0f, 1.0f,

				// Front // 6, 2, 3
				1.0f, 1.0f,
				1.0f, 0.0f,
				0.0f, 0.0f,

				// Left // 0, 4, 7
				0.0f, 0.0f,
				0.0f, 1.0f,
				1.0f, 1.0f,

				// Left // 7, 3, 0
				1.0f, 1.0f,
				1.0f, 0.0f,
				0.0f, 0.0f,
			};

			unsigned int tubIndices[] = {
				// Bottom
				0, 3, 2,
				2, 1, 0,

				// Front
				3, 7, 6,
				6, 2, 3,

				// Left
				0, 4, 7,
				7, 3, 0,
			};

			this->tub->element_amount = sizeof(tubIndices) / sizeof(unsigned int);

			GLfloat normal[3 * 8] = { 0.0f };
			GLfloat mycolor[3 * 8] = { 0.0f };
			for (int i = 0; i < 24; i++) {
				if ((i + 1) % 3 == 0) {
					mycolor[i] = 1.0f;	// R
					normal[i] = 0.0f;	// X
				}
				else if ((i + 1) % 3 == 1) {
					mycolor[i] = 1.0f;	// G
					normal[i] = 1.0f;	// Y
				}
				else {
					mycolor[i] = 1.0f;	// B
					normal[i] = 0.0f;	// Z
				}

				mycolor[i] = 1.0f;
			}

			// generate buffers
			glGenVertexArrays(1, &this->tub->vao);
			glGenBuffers(4, this->tub->vbo);
			glGenBuffers(1, &this->tub->ebo);

			// bind & write buffers
			glBindVertexArray(this->tub->vao);

				// position
				glBindBuffer(GL_ARRAY_BUFFER, this->tub->vbo[0]);
				glBufferData(GL_ARRAY_BUFFER, sizeof(tubVertices), tubVertices, GL_STATIC_DRAW);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
				glEnableVertexAttribArray(0);

				// normal
				glBindBuffer(GL_ARRAY_BUFFER, this->tub->vbo[1]);
				glBufferData(GL_ARRAY_BUFFER, sizeof(normal), normal, GL_STATIC_DRAW);
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
				glEnableVertexAttribArray(1);

				// texture Coordinate
				glBindBuffer(GL_ARRAY_BUFFER, this->tub->vbo[2]);
				glBufferData(GL_ARRAY_BUFFER, sizeof(texture_coordinate), texture_coordinate, GL_STATIC_DRAW);
				glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
				glEnableVertexAttribArray(2);

				// color
				glBindBuffer(GL_ARRAY_BUFFER, this->tub->vbo[3]);
				glBufferData(GL_ARRAY_BUFFER, sizeof(mycolor), mycolor, GL_STATIC_DRAW);
				glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
				glEnableVertexAttribArray(3);

				// ebo
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->tub->ebo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(tubIndices), &tubIndices, GL_STATIC_DRAW);

			// texture
			glGenTextures(1, &(this->tub->textures[0]));
			glBindTexture(GL_TEXTURE_2D, this->tub->textures[0]);
			glGenerateMipmap(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			// read image
			const char* path = PROJECT_DIR "/Images/tub.png";
			cv::Mat img;
			img = cv::imread(path, cv::IMREAD_COLOR);

			if (img.type() == CV_8UC3)
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, img.cols, img.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, img.data);
			else if (img.type() == CV_8UC4)
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.cols, img.rows, 0, GL_BGRA, GL_UNSIGNED_BYTE, img.data);

			img.release();

			// Unbind buffers
			glBindVertexArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glBindTexture(GL_TEXTURE_2D, 0);
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
			this->texture = new Texture2D(PROJECT_DIR "/Images/church.png");

		/*if (!this->device) {
			//Tutorial: https://ffainelli.github.io/openal-example/
			this->device = alcOpenDevice(NULL);
			if (!this->device)
				puts("ERROR::NO_AUDIO_DEVICE");

			ALboolean enumeration = alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");
			if (enumeration == AL_FALSE)
				puts("Enumeration not supported");
			else
				puts("Enumeration supported");

			this->context = alcCreateContext(this->device, NULL);
			if (!alcMakeContextCurrent(context))
				puts("Failed to make context current");

			this->source_pos = glm::vec3(0.0f, 5.0f, 0.0f);

			ALfloat listenerOri[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
			alListener3f(AL_POSITION, source_pos.x, source_pos.y, source_pos.z);
			alListener3f(AL_VELOCITY, 0, 0, 0);
			alListenerfv(AL_ORIENTATION, listenerOri);

			alGenSources((ALuint)1, &this->source);
			alSourcef(this->source, AL_PITCH, 1);
			alSourcef(this->source, AL_GAIN, 1.0f);
			alSource3f(this->source, AL_POSITION, source_pos.x, source_pos.y, source_pos.z);
			alSource3f(this->source, AL_VELOCITY, 0, 0, 0);
			alSourcei(this->source, AL_LOOPING, AL_TRUE);

			alGenBuffers((ALuint)1, &this->buffer);

			ALsizei size, freq;
			ALenum format;
			ALvoid* data;
			ALboolean loop = AL_TRUE;

			//Material from: ThinMatrix
			alutLoadWAVFile((ALbyte*)PROJECT_DIR "/Audios/bounce.wav", &format, &data, &size, &freq, &loop);
			alBufferData(this->buffer, format, data, size, freq);
			alSourcei(this->source, AL_BUFFER, this->buffer);

			if (format == AL_FORMAT_STEREO16 || format == AL_FORMAT_STEREO8)
				puts("TYPE::STEREO");
			else if (format == AL_FORMAT_MONO16 || format == AL_FORMAT_MONO8)
				puts("TYPE::MONO");

			alSourcePlay(this->source);

			// cleanup context
			//alDeleteSources(1, &source);
			//alDeleteBuffers(1, &buffer);
			//device = alcGetContextsDevice(context);
			//alcMakeContextCurrent(NULL);
			//alcDestroyContext(context);
			//alcCloseDevice(device);
		}*/
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
	GLfloat lightPosition1[]	= {0,1,1,0}; // {50, 200.0, 50, 1.0};
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

	// set linstener position 
	if(selectedCube >= 0)
		alListener3f(AL_POSITION, 
			m_pTrack->points[selectedCube].pos.x,
			m_pTrack->points[selectedCube].pos.y,
			m_pTrack->points[selectedCube].pos.z);
	else
		alListener3f(AL_POSITION, 
			this->source_pos.x, 
			this->source_pos.y,
			this->source_pos.z);


	//*********************************************************************
	// now draw the ground plane
	//*********************************************************************
	// set to opengl fixed pipeline(use opengl 1.x draw function)
	glUseProgram(0);

	setupFloor();
	glDisable(GL_LIGHTING);
	drawFloor(120,10);


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
	glBindBufferRange(
		GL_UNIFORM_BUFFER, /*binding point*/0, this->commom_matrices->ubo, 0, this->commom_matrices->size);

	
	drawSkybox();
	//drawTub();
	drawSineWave();
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
#ifdef EXAMPLE_SOLUTION
		trainCamView(this,aspect);
#endif
	}
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
	/*if (!tw->trainCam->value()) {
		for(size_t i=0; i<m_pTrack->points.size(); ++i) {
			if (!doingShadows) {
				if ( ((int) i) != selectedCube)
					glColor3ub(240, 60, 60);
				else
					glColor3ub(240, 240, 30);
			}
			m_pTrack->points[i].draw();
		}
	}*/
	// draw the track
	//####################################################################
	// TODO: 
	// call your own track drawing code
	//####################################################################

	

#ifdef EXAMPLE_SOLUTION
	drawTrack(this, doingShadows);
#endif

	// draw the train
	//####################################################################
	// TODO: 
	//	call your own train drawing code
	//####################################################################
#ifdef EXAMPLE_SOLUTION
	// don't draw the train if you're looking out the front window
	if (!tw->trainCam->value())
		drawTrain(this, doingShadows);
#endif
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