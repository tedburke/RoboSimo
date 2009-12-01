//
// RoboSimo - The RoboSumo simulator - Ted Burke - 26-9-2009
//
// Programs written in C can access this simulator over
// the network and control a virtual robot within the
// simulated environment.
//

// Header files
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <GL/glu.h>
#include "glut.h"

// My header files
#include "shared.h"

// GLUT callback functions
void update(void);
void display(void);
void reshape(int, int);
void keyboard(unsigned char key, int x, int y);

// Define the value of pi
const double pi = 3.14159;

// Remember when the scene was last updated (in clock ticks)
clock_t last_time;

// GLUT window identifier
static int window;

// Global flags
int fullscreen = 0; // to be set to 1 for fullscreen mode

// Global identifier for arean floor texture
static GLuint texName;
#define texImageWidth 64
#define texImageHeight 64
static GLubyte texImage[texImageHeight][texImageWidth][4];

// definition for network thread function
int p1 = 1;
int p2 = 2;
extern DWORD WINAPI network_thread(LPVOID);
HANDLE hNetworkThread1, hNetworkThread2;

// This string will be set to the network address
// info to be displayed on the screen.
extern char network_address_display_string[];
double x_networkAddressText, y_networkAddressText;

// Text rendering function prototype
void renderBitmapString(float, float, float, void *, char *);

int main(int argc, char **argv)
{
	int n;
	
	// Start GLUT
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);

	glutInitWindowSize(640,400);
	glutInitWindowPosition(100,100);
	window = glutCreateWindow("RoboSimo");

	// Go fullscreen if flag is set
	if (fullscreen) glutFullScreen();
	
	// Register callback functions
	glutIdleFunc(update);
	glutKeyboardFunc(keyboard);
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	
	// Set the background colour to green
	glClearColor(0, 1, 0, 0);
	
	// Load bitmap texture for arena floor
	// LOAD IMAGE FROM FILE
	int x, y;
	for (y = 0 ; y < texImageHeight ; ++y)
	{
		for (x = 0 ; x < texImageWidth ; ++x)
		{
			
		}
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &texName);
	glBindTexture(GL_TEXTURE_2D, texName);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texImageWidth, texImageHeight,
					0, GL_RGBA, GL_UNSIGNED_BYTE, texImage);
	
	// Initialise robots' states
	for (n=0 ; n<2 ; ++n)
	{
		robot[n].active = 0; // inactive until a competitor connects over the network
		robot[n].name[20];
		robot[n].w = 0.16; // 16cm wide
		robot[n].l = 0.20; // 20cm long
		robot[n].h = 0.10; // 10cm high
		robot[n].v1 = 0;
		robot[n].v2 = 0;
	}
	robot[0].x = -0.5; // Put first robot on left of arena facing right
	robot[0].y = 0;
	robot[0].angle = 0;
	robot[1].x = 0.5; // Put second robot on right of arena facing left
	robot[1].y = 0;
	robot[1].angle = pi;

	// Launch network threads (one for each player, ports 4009 and 4010)
	hNetworkThread1 = CreateThread(NULL, 0, network_thread, (LPVOID)(&p1), 0, NULL);
	hNetworkThread2 = CreateThread(NULL, 0, network_thread, (LPVOID)(&p2), 0, NULL);
	
	// Enter the main event loop
	glutMainLoop();

	// The program will actually never reach this point
	// because glutMainLoop() never returns before the
	// program exits.
	return 0;
}

void update()
{
	int n;
	
	// Check if program should exit
	if (program_exiting)
	{
		// Clean up, then exit
		glutDestroyWindow(window);
		WaitForSingleObject(hNetworkThread1, 1000); // Wait up to 1 second for network thread to exit
		WaitForSingleObject(hNetworkThread2, 1000); // Wait up to 1 second for network thread to exit
		CloseHandle(hNetworkThread1);
		CloseHandle(hNetworkThread2);
		exit(0);
	}
	
	// Calculate time elapsed since last update
	double tau;
	clock_t new_time;
	new_time = clock();
	if (new_time > last_time) tau = (new_time - last_time) / (double)CLOCKS_PER_SEC;
	else tau = 0; // Just in case clock value reaches max value and wraps around
	last_time = new_time;
	
	// Update robot positions
	double x1, y1, x2, y2;
	for (n=0 ; n<2 ; ++n)
	{
		// Update wheel velocities
		robot[n].v1 = ((robot[n].LATD & 0x02)-(robot[n].LATD & 0x01)) * 0.25;//(robot[n].CCPR1L / 1000.0);
		robot[n].v2 = ((robot[n].LATD & 0x04)-(robot[n].LATD & 0x03)) * 0.25;//(robot[n].CCPR2L / 1000.0);
		
		// Left wheel position: x1, y1. Left wheel velocity: v1.
		// Right wheel position: x2, y2. Left wheel velocity: v2.
		x1 = robot[n].x - (robot[n].w/2.0)*sin(robot[n].angle) + tau*robot[n].v1*cos(robot[n].angle);
		y1 = robot[n].y + (robot[n].w/2.0)*cos(robot[n].angle) + tau*robot[n].v1*sin(robot[n].angle);
		
		x2 = robot[n].x + (robot[n].w/2.0)*sin(robot[n].angle) + tau*robot[n].v2*cos(robot[n].angle);
		y2 = robot[n].y - (robot[n].w/2.0)*cos(robot[n].angle) + tau*robot[n].v2*sin(robot[n].angle);
		
		robot[n].x = (x1 + x2)/2.0;
		robot[n].y = (y1 + y2)/2.0;
		
		if (x2 == x1)
		{
			// robot pointing (close to) straight up or down
			if (y2 < y1) robot[n].angle = 0;
			else robot[n].angle = pi;
		}
		else
		{
			// Calculate updated orientation from new wheel positions
			robot[n].angle = (pi/2.0) + atan2(y2-y1, x2-x1);
		}
	}
	
	// Request redrawing of scene
	glutPostRedisplay();
}

