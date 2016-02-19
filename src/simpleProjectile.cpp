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

double gravity = -9.81; // m/s*s
double PI = 3.1415926;
double iAngle = PI / 3.; // projectile inclination angle in radian
double iSpeed = 15.0;  // initial ball speed
double radius = 0.4;
int circleSections = 10;

const int winWidth = 640;
const int winHeight = 480;
float ratio = (float)winHeight / (float)winWidth;
float WorldWidth = 50.0; // 50 meter wide
float WorldHeight = WorldWidth * ratio; // 
int width = 0, height = 0;

static double clocktime = 0.f;

struct Circle2D
{
	double cx, cy, vx, vy, radius;
	double cix, ciy, vix, viy;
	int red, green, blue;
	
	double mass;
	void set(double x, double y, double rad, double m, double v_x, double v_y, int r, int g, int b)
	{
		radius = rad;
		cix = cx = rad;
		ciy = cy = rad;
		mass = m;
		vix = vx = v_x;
		viy = vy = v_y;
		green = r;
		red = g;
		blue = b;

	}


	// draws a hollow circle centered at (cx, cy) and with radius r, using num_segments triangle segments.
	// It rotates the circle by iAngle as well.
	void DrawCircle(double angle, int num_segments)
	{
		double theta = 2. * PI / double(num_segments);
		double c = cos(theta);//precalculate the sine and cosine
		double s = sin(theta);

		double x = radius;//we start at angle = 0 
		double y = 0.;
		double t = x;
		x = cos(angle) * x - sin(angle) * y;
		y = sin(angle) * t + cos(angle) * y;

		glLineWidth(2);
		glBegin(GL_LINE_LOOP);
		glColor3ub(red, green, blue);
		for (int i = 0; i < num_segments; i++)
		{
			glVertex2d(x + cx, y + cy);

			//apply the rotation matrix
			t = x;
			x = c * x - s * y;
			y = s * t + c * y;
		}
		glEnd();
	}

	// draws a solid circle centered at (cx, cy) and with radius r, using num_segments triangle segments.
	// It rotates the circle by iAngle as well.
	void DrawSolidCircle(double angle, int num_segments)
	{
		double theta = 2. * PI / double(num_segments);
		double c = cos(theta);//precalculate the sine and cosine
		double s = sin(theta);

		double x = radius;//we start at angle = 0 
		double y = 0;
		double t = x;
		double sx = cos(angle) * x - sin(angle) * y;
		double sy = sin(angle) * t + cos(angle) * y;
		x = sx; y = sy;

		glLineWidth(2);
		glBegin(GL_TRIANGLE_FAN);
		glColor3ub(red, green, blue);
		glVertex2d(cx, cy);
		for (int i = 0; i < num_segments; i++)
		{
			glVertex2d(x + cx, y + cy);//output vertex 

			//apply the rotation matrix
			t = x;
			x = c * x - s * y;
			y = s * t + c * y;
		}

		glVertex2d(sx + cx, sy + cy);
		glEnd();
	}
};
Circle2D simBall1, realBall;

void initPhysics(double rad, double speed, double angle);

//////////////////////////////////////////////////////////////////////////////////////////////
int Menu(void)
{
	int r=0,key;
	const double angleInc = PI/180.;
	while(r!=eStart && r!= eStop)
	{
		FsPollDevice();
		key=FsInkey();
		switch(key)
		{
		case FSKEY_S:
			r=eStart;
			break;
		case FSKEY_ESC:
			r=eStop;
			break;
		case FSKEY_UP:
			iSpeed++;
			break;
		case FSKEY_DOWN:
			iSpeed = max(2., iSpeed-1);
			break;
		case FSKEY_LEFT:
			iAngle = max(0., iAngle-angleInc);
			break;
		case FSKEY_RIGHT:
			iAngle = min(90.0, iAngle+angleInc);
			break;
		}

		initPhysics(radius, iSpeed, iAngle);
		if (r == eStop)
			return r;
		int wid,hei;
		FsGetWindowSize(wid,hei);


		glViewport(0,0,wid,hei);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-0.5,(GLdouble)wid-0.5,(GLdouble)hei-0.5,-0.5,-1,1);

		glClearColor(0.0,0.0,0.0,0.0);
		glClear(GL_COLOR_BUFFER_BIT);
		glColor3ub(127,127,127);

		char sSpeed[128];
		sprintf(sSpeed, "Initial ball speed is %f m/s. Use Up/Down keys to change it!\n", iSpeed);
		char sAngle[128];
		sprintf(sAngle, "Initial horizon Angle is %f degrees. Use Left/Right keys to change it!\n", iAngle*180./PI);
		glColor3ub(255,255,255);
		glRasterPos2i(32,32);
		glCallLists(strlen(sSpeed),GL_UNSIGNED_BYTE,sSpeed);
		glRasterPos2i(32,64);
		glCallLists(strlen(sAngle),GL_UNSIGNED_BYTE,sAngle);
		const char *msg1="S.....Start Game";
		const char *msg2="ESC...Exit";
		glRasterPos2i(32,96);
		glCallLists(strlen(msg1),GL_UNSIGNED_BYTE,msg1);
		glRasterPos2i(32,128);
		glCallLists(strlen(msg2),GL_UNSIGNED_BYTE,msg2);

		FsSwapBuffers();
		FsSleep(10);
	}
	return r;
}

