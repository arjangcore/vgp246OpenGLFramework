#include <stdio.h>
#include <string.h>
#include <math.h>

#ifdef WIN32
#include <windows.h>
#endif

#ifndef MACOSX
#include <GL/gl.h>
#include <GL/glu.h>
#else
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#endif

#include "fssimplewindow.h"
#include "bitmapfont/ysglfontdata.h"
#include <vector>
#include <ctime>
#include <random>
//#include "vector2d.h"
#include "vector3d.h"
#include  "matrices.h"
using namespace std;

typedef enum 
{
	eStop = -1,
	eIdle = 0,
	eStart = 1,
	eSpeedUp,
	eSpeedDown,
	eAngleUp,
	eAngleDown,
} changeType;

double friction = 0.1; // this is friction coefficient between the sphere and the slope.
double airResistance = 0.1; //
double gravity = 9.81; // m/s*s
double PI = 3.1415926;
double iAngle = PI / 3.; // projectile inclination angle in radian
double iSpeed = 25.0f;
int circleSections = 16;
const double angleInc = PI / 180.;

float eyeX = 25.0f, eyeY = 5.0f, eyeZ = 90.0f;

int winWidth = 800;
int winHeight = 600;
const double ratio = (float)winHeight / (float)winWidth;
const double WorldWidth = 120.0; // meter wide
const double WorldDepth = WorldWidth * ratio; // 
const double WorldHeight = 100.;
int width = 0, height = 0;

// draw the slop /////////
double radius = 1.;

static double clocktime = 0.f;
int framerate = 30;

const int TEXDIM = 1024;
GLfloat *tex;

bool checkWindowResize();

struct Object3D
{
	Vector3d<double> pos, vel, acc;
	int red, green, blue;
	
	double mass;
	void set(double x, double y, double z, double m, Vector3d<double> v, int r, int g, int b)
	{
		pos.x = x;
		pos.y = y;
		pos.z = z;
		vel = v;
		mass = m;
		green = r;
		red = g;
		blue = b;
	}

	////////////////////////////////////////////////////////////////
	void DrawAxis(float extend)
	{
		glPushMatrix();
		glLineWidth(3.f);
		glMatrixMode(GL_MODELVIEW);
		Matrix4 mModel, mView, mModelView;
		static float angle = 0.f;
		angle += 0.3;
		// set rotation matrix for the frame to be rotaton around z axis by angle degrees
		Vector3 T(cosf(angle), sinf(angle), 0);
		Vector3 N(-sinf(angle), cosf(angle), 0);
		Vector3 B(0.f, 0.f, 1.f);
		mView.setColumn(0, T);
		mView.setColumn(1, N);
		mView.setColumn(2, B);
		
		// set translation to move the frame to where the object is.
		mModel.translate(pos.x, pos.y, pos.z);
		mModelView = mModel * mView;
		// set the final matrix to be used as modelview matrix for opengl pipeline.
		glMultMatrixf(mModelView.get());

		glBegin(GL_LINES); 
		glColor3f(1.f, 0.f, 0.f); 
		glVertex3d(0.f, 0.f, 0.f);    // x axis
		glVertex3d(extend, 0.f, 0.f);
		glColor3f(0.f, 1.f, 0.f);
		glVertex3d(0.f, 0.f, 0.f);  // y axis
		glVertex3d(0.f, extend, 0.f);
		glColor3f(0.f, 0.f, 1.f);
		glVertex3d(0.f, 0.f, 0.f);  //z axis
		glVertex3d(0.f, 0.f, extend);
		glEnd();
		glPopMatrix();
	}

};
Object3D simBall;


/* Create a single component texture map */
GLfloat *make_texture(int maxs, int maxt)
{
	int s, t;
	static GLfloat *texture;

	texture = (GLfloat *)malloc(maxs * maxt * sizeof(GLfloat));
	for (t = 0; t < maxt; t++) {
		for (s = 0; s < maxs; s++) {
			texture[s + maxs * t] = (((s >> 4) & 0x1) ^ ((t >> 4) & 0x1));
		}
	}
	return texture;
}


