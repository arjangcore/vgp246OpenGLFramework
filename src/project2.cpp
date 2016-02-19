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
#include "wcode/fswin32keymap.h"
#include "bitmapfont\ysglfontdata.h"

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
double PI = 3.1415926;
double iAngle = PI / 4.;
double iSpeed = 150.0f;
double radius = 35.;
int circleSections = 10;
int wid=0,hei=0;

// draw the slop /////////
double slopeStartX = radius;
double slopeStartY = radius*2.;
double slopeEndX = slopeStartX;
double slopeEndY = slopeStartY;

// draws a hollow circle centered at (cx, cy) and with radius r, using num_segments triangle segments.
// It rotates the circle by iAngle as well.
void DrawCircle(double angle, double cx, double cy, double r, int num_segments, int red, int green, int blue) 
{ 
	double theta = 2. * PI / double(num_segments); 
	double c = cos(theta);//precalculate the sine and cosine
	double s = sin(theta);

	double x = r;//we start at angle = 0 
	double y = 0.; 
 	double t = x;
	x = cos(angle) * x - sin(angle) * y;
	y = sin(angle) * t + cos(angle) * y;
   
	glBegin(GL_LINES); 
	for(int i = 0; i < num_segments; i++) 
	{ 
		if(i%2==0)
			glColor3ub(red,green,blue);
		else
			glColor3ub(green,red,blue);

		glVertex2d(x + cx, y + cy);
        
		//apply the rotation matrix
		t = x;
		x = c * x - s * y;
		y = s * t + c * y;
		glVertex2d(x + cx, y + cy);
	} 
	glEnd(); 
}

// draws a solid circle centered at (cx, cy) and with radius r, using num_segments triangle segments.
// It rotates the circle by iAngle as well.
void DrawSolidCircle(double angle, double cx, double cy, double r, int num_segments, int red, int green, int blue) 
{ 
	double theta = 2. * PI / double(num_segments); 
	double c = cos(theta);//precalculate the sine and cosine
	double s = sin(theta);

	double x = r;//we start at angle = 0 
	double y = 0; 
 	double t = x;
	double sx = cos(angle) * x - sin(angle) * y;
	double sy = sin(angle) * t + cos(angle) * y;
    x = sx; y = sy;
	glBegin(GL_TRIANGLE_FAN); 
	glColor3ub(red, green, blue);
	glVertex2d(cx, cy);
	for(int i = 0; i < num_segments; i++) 
	{ 
		if(i%2)
			glColor3ub(red, green, blue);
		else
			glColor3ub(blue,green, red);
		glVertex2d(x + cx, y + cy);//output vertex 
        
		//apply the rotation matrix
		t = x;
		x = c * x - s * y;
		y = s * t + c * y;
	} 

	if(num_segments%2)
		glColor3ub(red, green, blue);
	else
		glColor3ub(blue,green, red);
	glVertex2d(sx + cx, sy + cy);
	glEnd(); 
}

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
			friction+=0.1;
			break;
		case FSKEY_DOWN:
			friction = max(1., friction-0.1);
			break;
		case FSKEY_LEFT:
			iAngle = max(0., iAngle-angleInc);
			break;
		case FSKEY_RIGHT:
			iAngle = min(90.0, iAngle+angleInc);
			break;
		}

		if(r== eStop)
			return r;
		FsGetWindowSize(wid,hei);

		glViewport(0,0,wid,hei);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-0.5,(GLdouble)wid-0.5,(GLdouble)hei-0.5,-0.5,-1,1);

		glClearColor(0.0,0.0,0.0,0.0);
		glClear(GL_COLOR_BUFFER_BIT);
		glColor3ub(127,127,127);

		char sSpeed[128];
		sprintf(sSpeed, "Initial ball-slope friction is %f. Use Up/Down keys to change it by 1/10!\n", friction);
		char sAngle[128];
		sprintf(sAngle, "Initial slope Angle is %f degrees. Use Left/Right keys to change it!\n", iAngle*180./PI);
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

		glClear(GL_COLOR_BUFFER_BIT);
		
		//////////////////// draw the slope ///////////////
		glLineWidth(4);
		double count = (4*circleSections);
		double xstep = (slopeEndX - slopeStartX)/count;
		double ystep = (slopeEndY - slopeStartY)/count;
		glBegin(GL_LINES);
		for(int i = 0; i<(count-1); i++) {
			if(i%2==0)
				glColor3ub(0,144,144);
			else
				glColor3ub(144,0,144);
			
			glVertex2i(slopeStartX+ (double)i*xstep, slopeStartY+(double)i*ystep);
			glVertex2i(slopeStartX+ (double)(i+1)*xstep, slopeStartY+(double)(i+1)*ystep);
		}
		glEnd();
		////////////////////////////////////////////////////////
		
		/////////////////////////draw the hallow 2d disc /////////////
		glLineWidth(2);
		DrawCircle(angle, ballX, ballY, radius, circleSections, 0, 144, 144);
		angle+= angleInc;

		glColor3ub(127,127,127);
		char str[256];
		sprintf(str,"Velocity:(%f, %f)",ballVx, ballVy);
		glRasterPos2i(32,40);
		glCallLists(strlen(str),GL_UNSIGNED_BYTE,str);
		sprintf(str,"Pos:(%f, %f))",ballX, ballY);
		glRasterPos2i(32,25);
		glCallLists(strlen(str),GL_UNSIGNED_BYTE,str);

		FsSwapBuffers();
}
/////////////////////////////////////////////////////////////////////
void updatePhysics(Circle2D &ball, double timeInc)
{
	//////////// your physics goes here //////////////////////////
	// we use a coordinate system in which x goes from left to right of the screen and y goes from top to bottom of the screen
	// we have 1 forces here: 1) gravity which is in positive y direction. 
		ballVx = ballVx_last - airResistance*ballVx_last*timeInc;
		ballX = ballX_last + ballVx_last*timeInc; // x position update, x speed is constant.
		
		ballVy = ballVy_last + (gravity - airResistance*ballVy_last)*timeInc; // y speed update
		ballY = ballY_last + ballVy_last*timeInc; // y position update
}

