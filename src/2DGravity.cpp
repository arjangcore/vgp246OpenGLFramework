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
#include "vector2d.h"

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

int gBallCount = 40;
const double gGravityConstant = 6.674E-11;
double gAverageMass = 1.5E11;

double PI = 3.1415926;
double iAngle = PI / 3.; // projectile inclination angle in radian
double iSpeed = 5.0;  // initial ball speed
double radius = 0.6;
int circleSections = 30;


const int winWidth = 800;
const int winHeight = 600;
const double ratio = (float)winHeight / (float)winWidth;
const double WorldWidth = 100.0; // 50 meter wide
const double WorldHeight = WorldWidth * ratio; // 
int width = 0, height = 0;


static double clocktime = 0.f;
int framerate = 30;

struct Circle2D
{
	Vector2d<double> pos, vel, acc;
	double radius;
	int red, green, blue;
	
	double mass;
	double massG; // mass * G
	Circle2D(double x, double y, double rad, double m, Vector2d<double> &vel, int r, int g, int b)
	{
		radius = rad;
		pos.x = x;
		pos.y = y;
		vel = vel;
		mass = m;
		massG = m * gGravityConstant;
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
			glVertex2d(x + pos.x, y + pos.y);

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
		glVertex2d(pos.x, pos.y);
		for (int i = 0; i < num_segments; i++)
		{
			glVertex2d(x + pos.x, y + pos.y);//output vertex 

			//apply the rotation matrix
			t = x;
			x = c * x - s * y;
			y = s * t + c * y;
		}

		glVertex2d(sx + pos.x, sy + pos.y);
		glEnd();
	}
};

vector< Circle2D> simBalls;
bool isInside(double x, double y)
{
	Vector2d<double> test(x, y);
	for (auto &ball : simBalls)
	{
		if ((test - ball.pos).LengthSq() <= (2.0*ball.radius)*(2.0*ball.radius))
			return true;
	}
	return false;
}
//////////////////////////////////////////////////////////////
double gausianRandomMass(double average, double dev)
{
	std::random_device rd;
	std::mt19937 e2(rd());
	std::normal_distribution<double> dist(average, dev);
	double rawRand = round(dist(e2));
	return min(average + dev, rawRand);
}
///////////////////////////////////////////////////////////////
void initPhysics(double rad, double speed, double angle)
{
	simBalls.clear();
	simBalls.reserve(gBallCount);
	srand(time(0));
	for (int i = 0; i < gBallCount; i++)
	{
		double x = rand() % (int)WorldWidth;
		double y = rand() % (int)WorldHeight;
		if (isInside(x, y))
		{
			i--;
			continue;
		}
		double vx = rand() % (int)speed - speed;
		double vy = rand() % (int)speed - speed;
		Vector2d<double> vel(vx, vy);
		double pSpeed = vel.Length();
		double speedDist = gausianRandomMass(speed, speed / 20.);
		vel = vel *(speedDist / pSpeed);
		double mass = gausianRandomMass(gAverageMass, gAverageMass/15.);
		simBalls.push_back(std::move(Circle2D(x, y, rad, mass, vel, 128 + i, 2 * i, 20 * i)));
	}
	clocktime = 0.f;
	//	printf("initPhysics: ball(%f, %f)\n", simBall1.cx, simBall1.cy);
	//	printf("initPhysics: realball(%f, %f)\n", realBall.cx, realBall.cy);
}

//////////////////////////////////////////////////////////////////////////////////////////////
int Menu(void)
{
	int r=0,key;
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
			radius = max(0.5, radius - 0.2);
			break;
		case FSKEY_RIGHT:
			radius = min(5.0, radius + 0.2);
			break;
		}

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
		sprintf_s(sSpeed, "Current planet speed is %f m/s. Use Up/Down keys to change it!\n", iSpeed);
		char sAngle[128];
		sprintf_s(sAngle, "Current planet radius is %f meter. Use Left/Right keys to change it!\n", radius);
		glColor3ub(255,255,255);
		glRasterPos2i(32,32);
		glCallLists(strlen(sSpeed),GL_UNSIGNED_BYTE,sSpeed);
		glRasterPos2i(32,64);
		glCallLists(strlen(sAngle),GL_UNSIGNED_BYTE,sAngle);
		const char *msg1="S.....Start Simulation";
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
	for (auto &w : simBalls)
	{
		w.DrawSolidCircle(0, circleSections);
	}

	///////////// draw the overlay HUD /////////////////////
	glColor3ub(127, 127, 127);
	char str[256];
