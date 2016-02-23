#include <stdio.h>
#include <string.h>
#include <math.h>
#include <vector>

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

float PI = 3.1415926;
float iAngle = PI / 3.; // projectile inclination angle in radian
float iSpeed = 25.0f;
int circleSections = 16;
const float angleInc = PI / 180.f;

float eyeX = 25.0f, eyeY = 10.0f, eyeZ = 100.0f;

int winWidth = 800;
int winHeight = 600;
const float ratio = (float)winHeight / (float)winWidth;
const float WorldWidth = 120.0; // meter wide
const float WorldDepth = WorldWidth * ratio; // 
const float WorldHeight = 100.;
int width = 0, height = 0;

static float clocktime = 0.f;
int framerate = 30;

bool checkWindowResize();
typedef Vector3d<float> MathVec;
struct TracePoint
{
	MathVec position;
	MathVec color;
};
struct Object3D
{
	MathVec pos, vel, acc;
	int red, green, blue;

	float mass;
	static const size_t maxPointCount = 400;
	TracePoint posTrace[maxPointCount];
	int traceCount;
	TracePoint &SetNextTracePoint()
	{
		posTrace[traceCount].position = pos;
		posTrace[traceCount].color = MathVec(255, 255, 255) - vel;
		return posTrace[traceCount++];
	}
	Object3D() { traceCount = 0; }
	void set(float x, float y, float z, float m, MathVec v, int r, int g, int b)
	{
		pos.x = x;
		pos.y = y;
		pos.z = z;
		vel = v;
		mass = m;
		green = r;
		red = g;
		blue = b;
		traceCount = 0;
	}

	void DrawTrace()
	{
		glPushMatrix();
		glLineWidth(2.f);
		glBegin(GL_POINTS);
		for (int i = 0; i < traceCount; i++)
		{
			auto &tp = posTrace[i];
			glColor3i(tp.color.x, tp.color.y, tp.color.z);
			glVertex3fv((float*)&tp.position);
		}
		glEnd();
		glPopMatrix();
	}
	////////////////////////////////////////////////////////////////
	void DrawAxis(float extend, bool running = false)
	{
		glPushMatrix();
		glPointSize(3.f);
		glMatrixMode(GL_MODELVIEW);
		Matrix4 mModel, mView, mModelView;
		static float angle = 0.f;
		if (running)
			angle += 0.3f;
		else
			angle = 0.f;
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
		glDisable(GL_LIGHTING);
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
		glEnable(GL_LIGHTING);

		glPopMatrix();
	}

};
Object3D simBall;

/* material properties for objects in scene */
static GLfloat wall_mat[] = { 1.f, 1.f, 1.f, 1.f };

/* Create a single component texture map */
GLfloat *make_texture(int maxs, int maxt)
{
	int s, t;
	static GLfloat *texture;

	texture = (GLfloat *)malloc(maxs * maxt * sizeof(GLfloat));
	for (t = 0; t < maxt; t++) {
		for (s = 0; s < maxs; s++) {
			texture[s + maxs * t] = 1.f - (((s >> 4) & 0x1) ^ ((t >> 4) & 0x1))*0.3f;
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
	double vz = 0.f;
	double initX = rad;
	double inity = rad;
	double initz = WorldDepth / 4.f;
	simBall.set(initX, inity, initz, 2, MathVec(vx, vy, vz), 128, 128, 0);
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
		//friction += 0.1;
		eyeY += 0.6f;
		break;
	case FSKEY_DOWN:
		//	friction = max(1., friction - 0.1);
		eyeY -= 0.6f;
		break;
	case FSKEY_LEFT:
		iAngle = max(0., iAngle - angleInc);
		eyeX -= 0.6f;
		break;
	case FSKEY_RIGHT:
		iAngle = min(90.0f, iAngle + angleInc);
		eyeX += 0.6f;
		break;
	case FSKEY_PAGEDOWN:
		iSpeed = max(iSpeed - 5.0f, 0.0f);
		break;
	case FSKEY_PAGEUP:
		iSpeed = min(iSpeed + 5.0f, 100.f);
		break;
	case FSKEY_I:
		eyeY = min(eyeY + 1.0f, 100.0f);
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
		//sprintf_s(msg, "Friction is %f. Use Up/Down keys to change it by 1/10!\n", friction);
		//glRasterPos2i(32, 32);
		//glCallLists(strlen(msg), GL_UNSIGNED_BYTE, msg);

		sprintf_s(msg, "Slope Angle is %f degrees. Use Left/Right keys to change it!\n", iAngle*180. / PI);
		glRasterPos2i(32, 64);
		glCallLists(strlen(msg), GL_UNSIGNED_BYTE, msg);

		sprintf_s(msg, "Projectile speed is %f m/s. Use PageUp/PageDown keys to change it!\n", iSpeed);
		glRasterPos2i(32, 96);
		glCallLists(strlen(msg), GL_UNSIGNED_BYTE, msg);

		sprintf_s(msg, "Camera height is %f. Use I/K keys to change it!\n", eyeY);
		glRasterPos2i(32, 128);
		glCallLists(strlen(msg), GL_UNSIGNED_BYTE, msg);

		const char *msg1 = "S.....Start Game";
		const char *msg2 = "ESC...Exit";
		glRasterPos2i(32, 160);
		glCallLists(strlen(msg1), GL_UNSIGNED_BYTE, msg1);
		glRasterPos2i(32, 192);
		glCallLists(strlen(msg2), GL_UNSIGNED_BYTE, msg2);

		FsSwapBuffers();
		FsSleep(10);
	}

	initPhysics(1.f, iSpeed, iAngle);
	return key;
}

///////////////////////////////////////////////////////////////
int timeSpan = 33; // milliseconds
double timeInc = (double)timeSpan * 0.001; // time increment in seconds

///////////////////////////////////////////////////////////////////////////////////////////
void renderScene(bool reset)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	checkWindowResize();

	// set the camera
	gluLookAt(eyeX, eyeY, eyeZ, 20., 0., 0., 0.0f, 1.0f, 0.0f);

	//////////////////// draw the ground ///////////////
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	glColor3f(1.f, 1.f, 1.f);
	glNormal3f(0.f, 1.f, 0.f);
	glTexCoord2i(0, 0);
	glVertex3f(0.f, 0.f, 0.f);
	glTexCoord2i(2, 0);
	glVertex3f(WorldWidth, 0.f, 0.f);
	glTexCoord2i(2, 2);
	glVertex3f(WorldWidth, 0.f, WorldHeight);
	glTexCoord2i(0, 2);
	glVertex3f(0.f, 0.f, WorldHeight);
	glEnd();
	glDisable(GL_TEXTURE_2D);

	// draw the frame and its trace:
	simBall.DrawAxis(2.f, reset);
	simBall.DrawTrace();

	// draw walls:
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, wall_mat);
	glBegin(GL_QUADS);
	/* left wall */
	glNormal3f(1.f, 0.f, 0.f);
	glVertex3f(0.f, 0.f, WorldHeight);
	glVertex3f(0.f, 0.f, 0.f);
	glVertex3f(0.f, 100.f, 0.f);
	glVertex3f(0.f, 100.f, WorldHeight);

	/* ceiling */
	glNormal3f(0.f, -1.f, 0.f);
	glVertex3f(0.f, 100.f, WorldHeight / 2.);
	glVertex3f(0.f, 100.f, 0.f);
	glVertex3f(WorldWidth, 100.f, 0.f);
	glVertex3f(WorldWidth, 100.f, WorldHeight / 2.);

	/* back wall */
	glNormal3f(0.f, 0.f, 1.f);
	glVertex3f(0.f, 0.f, 0.f);
	glVertex3f(WorldWidth, 0.f, 0.f);
	glVertex3f(WorldWidth, 100.f, 0.f);
	glVertex3f(0.f, 100.f, 0.f);

	glEnd();

	FsSwapBuffers();
}