///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////

void renderScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0., WorldWidth, 0., (GLdouble)WorldHeight);

	////////////////////////// Drawing The Coordinate Plane Starts Here.
	// We Will Draw Horizontal And Vertical Lines With A Space Of 1 Meter Between Them.
/*	glColor3ub(0, 0, 255);										// Draw In Blue
	glBegin(GL_LINES);
	glLineWidth(0.5);
	// Draw The Vertical Lines
	for (float x = 0.; x <= WorldWidth; x += 1.0f)						// x += 1.0f Stands For 1 Meter Of Space In This Example
	{
		glVertex3f(x, WorldHeight, 0);
		glVertex3f(x, -WorldHeight, 0);
	}

	// Draw The Horizontal Lines
	for (float y = 0.; y <= WorldHeight; y += 1.0f)						// y += 1.0f Stands For 1 Meter Of Space In This Example
	{
		glVertex3f(0, y, 0);
		glVertex3f(WorldWidth, y, 0);
	}
	glEnd(); */
	/////////////////////////// Drawing The Coordinate Plane Ends Here.

	/////////////////////////draw the hallow 2d disc /////////////

	simBall1.DrawCircle(0, circleSections);
	realBall.DrawCircle(0, circleSections);

	///////////// draw the overlay HUD /////////////////////
	glColor3ub(127, 127, 127);
	char str[256];
	sprintf(str, "simBall1: pos(%f, %f), velocity(%f, %f)", simBall1.cx, simBall1.cy, simBall1.vx, simBall1.vy);
	glRasterPos2i(32, height-32);
	glCallLists(strlen(str), GL_UNSIGNED_BYTE, str);
	sprintf(str, "real: pos(%f, %f), velocity(%f, %f)", realBall.cx, realBall.cy, realBall.vx, realBall.vy);
	glRasterPos2i(32, height - 64);
	glCallLists(strlen(str), GL_UNSIGNED_BYTE, str);

	FsSwapBuffers();
}
/////////////////////////////////////////////////////////////////////
void updateNumPhysics(Circle2D &ball, double timeInc)
{
	//////////// your physics goes here //////////////////////////
	// we use a coordinate system in which x goes from left to right of the screen and y goes from top to bottom of the screen
	// we have 1 forces here: 1) gravity which is in positive y direction. 
	//////////////Explicit Euler Integration:///////////////////////
	ball.cx += ball.vx * timeInc; // x position update, x speed is constant.

	ball.vy += gravity * timeInc; // y speed update
	ball.cy += ball.vy * timeInc; // y position update

	/////////////////////check edge collision ////////////////////////////////////////
	if (ball.cx<0 && ball.vx <0)
	{
		ball.cx = -ball.cx;
		ball.vx = -ball.vx;
	}
	if (ball.cy<0 && ball.vy<0)
	{
		ball.cy = -ball.cy;
		ball.vy = -ball.vy;
	}
	if (ball.cx>WorldWidth && ball.vx>0.f)
	{
		ball.cx = WorldWidth - (ball.cx - WorldWidth);
		ball.vx = -ball.vx;
	}
	if (ball.cy > WorldHeight && ball.vy>0.f)
	{
		ball.cy = WorldHeight - (ball.cy - WorldHeight);
		ball.vy = -ball.vy;
	}
}
/////////////////////////////////////////////////////////////////////////
void updatePrecisePhysics(Circle2D &ball, double timeInc)
{
	double ax = 0;
	ball.cx = ball.cix + ball.vix * timeInc; // +0.5 * ax * time * time; // x position update, x speed is constant.

	ball.vy = ball.viy + gravity * timeInc; // y speed update
	ball.cy = ball.ciy + ball.viy * timeInc + 0.5 * gravity * timeInc * timeInc; // y position update
	
	/////////////////////check edge collision ////////////////////////////////////////
	if (ball.cx<0.f && ball.vx <0.f)
	{
		ball.cx = -ball.cx;
		ball.vx = -ball.vx;
	}
	if (ball.cy<0 && ball.vy<0.f)
	{
		ball.cy = -ball.cy;
		ball.vy = -ball.vy;
	}
	if (ball.cx>WorldWidth && ball.vx>0.f)
	{
		ball.cx = WorldWidth - (ball.cx - WorldWidth);
		ball.vx = -ball.vx;
	}
	if (ball.cy > WorldHeight && ball.vy>0.f)
	{
		ball.cy = WorldHeight - (ball.cy - WorldHeight);
		ball.vy = -ball.vy;
	}

	// update initial data for next frame.
	ball.cix = ball.cx;
	ball.ciy = ball.cy;
	ball.vix = ball.vx;
	ball.viy = ball.vy;

}