//	sprintf(str, "# of Balls=%d, frame rate=%d", gBallCount, framerate);
//	glRasterPos2i(64, 64);
//	glCallLists(strlen(str), GL_UNSIGNED_BYTE, str);
	printf("# of Balls=%d, frame rate=%d\n", gBallCount, framerate);

	FsSwapBuffers();
}
/////////////////////////////////////////////////////////////////////
void updateNumPhysics(double timeInc)
{
	//////////// your physics goes here //////////////////////////
	// we use a coordinate system in which x goes from left to right of the screen and y goes from top to bottom of the screen
	// we have 1 forces here: 1) gravity which is in positive y direction. 
	//////////////Compute Gravity force:///////////////////////
	for (auto &ball : simBalls)
		ball.acc.set(0.0, 0.0);

	for (int i = 0; i < gBallCount; i++)
	{
		for (int j = i + 1; j < gBallCount; j++)
		{
			Circle2D &balli = simBalls[i];
			Circle2D &ballj = simBalls[j];
			Vector2d<double> d = ballj.pos - balli.pos; // displacement vector
			Vector2d<double> ud = Normal<double>(d);

			////// Collision detection and Resolution ////////
			if (d.Length() <= (balli.radius + ballj.radius)*(balli.radius + ballj.radius))
			{
				double iud = DotProduct<double>(balli.vel, ud);
				double jud = DotProduct<double>(ballj.vel, ud);
				balli.vel = balli.vel - ud *(2.0 * iud);
				ballj.vel = ballj.vel + ud *(2.0 * jud);
			}

			double inv = 1.0 / d.LengthSq();
			Vector2d<double> force = ud * inv;
			balli.acc = balli.acc + (force * ballj.massG);
			ballj.acc = balli.acc - (force * balli.massG);
		}
	}

	//////////////Explicit Euler Integration:///////////////////////
	for (auto &ball : simBalls)
	{
		ball.pos = ball.pos + ball.vel * timeInc;
		ball.vel = ball.vel + ball.acc * timeInc;
		if (ball.vel.LengthSq() > (10.*iSpeed)*(10.*iSpeed))
			printf("Rogue planet!!\n");
		/////////////////////check edge collision ////////////////////////////////////////
		if (ball.pos.x<0 && ball.vel.x <0)
		{
			ball.pos.x = -ball.pos.x;
			ball.vel.x = -ball.vel.x;
		}
		if (ball.pos.y<0 && ball.vel.y<0)
		{
			ball.pos.y = -ball.pos.y;
			ball.vel.y = -ball.vel.y;
		}
		if (ball.pos.x > WorldWidth && ball.vel.x > 0.001)
		{
			ball.pos.x = WorldWidth - (ball.pos.x - WorldWidth);
			ball.vel.x = -ball.vel.x;
		}
		if (ball.pos.y > WorldHeight && ball.vel.y > 0.001)
		{
			ball.pos.y = WorldHeight - (ball.pos.y - WorldHeight);
			ball.vel.y = -ball.vel.y;
		}
	}

}

///////////////////////////////////////////////////////////////////
int Game(void)
{
	DWORD passedTime = 0;
	FsPassedTime(true);

	//////////// initial setting up the scene ////////////////////////////////////////
	int timeSpan = 33; // milliseconds
	double timeInc = (double)timeSpan * 0.001; // time increment in seconds
	
	initPhysics(radius, iSpeed, iAngle);

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
		updateNumPhysics(timeInc);
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
		framerate = 1000 / passedTime;
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

		sprintf_s(msg2,"Your score is %d",score);

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