///////////////////////////////////////////////////////////////
void initPhysics(double rad, double speed, double angle)
{
	clocktime = 0.f;
	double vx = speed * cos(angle);
	double vy = speed * sin(angle);
	double vz =0.f;
	double initX = rad;
	double inity = rad;
	double initz = WorldDepth/4.f;
	simBall.set(initX, inity, initz, 2, Vector3d<double>(vx, vy, vz), 128, 128, 0);
}

int PollKeys()
{
	FsPollDevice();
	int keyRead = FsInkey();
	switch (keyRead)
	{
	case FSKEY_S:
		keyRead = eStart;
		break;
	case FSKEY_ESC:
		keyRead = eStop;
		break;
	case FSKEY_UP:
		friction += 0.1;
		eyeY += 0.6;
		break;
	case FSKEY_DOWN:
		friction = max(1., friction - 0.1);
		eyeY -= 0.6;
		break;
	case FSKEY_LEFT:
		iAngle = max(0., iAngle - angleInc);
		eyeX -= 0.6;
		break;
	case FSKEY_RIGHT:
		iAngle = min(90.0, iAngle + angleInc);
		eyeX += 0.6;
		break;
	case FSKEY_PAGEDOWN:
		iSpeed = max(iSpeed - 5.0, 0.0);
		break;
	case FSKEY_PAGEUP:
		iSpeed = min(iSpeed + 5.0, 100.);
		break;
	case FSKEY_I:
		eyeY = min(eyeY + 1.0, 100.0);
		break;
	case FSKEY_K:
		eyeY = max(eyeY - 1.0, 1.);
		break;
	}
	return keyRead;

}
//////////////////////////////////////////////////////////////////////////////////////////////
int Menu(void)
{
	int key = eIdle;
	FsGetWindowSize(width, height);
	glDisable(GL_DEPTH_TEST);

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-0.5, (GLdouble)width - 0.5, (GLdouble)height - 0.5, -0.5, -1, 1);

	glClearColor(0.0, 0.0, 0.0, 0.0);
	
	while (key != eStart && key != eStop)
	{
		key = PollKeys();
		if (key == eStop)
			return key;

		glClear(GL_COLOR_BUFFER_BIT);

		// printing UI message info
		glColor3f(1., 1., 1.);
		char msg[128];
		sprintf_s(msg, "Friction is %f. Use Up/Down keys to change it by 1/10!\n", friction);
		glRasterPos2i(32, 32);
		glCallLists(strlen(msg), GL_UNSIGNED_BYTE, msg);

		sprintf_s(msg, "Slope Angle is %f degrees. Use Left/Right keys to change it!\n", iAngle*180. / PI);
		glRasterPos2i(32,64);
		glCallLists(strlen(msg),GL_UNSIGNED_BYTE, msg);

		sprintf_s(msg, "Projectile speed is %f m/s. Use PageUp/PageDown keys to change it!\n", iSpeed);
		glRasterPos2i(32, 96);
		glCallLists(strlen(msg), GL_UNSIGNED_BYTE, msg);

		sprintf_s(msg, "Camera height is %f. Use I/K keys to change it!\n", eyeY);
		glRasterPos2i(32, 128);
		glCallLists(strlen(msg), GL_UNSIGNED_BYTE, msg);

		const char *msg1 = "S.....Start Game";
		const char *msg2="ESC...Exit";
		glRasterPos2i(32, 160);
		glCallLists(strlen(msg1),GL_UNSIGNED_BYTE, msg1);
		glRasterPos2i(32,192);
		glCallLists(strlen(msg2),GL_UNSIGNED_BYTE, msg2);

		FsSwapBuffers();
		FsSleep(10);
	}

	initPhysics(radius, iSpeed, iAngle);
	return key;
}

///////////////////////////////////////////////////////////////
int timeSpan = 33; // milliseconds
double timeInc = (double)timeSpan * 0.001; // time increment in seconds