void reshape(int window_width, int window_height)
{
	int text_bar_height = 25;
	double scene_width, scene_height;
	double left, right, bottom, top;
	
	// Set viewport to new window size
	glViewport(0,0,window_width,window_height);
	
	// Set up OpenGL projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (window_width > window_height - text_bar_height)
	{
		top = 0.7;
		bottom = -0.7 * (window_height + 2.0*text_bar_height) / (double)window_height;
		right = 0.7 * (window_width/(double)(window_height-text_bar_height));
		left = -right;
	}
	else
	{
		right = 0.7;
		left = -right;
		top = 0.7*((window_height-text_bar_height)/(double)window_width);
		bottom = -top * (window_height + 2.0*text_bar_height) / (double)window_height;
	}
	
	gluOrtho2D(left,right,bottom,top);
	
	// Update position of network address text
	x_networkAddressText = left; // NB string starts with a couple of spaces, so ok to place at left edge
	y_networkAddressText = -0.7;
}

void display()
{
	// Clear the background
	glClear(GL_COLOR_BUFFER_BIT);

	// Draw arena
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	GLUquadricObj *pQuad;
	pQuad = gluNewQuadric();
	glColor3d(1,1,1);
	gluDisk(pQuad, 0, 0.6, 36, 2);
	glColor3d(0,0,0);
	gluDisk(pQuad, 0, 0.5, 36, 2);
	gluDeleteQuadric(pQuad);
	
	// Render bitmap on arena floor
	glEnable(GL_TEXTURE_2D);
	glBindTexture (GL_TEXTURE_2D, bitmap_number);
	glBegin (GL_QUADS);
	glTexCoord2f (0.0, 0.0);
	glVertex3f (-1.0, -1.0, 0.0);
	glTexCoord2f (1.0, 0.0);
	glVertex3f (1.0, -1.0, 0.0);
	glTexCoord2f (1.0, 1.0);
	glVertex3f (1.0, 1.0, 0.0);
	glTexCoord2f (0.0, 1.0);
	glVertex3f (-1.0, 1.0, 0.0);
	glEnd ();
	glDisable(GL_TEXTURE_2D);

	// Draw robots
	int n;
	for (n=0 ; n<2 ; ++n)
	{
		if (n==0) glColor3d(1,0,0);
		else glColor3d(0,0,1);
		glLoadIdentity();
		glTranslated(robot[n].x, robot[n].y, robot[n].h/2.0);
		glRotated(robot[n].angle * (180.0/pi), 0, 0, 1);
		glScaled(robot[n].l,robot[n].w,robot[n].h);
		glutSolidCube(1.0);
	}
	
	// Draw text information
	glColor3f(0, 0, 0);
	glLoadIdentity();
	renderBitmapString(x_networkAddressText, y_networkAddressText, 0.0,
	GLUT_BITMAP_HELVETICA_18, network_address_display_string);

	// Swap back buffer to screen
	glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y)
{
	//printf("Pressed key %c on coordinates %d,%d\n", key, x, y);
	
	if (key == 'q') program_exiting = 1; // Set exiting flag
}

// This function renders a string (some text) on the screen at a specified position
void renderBitmapString(float x, float y, float z, void *font, char *text)
{
	// Variable to count through the string's character
	int n = 0;
	
	// Print the characters of the string one by one
	glRasterPos3f(x, y, z);
	while(text[n] != '\0')
	{
		// Render a character
		glutBitmapCharacter(font, text[n]);
		n = n + 1;
	}
}
