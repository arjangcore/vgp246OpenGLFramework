#include <stdio.h>
#include <string.h>
#include <math.h>
#include <random>
#include <time.h>

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
	eIdle = -2,
	eStop = -1,
	eStart = 0,
	eSpeedUp,
	eSpeedDown,
} changeType;

double PI = 3.1415926;
double iSpeed = 70.0;
double radius = 20.;
int num_segments = 30;
double restitution = 0.8;  // coefficient of restitution.
int BallCount = 5;

struct BallS
{
	double x,y, vx, vy;
	double radius;
	unsigned char colorx,colory, colorz;
	void set(double xs, double ys, double v, double u, double rad)
	{ x=xs; y=ys; vx = v; vy=u; radius=rad;}
	double speedSq() { return vx*vx + vy*vy; }
	double distSquare(BallS &ball) {
		return (ball.x-x)*(ball.x-x) + (ball.y-y)*(ball.y-y);
	}
};
BallS *sBalls = NULL;

bool **collisionFlags = NULL;

//////////////////////////////////////////////////////////////////////////////////////
void DrawCircle(double cx, double cy, int i) 
{ 
	double theta = 2. * PI / double(num_segments); 
	double c = cos(theta);//precalculate the sine and cosine
	double s = sin(theta);

	double x = sBalls[i].radius;//we start at angle = 0 
	double y = 0.; 
 	double t = x;
   
	glColor3ub(sBalls[i].colorx,sBalls[i].colory,sBalls[i].colorz);
	glBegin(GL_LINE_LOOP); 
	for(int i = 0; i < num_segments; i++) 
	{ 
		glVertex2d(x + cx, y + cy);
		//apply the rotation matrix
		t = x;
		x = c * x - s * y;
		y = s * t + c * y;
	} 
	glEnd(); 
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
void DrawSolidCircle(double cx, double cy, int i) 
{ 
	double theta = 2. * PI / double(num_segments); 
	double c = cos(theta);//precalculate the sine and cosine
	double s = sin(theta);

	glColor3ub(sBalls[i].colorx,sBalls[i].colory,sBalls[i].colorz);
	double x = sBalls[i].radius;  
	double y = 0; 
	double t = x;
	glBegin(GL_TRIANGLE_FAN); 
	glVertex2d(cx, cy);
	for(int i = 0; i < num_segments; i++) 
	{ 
		glVertex2d(x + cx, y + cy);//output vertex 
        
		//apply the rotation matrix
		t = x;
		x = c * x - s * y;
		y = s * t + c * y;
	} 

	glVertex2d(sBalls[i].radius + cx, cy);
	glEnd(); 
}
//////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////
int Menu(void)
{
	int r=eIdle,key;
	int oldBallCount = BallCount;

	while(r!=eStop && r!=eStart)
	{
		FsPollDevice();
		key=FsInkey();
		switch(key)
		{
		case FSKEY_G:
			r=eStart;
			break;
		case FSKEY_ESC:
			r=eStop;
			break;
		case FSKEY_UP:
			iSpeed++;
			break;
		case FSKEY_DOWN:
			iSpeed = max(5., iSpeed-1);
			break;
		case FSKEY_PAGEUP:
			BallCount++;
			break;
		case FSKEY_PAGEDOWN:
			BallCount = max(2, BallCount-1);
			break;
		case FSKEY_LEFT:
			restitution = max(0.0, restitution-0.1);
			break;
		case FSKEY_RIGHT:
			restitution = min(1.0, restitution+0.1);
			break;
		}

		int wid,hei;
		FsGetWindowSize(wid,hei);

		glViewport(0,0,wid,hei);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-0.5,(GLdouble)wid-0.5,(GLdouble)hei-0.5,-0.5,-1,1);

		glClearColor(0.0,0.0,0.0,0.0);
		glClear(GL_COLOR_BUFFER_BIT);
		char sSpeed[128];
		sprintf(sSpeed, "Average ball speed is %f m/s. Use Up/Down keys to change it!\n", iSpeed);
		char sBallCnt[128];
		sprintf(sBallCnt, "Ball count is %d. Use PageUp/PageDown keys to change it!\n", BallCount);
		char sRadius[128];
		sprintf(sRadius, "ball-ball restitution factor is %f. Use Left/Right keys to change it by 0.1!\n", restitution);

		glColor3ub(255,255,255);

		glRasterPos2i(32,32);
		glCallLists(strlen(sSpeed),GL_UNSIGNED_BYTE,sSpeed);
		glRasterPos2i(32,64);
		glCallLists(strlen(sRadius),GL_UNSIGNED_BYTE,sRadius);
		glRasterPos2i(32,96);
		glCallLists(strlen(sBallCnt),GL_UNSIGNED_BYTE,sBallCnt);

		const char *msg1="G.....Start Game\n";
		const char *msg2="ESC...Exit";
		glRasterPos2i(32,160);
		glCallLists(strlen(msg1),GL_UNSIGNED_BYTE,msg1);
		glRasterPos2i(32,192);
		glCallLists(strlen(msg2),GL_UNSIGNED_BYTE,msg2);

		FsSwapBuffers();
		FsSleep(10);
	}
	
	if(r==eStart){
		if(sBalls){
			delete[] sBalls;
			for(int i = 0; i<oldBallCount; i++)
				delete[] collisionFlags[i];
		delete [] collisionFlags;
		}
		sBalls = new BallS[BallCount];
		collisionFlags = new bool*[BallCount];
		for(int i=0; i<BallCount; i++)
			collisionFlags[i] = new bool[BallCount];
	}
	return r;
}

