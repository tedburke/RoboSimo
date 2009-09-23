#define _STDCALL_SUPPORTED
#define _M_IX86
#include "GL/glut.h"

#include <stdio.h>

void display(void);
void keyboard(unsigned char key, int x, int y);

static int window;

int main(int argc, char **argv)
{
	// Start GLUT
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
	glutInitWindowSize(640,400);
	glutInitWindowPosition(100,100);
	
	// glutFullScreen();
	
	window = glutCreateWindow("RoboSimo");
	
	glutKeyboardFunc(keyboard);
	glutDisplayFunc(display);
	
	glClearColor(0, 0, 0, 0);
	
	glutMainLoop();
	
	printf("Exited GLUT main loop - program closing\n");
	return 0;
}

void display()
{
	// do  a clearscreen
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
}

void keyboard(unsigned char key, int x, int y)
{
	printf("Pressed key %c on coordinates %d,%d\n", key, x, y);
	if (key == 'q') glutDestroyWindow(window);
}
