/************************************************************************
     File:        TrainView.H

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

#pragma once

#include "RenderUtilities/BufferObject.h"
#include "RenderUtilities/Shader.h"
#include "RenderUtilities/Texture.h"
#include "Object.H"
#include "Cups.h"
#include "Firework.h"

// Preclarify for preventing the compiler error
class TrainWindow;
class CTrack;


//#######################################################################
// TODO
// You might change the TrainView in order to add different objects to
// be drawn, or adjust the lighting, or ...
//#######################################################################


// The TrainView also needs to handle its user interface events.
// since this is an FLTk Window, we need to include FlTk Headers
// the pragmas turn off the warnings from FlTk - our code should have no warnings,
// but their code doesn't meet that standard
#pragma warning(push)
#pragma warning(disable:4312)
#pragma warning(disable:4311)
#include <Fl/Fl_Gl_Window.h>
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include "Utilities/Pnt3f.H"
#include <vector>
#include <chrono>
#include <cstdlib>
#pragma warning(pop)

// this uses the old ArcBall Code
#include "Utilities/ArcBallCam.H"

class TrainView : public Fl_Gl_Window
{
	public:
		// note that we keep the "standard widget" constructor arguments
		TrainView(int x, int y, int w, int h, const char* l = 0);

		void setObject(Object& obj);

		void drawFlag();
		void drawSkybox();
		void drawObject(Object obj, glm::vec3 scale_val, glm::vec3 trans_pos, bool transparency, glm::vec4 color, float time);
		void drawCups(glm::vec3 scale_val);
		void drawStreetlight(glm::vec3 scale_val, glm::vec3 trans_pos);
		void drawTrack(bool doingShadows);
		void drawTrain(bool doingShadows);
		void drawShops();

		// overrides of important window things
		virtual int handle(int);
		virtual void draw();

		// all of the actual drawing happens in this routine
		// it has to be encapsulated, since we draw differently if
		// we're drawing shadows (no colors, for example)
		void drawStuff(bool doingShadows=false);

		// setup the projection - assuming that the projection stack has been
		// cleared for you
		void setProjection();

		// Reset the Arc ball control
		void resetArcball();

		// pick a point (for when the mouse goes down)
		void doPick();

		//set ubo
		void setUBO();

		bool loadOBJ(
			const char* path,
			std::vector<glm::vec3>& out_vertices,
			std::vector<glm::vec2>& out_uvs,
			std::vector<glm::vec3>& out_normals);

		const std::vector<std::string> split(const std::string& str, const char& delimiter);

	public:
		ArcBallCam		arcball;			// keep an ArcBall for the UI
		int				selectedCube = -1;  // simple - just remember which cube is selected

		TrainWindow*	tw;				// The parent of this display window
		CTrack*			m_pTrack;		// The track of the entire scene

		std::vector<Pnt3f> point_list;
		Pnt3f dir_up{ 0, 1, 0 };

		Pnt3f train_pos;
		Pnt3f train_toward;
		Pnt3f train_left;

		Pnt3f train_pos2;
		Pnt3f train_toward2;
		Pnt3f train_left2;
		bool train_setup = false;

		Texture2D* texture	= nullptr;
		UBO* commom_matrices= nullptr;
		
		Drawable* flag = nullptr;
		Drawable* skybox = nullptr;

		Object* streetLight = nullptr;
		Object* light = nullptr;

		Cups* cups = nullptr;
		Object* cup_stage = nullptr;

		Firework* fireworks = nullptr;

		Object* house = nullptr;

		std::chrono::steady_clock::time_point start_time;
		float duration_time;
		std::string baseDIR;

		//OpenAL
		glm::vec3 source_pos;
		glm::vec3 listener_pos;
};