///////////////////////////////////////////////////////////////////////////////////////////
void renderScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	checkWindowResize();

	// set the camera
	gluLookAt(	eyeX, eyeY, eyeZ,
				25., 0., 0., //camX + 0.f, camY, camZ - 10.f,
				0.0f, 1.0f, 0.0f);
	

	//////////////////// draw the ground ///////////////
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	glColor3f(1.f, 1.f, 1.f);
	glNormal3f(0.f, 1.f, 0.f);
	glTexCoord2i(0, 0);
	glVertex3f(0.f, 0.f, 0.f);
	glTexCoord2i(1, 0);
	glVertex3f(WorldWidth, 0.f, 0.f);
	glTexCoord2i(1, 1);
	glVertex3f(WorldWidth, 0.f, WorldHeight);
	glTexCoord2i(0, 1);
	glVertex3f(0.f, 0.f, WorldHeight);
	glEnd();
	glDisable(GL_TEXTURE_2D);

	simBall.DrawAxis(2.f);

	FsSwapBuffers();
}
/////////////////////////////////////////////////////////////////////
void updatePhysics(Object3D &ball, double timeInc)
{
	//////////// your physics goes here //////////////////////////
	// we use a coordinate system in which x goes from left to right of the screen and y goes from bottom to top of the screen
	// we have 1 forces here: 1) gravity which is in negative y direction. 
	//////////////Explicit Euler Integration:///////////////////////
	ball.pos.x += ball.vel.x * timeInc; // x position update, x speed is constant.
	ball.pos.y += ball.vel.y * timeInc; // y position update
	ball.pos.z += ball.vel.z * timeInc; // y position update

	ball.vel.y -= gravity * timeInc; // y speed update
}

void resetPhysics()
{
	initPhysics(radius, iSpeed, iAngle);
}

bool checkWindowResize()
{
	int wid, hei;
	FsGetWindowSize(wid, hei);
	if (wid != width || hei != height)
	{
		width = wid; height = hei;
		glViewport(0, 0, width, height);
		return true;
	}
	return false;
}


///////////////////////////////////////////////////////////////////
int Game(void)
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	/* load pattern for current 2d texture */
	tex = make_texture(TEXDIM, TEXDIM);
	glTexImage2D(GL_TEXTURE_2D, 0, 1, TEXDIM, TEXDIM, 0, GL_RED, GL_FLOAT, tex);
	free(tex);

//	int lb, mb, rb, mx, my;
	DWORD passedTime = 0;
	FsPassedTime(true);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	//////////// initial setting up the scene ////////////////////////////////////////
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	checkWindowResize();

	// set the camera
	float ratio = width / height;
	gluPerspective(45.f, ratio, 0.1f, 150.f);
	
	int key = eIdle;

	glMatrixMode(GL_MODELVIEW);
	bool resetFlag = false;
	while(1)
	{
		if (checkWindowResize())
		{
			float ratio = width / height;
			gluPerspective(45.f, ratio, 0.1f, 100.f);

		}
		//FsGetMouseState(lb,mb,rb,mx,my);
		key = PollKeys(); 
		if(key == eStop)
			break;
		if (key == eStart)
			resetFlag = false;

		timeInc = (double)(passedTime) * 0.001;
		clocktime += timeInc;
		/////////// update physics /////////////////
		if (simBall.pos.y < -0.01)
		{
			resetPhysics();
			resetFlag = true;
		}
		
		if (!resetFlag)
			updatePhysics(simBall, timeInc);
		/////////////////////////////////////////
		renderScene();

		////// update time lapse /////////////////
		passedTime = FsPassedTime(); // Making it up to 50fps
		int timediff = timeSpan-passedTime;
	//	printf("\ntimeInc=%f, passedTime=%d, timediff=%d", timeInc, passedTime, timediff);
		while(timediff >= timeSpan/3)
		{
			FsSleep(5);
			passedTime=FsPassedTime(); // Making it up to 50fps
			timediff = timeSpan-passedTime;
	//		printf("--passedTime=%d, timediff=%d", passedTime, timediff);
		}
		passedTime=FsPassedTime(true); // Making it up to 50fps
	}
	return key;
}

//////////////////////////////////////////////////////////////////////////////////////
int main(void)
{
	int menu;
	FsOpenWindow(32, 32, winWidth, winHeight, 1); // 800x600 pixels, useDoubleBuffer=1

	int listBase = glGenLists(256);
	YsGlUseFontBitmap8x12(listBase);
	glListBase(listBase);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	menu=Menu();
	if (Menu() != eStop)
		Game();
	
	return 0;
}