///////////////////////////////////////////////////////////////
void initPhysics(double rad, double speed, double angle)
{
	clocktime = 0.f;
	double vx = speed * cos(angle);
	double vy = speed * sin(angle);
	simBall1.set(rad, rad, rad, 2, vx, vy, 128, 128, 0);
	realBall.set(rad, rad, rad, 2, vx, vy, 20, 160, 60);
	printf("initPhysics: ball(%f, %f)\n", simBall1.cx, simBall1.cy);
	printf("initPhysics: realball(%f, %f)\n", realBall.cx, realBall.cy);
}

///////////////////////////////////////////////////////////////////
int Game(void)
{
	DWORD passedTime = 0;
	FsPassedTime(true);

	//////////// initial setting up the scene ////////////////////////////////////////
	int timeSpan = 33; // milliseconds
	double timeInc = (double)timeSpan * 0.001; // time increment in seconds

	FsGetWindowSize(width, height);

	int lb,mb,rb,mx,my;
	glViewport(0, 0, width, height);

	////////////////////// main simulation loop //////////////////////////
	while (1)
	{
		FsPollDevice();
		FsGetMouseState(lb,mb,rb,mx,my);
		int key=FsInkey();
		if(key == FSKEY_ESC)
			break;
		timeInc = (double)(passedTime) * 0.001;
		clocktime += timeInc;
		/////////// update physics /////////////////
		updateNumPhysics(simBall1, timeInc);
		updatePrecisePhysics(realBall, timeInc);
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
	return 0;
}

/////////////////////////////////////////////////////////////////
void GameOver(int score)
{
	int r=0;

	FsPollDevice();
	while(FsInkey()!=0)
	{
		FsPollDevice();
	}

	while(FsInkey()==0)
	{
		FsPollDevice();

		int wid,hei;
		FsGetWindowSize(wid,hei);

		glViewport(0,0,wid,hei);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0,(float)wid-1,(float)hei-1,0,-1,1);

		glClearColor(0.0,0.0,0.0,0.0);
		glClear(GL_COLOR_BUFFER_BIT);

		const char *msg1="Game Over";
		char msg2[256];
		glColor3ub(255,255,255);
		glRasterPos2i(32,32);
		glCallLists(strlen(msg1),GL_UNSIGNED_BYTE,msg1);

		sprintf(msg2,"Your score is %d",score);

		glRasterPos2i(32,48);
		glCallLists(strlen(msg2),GL_UNSIGNED_BYTE,msg2);

		FsSwapBuffers();
		FsSleep(10);
	}
}

//////////////////////////////////////////////////////////////////////////////////////
int main(void)
{
	int menu;
	FsOpenWindow(32, 32, winWidth, winHeight, 1); // 800x600 pixels, useDoubleBuffer=1

	int listBase;
	listBase=glGenLists(256);
	YsGlUseFontBitmap8x12(listBase);
	glListBase(listBase);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDepthFunc(GL_ALWAYS);

	while(1)
	{
		menu=Menu();
		if(menu==1)
		{
			int score;
			score=Game();
			GameOver(score);
		}
		else if(menu==eStop)
		{
			break;
		}
	}

	return 0;
}