///////////////////////////////////////////////////////////////
void initPhysics(double rad, double speed, double angle)
{
	ballX = ballX_last=32;
	ballY=ballY_last=slopeStartY-radius;
	ballVx = ballVx_last =  iSpeed * cos(iAngle);
	ballVy = ballVy_last = -iSpeed * sin(iAngle);
}

///////////////////////////////////////////////////////////////////
int Game(void)
{
	DWORD passedTime = 0;
	FsPassedTime(true);

	double ballX,ballY,ballVx,ballVy;
	double ballX_last,ballY_last,ballVx_last,ballVy_last;

	int timeSpan = 33; // milliseconds
	double timeInc = (double)timeSpan * 0.001; // time increment in seconds
	double timeInc2 = timeInc * timeInc;
	double gravity = 98.1;

	FsGetWindowSize(wid, hei);

	/////////////////////////////////////////////////////

	ballX = ballX_last=32;
	ballY=ballY_last=slopeStartY-radius;
	ballVx = ballVx_last =  iSpeed * cos(iAngle);
	ballVy = ballVy_last = -iSpeed * sin(iAngle);
	int lb,mb,rb,mx,my;

	glViewport(0,0,wid,hei);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-0.5,(GLdouble)wid-0.5,(GLdouble)hei-0.5,-0.5,-1,1);
		
	FsGetWindowSize(wid,hei);

	slopeEndX = (double)(hei-radius) * tanf(iAngle) + slopeStartX;
	slopeEndY = (double)(hei-radius) + slopeStartY;

	float angleInc = PI/40.;
	float angle = 0.;
	while(1)
	{
		FsPollDevice();
		FsGetMouseState(lb,mb,rb,mx,my);
		int key=FsInkey();
		if(key == FSKEY_ESC)
			break;
		timeInc = (double)(passedTime) * 0.001;
		//////////// your physics goes here //////////////////////////
		/////////////////////////////////////////////////////////////
		ballVx = ballVx_last - airResistance*ballVx_last*timeInc;
		ballX = ballX_last + ballVx_last*timeInc; // x position update, x speed is constant.
		
		ballVy = ballVy_last + (gravity - airResistance*ballVy_last)*timeInc; // y speed update
		ballY = ballY_last + ballVy_last*timeInc; // y position update
		////////////////////////////////////////////////////////////////


		glClear(GL_COLOR_BUFFER_BIT);
		
		//////////////////// draw the slope ///////////////
		glLineWidth(4);
		double count = (4*circleSections);
		double xstep = (slopeEndX - slopeStartX)/count;
		double ystep = (slopeEndY - slopeStartY)/count;
		glBegin(GL_LINES);
		for(int i = 0; i<(count-1); i++) {
			if(i%2==0)
				glColor3ub(0,144,144);
			else
				glColor3ub(144,0,144);
			
			glVertex2i(slopeStartX+ (double)i*xstep, slopeStartY+(double)i*ystep);
			glVertex2i(slopeStartX+ (double)(i+1)*xstep, slopeStartY+(double)(i+1)*ystep);
		}
		glEnd();
		////////////////////////////////////////////////////////
		
		/////////////////////////draw the hallow 2d disc /////////////
		glLineWidth(2);
		DrawCircle(angle, ballX, ballY, radius, circleSections, 0, 144, 144);
		angle+= angleInc;

		glColor3ub(127,127,127);
		char str[256];
		sprintf(str,"Velocity:(%f, %f)",ballVx, ballVy);
		glRasterPos2i(32,40);
		glCallLists(strlen(str),GL_UNSIGNED_BYTE,str);
		sprintf(str,"Pos:(%f, %f))",ballX, ballY);
		glRasterPos2i(32,25);
		glCallLists(strlen(str),GL_UNSIGNED_BYTE,str);

		FsSwapBuffers();

		//////////////////////////// set last frame velocity and position//////////////
		ballVx_last = ballVx;
		ballVy_last = ballVy;
		ballX_last = ballX;
		ballY_last = ballY;

		passedTime=FsPassedTime(); 
		int timediff = timeSpan-passedTime;
	//	printf("\ntimeInc=%f, passedTime=%d, timediff=%d", timeInc, passedTime, timediff);
		while(timediff >= timeSpan/3)
		{
			FsSleep(5);
			passedTime=FsPassedTime(); 
			timediff = timeSpan-passedTime;
		//	printf("--passedTime=%d, timediff=%d", passedTime, timediff);
		}
		passedTime=FsPassedTime(true); 
		//printf("**passedTime=%d, timediff=%d\n", passedTime, timeSpan-passedTime);
	}
	return 0;
}

/////////////////////////////////////////////////////////////////
void GameOver(int score)
{
	int r;
	r=0;

	FsPollDevice();
	while(FsInkey()!=0)
	{
		FsPollDevice();
	}

	while(FsInkey()==0)
	{
		FsPollDevice();

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
	FsOpenWindow(32,32,800,600,1); // 800x600 pixels, useDoubleBuffer=1

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