////////////////////////////
//detects if ball j collides with  ball i
bool ballsCollide(int i, int j)
{
	if(i!=j)
		if( sBalls[j].distSquare(sBalls[i]) <= (sBalls[j].radius + sBalls[i].radius) * (sBalls[j].radius + sBalls[i].radius) )
			return true;

	return false;
}

void checkCollisions()
{
	for(int i=0; i<BallCount; i++)
	{
		collisionFlags[i][i] = false;
		for(int j=i+1; j<BallCount; j++)
			collisionFlags[i][j] = collisionFlags[j][i] = ballsCollide(i,j);
	}
}

bool ballCollides(int idx)
{
	bool res = false;
	for(int i=0; i<BallCount; i++)
	{
		res |= collisionFlags[idx][i];
	}
	return res;
}

/////////////////////////////////////////////////////////////////////
void updatePhysics(double timeInc, int width, int height)
{
	////////////First update balls positions //////////////////
	for (int j = 0; j < BallCount; j++) {

		//update ball position
		sBalls[j].x += sBalls[j].vx * timeInc;
		sBalls[j].y += sBalls[j].vy * timeInc;

		/////////////////////check edge collision ////////////////////////////////////////

		if (sBalls[j].x < sBalls[j].radius && sBalls[j].vx<0) //checking left wall
		{
			//	sBalls[j].x=-sBalls[j].x;
			sBalls[j].vx = -sBalls[j].vx;
		}
		if (sBalls[j].y < sBalls[j].radius && sBalls[j].vy<0) // checking top wall
		{
			//sBalls[j].y=-sBalls[j].y;
			sBalls[j].vy = -sBalls[j].vy;
		}
		if (sBalls[j].x>(width - sBalls[j].radius) && sBalls[j].vx > 0) // checking right wall
		{
			//	sBalls[j].x=width-(sBalls[j].x-width);
			sBalls[j].vx = -sBalls[j].vx;
		}
		if (sBalls[j].y>(height - sBalls[j].radius) && sBalls[j].vy > 0) // check bottom wall
		{
			//	sBalls[j].y = height -(sBalls[j].y-height);
			sBalls[j].vy = -sBalls[j].vy;
		}
	}

	///// next check collisions ///////////////////////
	checkCollisions();


	////// here you do collision resolution. /////////////

}

//////////////////////////////////////////////////////
void renderScene()
{
	////// render balls ///////////////////
	for (int j = 0; j < BallCount; j++)
	{
		if (!ballCollides(j))
			DrawSolidCircle(sBalls[j].x, sBalls[j].y, j);
		else
			DrawCircle(sBalls[j].x, sBalls[j].y, j);
	}
	////  swap //////////
	FsSwapBuffers();
}

//////////////////////////////////////////////////////////////////////////////
int Game(void)
{
	DWORD passedTime = 0;
	FsPassedTime(true);

	double ballX, ballY, ballVx, ballVy;
	int width=0,height=0;

	//////////// setting up the scene ////////////////////////////////////////
	const int timeSpan = 33; // milliseconds
	double timeInc = (double)timeSpan * 0.001; // time increment in seconds
	
	FsGetWindowSize(width, height);

	srand(time(NULL)); /* seed random number generator */
	int xdist = width/3;
	int ydist = height/3;
	for(int i=0; i<BallCount; i++)
	{
		double rad = radius * (1. + double(rand()%BallCount)/double(BallCount));
		ballX = width/2 + (i-BallCount/2) * rand()% xdist;
		ballY = height/2 + (i-BallCount/2) *rand()% ydist;
		double angle = double(rand() % 360)/180. * PI;
		double speed = iSpeed *(1. + double(rand()%BallCount)/double(BallCount));
		ballVx = speed * cos(angle);
		ballVy = speed * sin(angle);
		sBalls[i].set(ballX, ballY, ballVx, ballVy, rad);
		sBalls[i].colorx=rand()%250; sBalls[i].colory=rand()%250; sBalls[i].colorz=rand()%250;
	}
	
	glViewport(0,0,width,height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-0.5,(GLdouble)width-0.5,(GLdouble)height-0.5,-0.5,-1,1);

	glClearColor(1.0,1.0,1.0,1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	////////////////////// main simulation loop //////////////////////////
	while(1)
	{
		glClear(GL_COLOR_BUFFER_BIT);
		int lb,mb,rb,mx,my;

		FsPollDevice();
		FsGetMouseState(lb,mb,rb,mx,my);
		int key=FsInkey();
		if(key == FSKEY_ESC)
			break;
		timeInc = (double)(passedTime) * 0.001;

		/////////// update physics /////////////////
		updatePhysics(timeInc, width, height);
		
		renderScene();
		
		////// update time lapse /////////////////
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
	}
	return 0;
}

////////////////////////////////////////////////////////////////
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
/////////////////////////////////////////////////////////////////////////////////////////
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
		if(menu==eStart)
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


