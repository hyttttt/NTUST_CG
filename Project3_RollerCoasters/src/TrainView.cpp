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
#include "GL/glu.h"

#include "TrainView.H"
#include "TrainWindow.H"
#include "Utilities/3DUtils.H"


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
		//initiailize VAO, VBO, Shader...
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
		//glEnable(GL_LIGHT2);
	}

	//*********************************************************************
	//
	// * set the light parameters
	//
	//**********************************************************************
	GLfloat lightPosition1[] = { 0, 1, 1, 0 }; // {50, 200.0, 50, 1.0};
	GLfloat lightPosition2[] = { 1, 0, 0, 0 };
	GLfloat lightPosition3[] = { 0, -1, 0, 0 };
	GLfloat yellowLight[] = { 1.0f, 1.0f, 0.5f, 1.0 };
	GLfloat whiteLight[] = { 1.0f, 1.0f, 1.0f, 1.0 };
	GLfloat blueLight[] = { .1f,.1f,.3f,1.0 };
	GLfloat grayLight[] = { .3f, .3f, .3f, 1.0 };

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
	drawFloor(200,10);


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
		glRotatef(-90, 1, 0, 0);
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

void TrainView::drawHouse(bool doingShadows, Pnt3f house_pos, float house_color[3], float roof_color[3], float rotate_angle) {
	// coordinate system
	Pnt3f up(0, 1, 0);
	Pnt3f left(1, 0, 0);
	Pnt3f front(0, 0, 1);

	left = cos(rotate_angle) * left - sin(rotate_angle) * front;
	front = sin(rotate_angle) * left + cos(rotate_angle) * front;

	// house information
	float house_width = 25;
	float house_height = house_width;
	float house_depth = house_width * 1.5;

	Pnt3f house_p0 = house_pos;

	Pnt3f house_p1 = house_pos
		+ house_width * left;

	Pnt3f house_p2 = house_pos
		+ house_width * left
		+ house_depth * front;

	Pnt3f house_p3 = house_pos
		+ house_depth * front;

	Pnt3f house_p4 = house_pos
		+ house_depth * front
		+ house_height * up;

	Pnt3f house_p5 = house_pos
		+ house_width * left
		+ house_depth * front
		+ house_height * up;

	Pnt3f house_p6 = house_pos
		+ house_width * left
		+ house_height * up;

	Pnt3f house_p7 = house_pos
		+ house_height * up;

	// tunnel information
	float tunnel_width = house_width * 2 / 3;
	float tunnel_height = house_height * 3 / 4;

	// front tunnel
	Pnt3f tunnel_p0 = house_p2
		- (house_width - tunnel_width) / 2 * left;

	Pnt3f tunnel_p1 = tunnel_p0
		+ tunnel_height * up;

	Pnt3f tunnel_p2 = tunnel_p1
		- tunnel_width * left;

	Pnt3f tunnel_p3 = tunnel_p2
		- tunnel_height * up;

	Pnt3f tunnel_p4 = tunnel_p2
		- (house_width - tunnel_width) / 2 * left;

	Pnt3f tunnel_p5 = house_p2
		+ tunnel_height * up;

	// back tunnel
	Pnt3f tunnel_p6 = tunnel_p0
		- house_depth * front;

	Pnt3f tunnel_p7 = tunnel_p1
		- house_depth * front;

	Pnt3f tunnel_p8 = tunnel_p2
		- house_depth * front;

	Pnt3f tunnel_p9 = tunnel_p3
		- house_depth * front;

	Pnt3f tunnel_p10 = tunnel_p4
		- house_depth * front;

	Pnt3f tunnel_p11 = tunnel_p5
		- house_depth * front;

	// draw house
	if(!doingShadows)
		glColor3fv(house_color);
	glBegin(GL_QUADS);

		// front surface // right part
		glVertex3f(tunnel_p2.x, tunnel_p2.y, tunnel_p2.z);
		glVertex3f(tunnel_p3.x, tunnel_p3.y, tunnel_p3.z);
		glVertex3f(house_p3.x, house_p3.y, house_p3.z);
		glVertex3f(tunnel_p4.x, tunnel_p4.y, tunnel_p4.z);

		// front surface // left part
		glVertex3f(tunnel_p0.x, tunnel_p0.y, tunnel_p0.z);
		glVertex3f(tunnel_p1.x, tunnel_p1.y, tunnel_p1.z);
		glVertex3f(tunnel_p5.x, tunnel_p5.y, tunnel_p5.z);
		glVertex3f(house_p2.x, house_p2.y, house_p2.z);
		
		// front surface // up part
		glVertex3f(house_p4.x, house_p4.y, house_p4.z);
		glVertex3f(house_p5.x, house_p5.y, house_p5.z);
		glVertex3f(tunnel_p5.x, tunnel_p5.y, tunnel_p5.z);
		glVertex3f(tunnel_p4.x, tunnel_p4.y, tunnel_p4.z);
		

		// back surface // right part
		glVertex3f(tunnel_p8.x, tunnel_p8.y, tunnel_p8.z);
		glVertex3f(tunnel_p9.x, tunnel_p9.y, tunnel_p9.z);
		glVertex3f(house_p0.x, house_p0.y, house_p0.z);
		glVertex3f(tunnel_p10.x, tunnel_p10.y, tunnel_p10.z);

		// back surface // left part
		glVertex3f(tunnel_p6.x, tunnel_p6.y, tunnel_p6.z);
		glVertex3f(tunnel_p7.x, tunnel_p7.y, tunnel_p7.z);
		glVertex3f(tunnel_p11.x, tunnel_p11.y, tunnel_p11.z);
		glVertex3f(house_p1.x, house_p1.y, house_p1.z);

		// back surface // up part
		glVertex3f(house_p6.x, house_p6.y, house_p6.z);
		glVertex3f(house_p7.x, house_p7.y, house_p7.z);
		glVertex3f(tunnel_p10.x, tunnel_p10.y, tunnel_p10.z);
		glVertex3f(tunnel_p11.x, tunnel_p11.y, tunnel_p11.z);

		// left surface
		glVertex3f(house_p1.x, house_p1.y, house_p1.z);
		glVertex3f(house_p2.x, house_p2.y, house_p2.z);
		glVertex3f(house_p5.x, house_p5.y, house_p5.z);
		glVertex3f(house_p6.x, house_p6.y, house_p6.z);

		// right surface
		glVertex3f(house_p0.x, house_p0.y, house_p0.z);
		glVertex3f(house_p3.x, house_p3.y, house_p3.z);
		glVertex3f(house_p4.x, house_p4.y, house_p4.z);
		glVertex3f(house_p7.x, house_p7.y, house_p7.z);

	glEnd();

	// roof information
	float roof_width = house_width * 1.5;
	float roof_height = roof_width * 0.5;
	float roof_depth = house_depth;

	Pnt3f roof_pos = house_pos
		- (roof_width - house_width) * left * 0.5
		+ house_height * up;

	Pnt3f roof_p0 = roof_pos;

	Pnt3f roof_p1 = roof_pos
		+ roof_width * left;

	Pnt3f roof_p2 = roof_pos
		+ roof_width * left
		+ roof_depth * front;

	Pnt3f roof_p3 = roof_pos
		+ roof_depth * front;

	Pnt3f roof_p4 = roof_pos
		+ roof_width * left * 0.5
		+ roof_depth * front
		+ roof_height * up;

	Pnt3f roof_p5 = roof_pos
		+ roof_width * left * 0.5
		+ roof_height * up;

	// draw roof
	if (!doingShadows)
		glColor3fv(roof_color);
	glBegin(GL_QUADS);
		
		// bottom surface
		glVertex3f(roof_p0.x, roof_p0.y, roof_p0.z);
		glVertex3f(roof_p1.x, roof_p1.y, roof_p1.z);
		glVertex3f(roof_p2.x, roof_p2.y, roof_p2.z);
		glVertex3f(roof_p3.x, roof_p3.y, roof_p3.z);

		// right surface
		glVertex3f(roof_p0.x, roof_p0.y, roof_p0.z);
		glVertex3f(roof_p3.x, roof_p3.y, roof_p3.z);
		glVertex3f(roof_p4.x, roof_p4.y, roof_p4.z);
		glVertex3f(roof_p5.x, roof_p5.y, roof_p5.z);

		// left surface
		glVertex3f(roof_p1.x, roof_p1.y, roof_p1.z);
		glVertex3f(roof_p2.x, roof_p2.y, roof_p2.z);
		glVertex3f(roof_p4.x, roof_p4.y, roof_p4.z);
		glVertex3f(roof_p5.x, roof_p5.y, roof_p5.z);
	glEnd();

	glBegin(GL_TRIANGLES);

		// front surface
		glVertex3f(roof_p2.x, roof_p2.y, roof_p2.z);
		glVertex3f(roof_p3.x, roof_p3.y, roof_p3.z);
		glVertex3f(roof_p4.x, roof_p4.y, roof_p4.z);

		// back surface
		glVertex3f(roof_p0.x, roof_p0.y, roof_p0.z);
		glVertex3f(roof_p1.x, roof_p1.y, roof_p1.z);
		glVertex3f(roof_p5.x, roof_p5.y, roof_p5.z);
	glEnd();
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
	// don't draw the train if you're looking out the front window
	if (!tw->trainCam->value())
		drawTrain(doingShadows);

	// draw first house
	float white[3] = { 255,255,255 };
	float red[3] = { 255,0,0 };
	drawHouse(doingShadows, Pnt3f(50, 0, 50), white, red, 0);

	// draw second house
	float blue[3] = { 0, 0, 255 };
	drawHouse(doingShadows, Pnt3f(-100, 0, -50), white, blue, 0.5);
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