const float gravity = 9.81f;
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

	ball.vel.y -= gravity * timeInc;    // y speed update

	//update trace
	TracePoint &tp = ball.SetNextTracePoint();
	std::cout << "Pos(" << ball.traceCount << ":" << ball.pos.x << "," << ball.pos.y << "," << ball.pos.z << "==" << tp.position.x << "," << tp.position.y << "," << tp.position.z << std::endl;
}

void resetPhysics()
{
	initPhysics(1.f, iSpeed, iAngle);
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
	/* turn on features */
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	/* place light 0 in the right place */
	GLfloat lightpos[] = { WorldWidth / 2.f, 50.f, WorldHeight / 2.f, 1.f };
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

	/* remove back faces to speed things up */
	glCullFace(GL_BACK);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	/* load pattern for current 2d texture */
	const int TEXDIM = 256;
	GLfloat *tex = make_texture(TEXDIM, TEXDIM);
	glTexImage2D(GL_TEXTURE_2D, 0, 1, TEXDIM, TEXDIM, 0, GL_RED, GL_FLOAT, tex);
	free(tex);

	//	int lb, mb, rb, mx, my;
	DWORD passedTime = 0;
	FsPassedTime(true);

	//////////// initial setting up the scene ////////////////////////////////////////
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	checkWindowResize();

	// set the camera
	float ratio = (float)width / (float)height;
	gluPerspective(45.f, ratio, 0.1f, 150.f);
	int key = eIdle;

	glMatrixMode(GL_MODELVIEW);
	bool resetFlag = false;
	while (1)
	{
		if (checkWindowResize())
		{
			ratio = (float)width / (float)height;
			gluPerspective(50.f, ratio, 0.1f, 100.f);

		}
		//FsGetMouseState(lb,mb,rb,mx,my);
		key = PollKeys();
		if (key == eStop)
			break;
		if (key == eStart)
			resetFlag = false;

		timeInc = (double)(passedTime)* 0.001;
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
		renderScene(!resetFlag);

		////// update time lapse /////////////////
		passedTime = FsPassedTime(); // Making it up to 50fps
		int timediff = timeSpan - passedTime;
		//	printf("\ntimeInc=%f, passedTime=%d, timediff=%d", timeInc, passedTime, timediff);
		while (timediff >= timeSpan / 3)
		{
			FsSleep(5);
			passedTime = FsPassedTime(); // Making it up to 50fps
			timediff = timeSpan - passedTime;
			//		printf("--passedTime=%d, timediff=%d", passedTime, timediff);
		}
		passedTime = FsPassedTime(true); // Making it up to 50fps
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

	menu = Menu();
	if (Menu() != eStop)
		Game();

	return 0;
}

