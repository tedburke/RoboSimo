//
// RoboSimo - The RoboSumo simulator - Ted Burke - 26-9-2009
//
// Programs written in C can access this simulator over
// the network and control a virtual robot within the
// simulated environment.
//

#define _STDCALL_SUPPORTED
#define _M_IX86
#include "glut.h"

#include <stdio.h>

void update(void);
void display(void);
void keyboard(unsigned char key, int x, int y);

// Window handle
static int window;

// Global flags
int program_exiting = 0; // to be set to 1 when program is exiting
int fullscreen = 0; // to be set to 1 for fullscreen mode

// For counting calls to display function during debugging
static int counter = 0;

int main(int argc, char **argv)
{
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
	
	// Set the background colour to green
	glClearColor(0, 1, 0, 0);
	
	// Enter the main event loop
	glutMainLoop();

	// The program will actually never reach this point
	// because glutMainLoop() never returns before the
	// program exits.
	return 0;
}

void update()
{
	// Check if program should exit
	if (program_exiting)
	{
		// Clean up, then exit
		glutDestroyWindow(window);
		exit(0);
	}
	
	// Update scene
	
	// Request redrawing of scene
	glutPostRedisplay();
}

void reshape(int width, int height)
{
	// Set viewport to new window size
	glViewport(0,0,width,height);
	
}

void display()
{
	// Print a message to the console
	printf("In display function, counter = %d\n", counter++);
	
	// Clear the background
	glClear(GL_COLOR_BUFFER_BIT);

	// draw something

	glutWireTeapot(0.5);
	// glutSolidTeapot(0.5);
	// glutWireSphere(0.5,100,100);
	// glutSolidSphere(0.5,100,100);
	// glutWireTorus(0.3,0.5,100,100);
	// glutSolidTorus(0.3,0.5,100,100);
	// glutWireIcosahedron();
	// glutSolidIcosahedron();
	// glutWireDodecahedron();
	// glutSolidDodecahedron();
	// glutWireCone(0.5,0.5,100,100);
	// glutSolidCone(0.5,0.5,100,100);
	// glutWireCube(0.5);
	// glutSolidCube(0.5);
	
	// Trigger the next draw straight away, so that scene
	// will keep updating.
	
	// Swap back buffer to screen
	glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y)
{
	printf("Pressed key %c on coordinates %d,%d\n", key, x, y);
	
	if (key == 'q') program_exiting = 1; // Set exiting flag
}
