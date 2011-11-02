//
// RoboSimo - The RoboSumo simulator
// written by Ted Burke - last updated 2-11-2011
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
void keyboardSpecial(int key, int x, int y);
void mouse(int, int, int, int);
void mouseDrag(int, int);

// Define the value of pi
const double pi = 3.14159;

// Remember when the scene was last updated (in clock ticks)
clock_t last_time;

// GLUT window identifier
static int window;

// Window dimensions
int window_width = 640;
int window_height = 400;
int window_x = 100;
int window_y = 100;

// Orthographic projection dimensions
double ortho_left, ortho_right, ortho_bottom, ortho_top;
GLint orthographic_projection = 0;

// Camera position for perspective view
GLfloat camera_latitude = 60; // degrees "south" of vertical
GLfloat camera_longitude = 0; // degrees "east" of reference point
GLfloat camera_distance = 2.6; // distance from centre point of table

// Global flags
int fullscreen = 0; // to be set to 1 for fullscreen mode

// Global identifiers for arena floor and wall textures
static GLuint texFloorName;
static GLuint texWallName;
#define texImageWidth 512
#define texImageHeight 512
static GLubyte texFloorImage[texImageHeight][texImageWidth][3];
static GLubyte texWallImage[texImageHeight][texImageWidth][3];

// definition for network thread function
int p1 = 1;
int p2 = 2;
extern DWORD WINAPI network_thread(LPVOID);
HANDLE hNetworkThread1, hNetworkThread2;

// This string will be set to the network address
// info to be displayed on the screen.
extern char network_address_display_string[];
double x_networkAddressText, y_networkAddressText;

// Function prototypes
void renderBitmapString(float, float, float, void *, char *);
void initialise_robots();

int main(int argc, char **argv)
{
	// Start GLUT
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);

	glutInitWindowSize(window_width, window_height);
	glutInitWindowPosition(window_x, window_y);
	window = glutCreateWindow("RoboSimo");

	// Go fullscreen if flag is set
	if (fullscreen) glutFullScreen();
	
	// Register GLUT callback functions
	glutIdleFunc(update);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(keyboardSpecial);
	glutMouseFunc(mouse);
	glutMotionFunc(mouseDrag);
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	
	// Set the background colour to blue
	glClearColor(0.7, 0.7, 1.0, 1.0);

	// Create OpenGL texture for wall
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &texWallName);
	glBindTexture(GL_TEXTURE_2D, texWallName);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	// Create OpenGL texture for floor
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &texFloorName);
	glBindTexture(GL_TEXTURE_2D, texFloorName);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	// Load scene info - floor image and robot positions
	if (initialise_scene(1) != 0)
	{
		// Error occurred loading scene info from files
		fprintf(stderr, "Error loading scene 1 information from files. Exiting.\n");
		exit(1);
	}
		
	// Set up lighting
	// Following suggested setup from http://www.sjbaker.org/steve/omniv/opengl_lighting.html
	//glShadeModel(GL_SMOOTH);
	GLfloat light_ambient[] = {0.2, 0.2, 0.2, 1};
	GLfloat light_diffuse[] = {1.0, 1.0, 1.0, 1.0};
	GLfloat light_specular[] = {1.0, 1.0, 1.0, 1.0};
	GLfloat global_ambient[] = {0.3, 0.3, 0.3, 1.0};
	GLfloat material_specular[] = {0.3, 0.3, 0.3, 1.0};
	GLfloat material_emission[] = {0.1, 0.1, 0.1, 1.0};
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightModelfv(GL_AMBIENT, global_ambient);
	glEnable(GL_LIGHT0);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material_specular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, material_emission);
		
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

int initialise_scene(int scene_number)
{
	// Open robot_positionsX.txt to set starting positions
	char robot_positions_filename[50];
	sprintf(robot_positions_filename, "robot_positions%d.txt", scene_number);
	FILE *robot_positions_file = fopen(robot_positions_filename, "r");
	
	// Open floorX.bmp to get floor image
	char floor_filename[50];
	sprintf(floor_filename, "floor%d.bmp", scene_number);
	FILE *floor_file = fopen(floor_filename, "rb");
	
	// Open wall.bmp to get floor image
	char wall_filename[50];
	sprintf(wall_filename, "wall.bmp");
	FILE *wall_file = fopen(wall_filename, "rb");
	
	if (robot_positions_file == NULL || floor_file == NULL || wall_file == NULL)
	{
		// Report which file(s) could not be opened
		if (robot_positions_file == NULL)
		{
			fprintf(stderr, "Error opening %s\n", robot_positions_filename);
		}
		else fclose(robot_positions_file);
		
		if (floor_file == NULL)
		{
			fprintf(stderr, "Error opening %s\n", floor_filename);
		}
		else fclose(floor_file);
		
		if (wall_file == NULL)
		{
			fprintf(stderr, "Error opening %s\n", wall_filename);
		}
		else fclose(wall_file);

		return 1;
	}
	
	// Initialise robots' states
	int n;
	for (n=0 ; n<2 ; ++n)
	{
		robot[n].active = 0; // inactive until a competitor connects over the network
		robot[n].name[20];
		robot[n].w = 0.16; // 16cm wide
		robot[n].l = 0.20; // 20cm long
		robot[n].h = 0.10; // 10cm high

		robot[n].LATA = 0;
		robot[n].LATB = 0;
		robot[n].LATC = 0;
		robot[n].LATD = 0;
	
		// PWM output registers
		robot[n].CCPR1L = 255;
		robot[n].CCPR2L = 255;
		
		// Analog inputs
		robot[n].AN[0] = 0;
		robot[n].AN[1] = 0;
		robot[n].AN[2] = 0;
		robot[n].AN[3] = 0;
		robot[n].AN[4] = 0;
		robot[n].AN[5] = 0;
		robot[n].AN[6] = 0;
		robot[n].AN[7] = 0;
	}
		
	char word[20];
	double value;
	while(1)
	{
		if (fscanf(robot_positions_file, "%s", word) != 1)
		{
			fprintf(stderr, "Error reading word from file %s\n", robot_positions_filename);
			break;
		}
		
		if (strcmp(word, "END") == 0) break;
		else if (strcmp(word, "ROBOT_0_X") == 0)
		{
			if (fscanf(robot_positions_file, "%lf", &value) != 1)
			{
				fprintf(stderr, "Error reading value ROBOT_0_X from file %s\n", robot_positions_filename);
				break;
			}
			robot[0].x = value;
		}
		else if (strcmp(word, "ROBOT_0_Y") == 0)
		{
			if (fscanf(robot_positions_file, "%lf", &value) != 1)
			{
				fprintf(stderr, "Error reading value ROBOT_0_Y from file %s\n", robot_positions_filename);
				break;
			}
			robot[0].y = value;
		}
		else if (strcmp(word, "ROBOT_0_ANGLE") == 0)
		{
			if (fscanf(robot_positions_file, "%lf", &value) != 1)
			{
				fprintf(stderr, "Error reading value ROBOT_0_ANGLE from file %s\n", robot_positions_filename);
				break;
			}
			robot[0].angle = value;
		}
		else if (strcmp(word, "ROBOT_0_MAX_RANDOM_ANGLE_OFFSET") == 0)
		{
			if (fscanf(robot_positions_file, "%lf", &value) != 1)
			{
				fprintf(stderr, "Error reading value ROBOT_0_MAX_RANDOM_ANGLE_OFFSET from file %s\n", robot_positions_filename);
				break;
			}
			robot[0].max_random_angle_offset = value;
		}
		else if (strcmp(word, "ROBOT_1_X") == 0)
		{
			if (fscanf(robot_positions_file, "%lf", &value) != 1)
			{
				fprintf(stderr, "Error reading value ROBOT_1_X from file %s\n", robot_positions_filename);
				break;
			}
			robot[1].x = value;
		}
		else if (strcmp(word, "ROBOT_1_Y") == 0)
		{
			if (fscanf(robot_positions_file, "%lf", &value) != 1)
			{
				fprintf(stderr, "Error reading value ROBOT_1_Y from file %s\n", robot_positions_filename);
				break;
			}
			robot[1].y = value;
		}
		else if (strcmp(word, "ROBOT_1_ANGLE") == 0)
		{
			if (fscanf(robot_positions_file, "%lf", &value) != 1)
			{
				fprintf(stderr, "Error reading value ROBOT_1_ANGLE from file %s\n", robot_positions_filename);
				break;
			}
			robot[1].angle = value;
		}
		else if (strcmp(word, "ROBOT_1_MAX_RANDOM_ANGLE_OFFSET") == 0)
		{
			if (fscanf(robot_positions_file, "%lf", &value) != 1)
			{
				fprintf(stderr, "Error reading value ROBOT_1_MAX_RANDOM_ANGLE_OFFSET from file %s\n", robot_positions_filename);
				break;
			}
			robot[1].max_random_angle_offset = value;
		}
	}

	// Add random angle offsets
	robot[0].angle += robot[0].max_random_angle_offset * (2 * (rand() / (double)RAND_MAX) - 1);
	robot[1].angle += robot[1].max_random_angle_offset * (2 * (rand() / (double)RAND_MAX) - 1);

	// Load floor image from file
	// Load wall pattern from bmp file
	//
	// NB Wall image is from:
	//
	//   http://dsibley.deviantart.com/art/Blue-Grey-Stone-Texture-129579594
	//
	// Read bitmap header - assume it's 0x36 bytes long
	int elements_read;
	elements_read = fread(texWallImage, 1, 0x36, wall_file);
	if (elements_read != 0x36)
	{
		fprintf(stderr, "Error reading header from file %s\n", wall_filename);
		fclose(robot_positions_file);
		fclose(floor_file);
		fclose(wall_file);
		return 1;
	}
	elements_read = fread(texWallImage, 3, texImageWidth*texImageHeight, wall_file);
	if (elements_read != texImageWidth*texImageHeight)
	{
		fprintf(stderr, "Error reading image data from file %s\n", wall_filename);
		fclose(robot_positions_file);
		fclose(floor_file);
		fclose(wall_file);
		return 1;
	}
	
	// Load floor pattern from bmp file
	// Read bitmap header - assume it's 0x36 bytes long
	elements_read = fread(texFloorImage, 1, 0x36, floor_file);
	if (elements_read != 0x36)
	{
		fprintf(stderr, "Error reading header from file %s\n", floor_filename);
		fclose(robot_positions_file);
		fclose(floor_file);
		fclose(wall_file);
		return 1;
	}
	elements_read = fread(texFloorImage, 3, texImageWidth*texImageHeight, floor_file);
	if (elements_read != texImageWidth*texImageHeight)
	{
		fprintf(stderr, "Error reading image data from file %s\n", floor_filename);
		fclose(robot_positions_file);
		fclose(floor_file);
		fclose(wall_file);
		//return 1;
	}
	
	// Convert floor and wall images from BGR to RGB
	int x, y;
	GLubyte temp;
	for (y = 0 ; y < texImageHeight ; ++y)
	{
		for (x = 0 ; x < texImageWidth ; ++x)
		{
			temp = texFloorImage[y][x][0];
			texFloorImage[y][x][0] = texFloorImage[y][x][2];
			texFloorImage[y][x][2] = temp;

			temp = texWallImage[y][x][0];
			texWallImage[y][x][0] = texWallImage[y][x][2];
			texWallImage[y][x][2] = temp;
		}
	}
	
	// Create OpenGL texture for wall
	glBindTexture(GL_TEXTURE_2D, texWallName);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texImageWidth, texImageHeight,
					0, GL_RGB, GL_UNSIGNED_BYTE, texWallImage);
	
	// Create OpenGL texture for floor
	glBindTexture(GL_TEXTURE_2D, texFloorName);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texImageWidth, texImageHeight,
					0, GL_RGB, GL_UNSIGNED_BYTE, texFloorImage);
	
	// Close files
	fclose(robot_positions_file);
	fclose(floor_file);
	fclose(wall_file);
	
	return 0;
}

// This function obtains the background colour (actually intensity between 0 and 255)
// at a specified OpenGL coordinate
GLubyte get_colour(GLfloat x, GLfloat y)
{
	// NB The loaded floor texture is mapped onto the square area from top left
	// at (-0.7, 0.7) to bottom right at (0.7, -0.7).
	//
	// NB The first line in the bitmap data is actually the bottom row of pixels
	// in the image.
	
	// First translate x and y to the range 0 to 1
	x = (x + 0.7) / 1.4;
	y = (y + 0.7) / 1.4;
	if (x > 1) x = 1;
	if (x < 0) x = 0;
	if (y > 1) y = 1;
	if (y < 0) y = 0;

	// Now identify the corresponding pixel coordinate
	GLint px, py;
	px = (GLint)(texImageWidth * x);
	py = (GLint)(texImageHeight * y);
	
	// Finally, calculate and return the average colour component value at that point
	return (GLubyte)((texFloorImage[py][px][0] + texFloorImage[py][px][1] + texFloorImage[py][px][2]) / 3.0);
}

void update()
{
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
	
	// Insert a short delay here.
	// The purpose of this is to get this thread to step down
	// in case it's hogging all the processor time.
	// This function is registered as the GLUT idle function,
	// so it should get called very frequently. I observed
	// that the primary thread hogging the processor in the client
	// program adversely affected the performace of the robot,
	// so I inserted short delays in both threads (in the client
	// and server programs) to make sure neither dominates.
	Sleep(1);
	
	// Calculate time elapsed since last update
	double tau;
	clock_t new_time;
	new_time = clock();
	if (new_time > last_time) tau = (new_time - last_time) / (double)CLOCKS_PER_SEC;
	else tau = 0; // Just in case clock value reaches max value and wraps around
	last_time = new_time;
	
	// Update robot positions
	GLfloat x1, y1, x2, y2;
	int n;
	for (n=0 ; n<=1 ; ++n)
	{
		// Update wheel velocities
		robot[n].v1 = 0;
		robot[n].v2 = 0;
		if (robot[n].LATD & 0x02) robot[n].v1 += 1;
		if (robot[n].LATD & 0x01) robot[n].v1 -= 1;
		robot[n].v1 *= (robot[n].CCPR1L / 1000.0);
		if (robot[n].LATD & 0x08) robot[n].v2 += 1;
		if (robot[n].LATD & 0x04) robot[n].v2 -= 1;
		robot[n].v2 *= (robot[n].CCPR2L / 1000.0);
		
		// Left wheel position: x1, y1. Left wheel velocity: v1.
		// Right wheel position: x2, y2. Left wheel velocity: v2.
		x1 = robot[n].x - (robot[n].w/2.0)*sin(robot[n].angle) + tau*robot[n].v1*cos(robot[n].angle);
		y1 = robot[n].y + (robot[n].w/2.0)*cos(robot[n].angle) + tau*robot[n].v1*sin(robot[n].angle);
		x2 = robot[n].x + (robot[n].w/2.0)*sin(robot[n].angle) + tau*robot[n].v2*cos(robot[n].angle);
		y2 = robot[n].y - (robot[n].w/2.0)*cos(robot[n].angle) + tau*robot[n].v2*sin(robot[n].angle);
		robot[n].x = (x1 + x2) / 2.0;
		robot[n].y = (y1 + y2) / 2.0;
		
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
		
		// Update sensor readings
		// First, calculate offset vector from centre of robot
		// to mid-right-side v1 = (x1, y1) and from centre of
		// robot to front-centre-point v2 = (x2, y2)
		GLfloat x = robot[n].x;
		GLfloat y = robot[n].y;
		GLfloat x1 = (robot[n].w/2.0)*sin(robot[n].angle);
		GLfloat y1 = -(robot[n].w/2.0)*cos(robot[n].angle);
		GLfloat x2 = (robot[n].l/2.0)*cos(robot[n].angle);
		GLfloat y2 = (robot[n].l/2.0)*sin(robot[n].angle);
		robot[n].AN[0] = get_colour(x - x1 + x2, y - y1 + y2); // front left
		robot[n].AN[1] = get_colour(x + x1 + x2, y + y1 + y2);; // front right
		robot[n].AN[2] = get_colour(x - x1 - x2, y - y1 - y2);; // back left
		robot[n].AN[3] = get_colour(x + x1 - x2, y + y1 - y2);; // back right
	}
	
	// Request redrawing of scene
	glutPostRedisplay();
}

void reshape(int w, int h)
{
	window_width = w;
	window_height = h;

	int text_bar_height = 25;
	
	// Set viewport to new window size
	glViewport(0, 0, window_width, window_height);
	
	// Calculate dimensions for orthographic OpenGL projection
	if (window_width > window_height - text_bar_height)
	{
		ortho_top = 0.7;
		ortho_bottom = -0.7 * (window_height + 2.0*text_bar_height) / (double)window_height;
		ortho_right = 0.7 * (window_width/(double)(window_height-text_bar_height));
		ortho_left = -ortho_right;
	}
	else
	{
		ortho_right = 0.7;
		ortho_left = -ortho_right;
		ortho_top = 0.7 * ((window_height-text_bar_height)/(double)window_width);
		ortho_bottom = -ortho_top * (window_height + 2.0 * text_bar_height) / (double)window_height;
	}
	
	// Update position of network address text
	// NB string starts with a couple of spaces, so ok to place at left edge
	x_networkAddressText = ortho_left;
	y_networkAddressText = ortho_top - 0.1;
}

void display()
{
	// Clear the background
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// Select appropriate projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (orthographic_projection == 1)
	{
		// Specify orthographic projection
		glOrtho(ortho_left, ortho_right, ortho_bottom, ortho_top, -1, 100);
		glDisable(GL_LIGHTING);
	}
	else
	{
		// Specify perspective projection - fovy, aspect, zNear, zFar
		gluPerspective(30, 1.0*window_width/window_height, 1, 100);
		glEnable(GL_LIGHTING);
	}
	
	// Render bitmaps on arena floor and walls
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0, 0, camera_distance, 0, 0, 0, 0, 1, 0);
	if (orthographic_projection == 0)
	{
		glRotatef(-camera_latitude, 1.0, 0.0, 0.0);
		glRotatef(-camera_longitude, 0.0, 0.0, 1.0);
	}
	
	GLfloat light_position[] = {-2.0, -2.0, 2.0, 0.0};
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);

	glEnable(GL_DEPTH_TEST);
	
	// Wall bitmaps
	glBindTexture(GL_TEXTURE_2D, texWallName);
	glEnable(GL_TEXTURE_2D);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	int wall;
	for (wall=0 ; wall<4 ; ++wall)
	{
		glRotatef(90.0, 0.0, 0.0, 1.0);
		glBegin(GL_QUADS);
		glNormal3d(0, 1.0, 0);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-0.7, 0.7, 0.0);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(0.7, 0.7, 0.0);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(0.7, 0.7, -1.4);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-0.7, 0.7, -1.4);
		glEnd();
	}
	
	// Floor bitmap
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBindTexture(GL_TEXTURE_2D, texFloorName);
	glBegin(GL_QUADS);
	glNormal3d(0, 0, 1);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-0.7, 0.7, 0.0);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(0.7, 0.7, 0.0);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(0.7, -0.7, 0.0);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-0.7, -0.7, 0.0);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	
	// Draw robots
	glEnable(GL_DEPTH_TEST);
	int n;
	for (n=0 ; n<=1 ; ++n)
	{
		// rectangular robot body
		if (n == 0) glColor4f(1.0, 0.0, 0.0, 1.0);
		else glColor4f(0.0, 0.0, 1.0, 1.0);
		glLoadIdentity();
		gluLookAt(0, 0, camera_distance, 0, 0, 0, 0, 1, 0);
		if (orthographic_projection == 0)
		{
			glRotatef(-camera_latitude, 1, 0, 0);
			glRotatef(-camera_longitude, 0, 0, 1);
		}
		glTranslatef(robot[n].x, robot[n].y, robot[n].h/2.0);
		glRotatef(robot[n].angle * (180.0/pi), 0.0, 0.0, 1.0);
		glScalef(robot[n].l, robot[n].w, robot[n].h);
		glutSolidCube(1.0);
		// Simple indicator which end of the robot is the front
		glColor3f(1.0, 1.0, 1.0);
		glLoadIdentity();
		gluLookAt(0, 0, camera_distance, 0, 0, 0, 0, 1, 0);
		if (orthographic_projection == 0)
		{
			glRotatef(-camera_latitude, 1, 0, 0);
			glRotatef(-camera_longitude, 0, 0, 1);
		}
		glTranslatef(robot[n].x, robot[n].y, robot[n].h);
		glRotatef(robot[n].angle * (180.0/pi), 0.0, 0.0, 1.0);
		glScalef(robot[n].l, robot[n].w, robot[n].w);
		glTranslatef(-0.25, 0.0, 0.0);
		glRotatef(90.0, 0.0, 1.0, 0.0);
		glutSolidCone(0.25, 0.5, 20, 10);
	}
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	
	// Specify OpenGL projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(ortho_left, ortho_right, ortho_bottom, ortho_top, -1, 100);

	// Draw text information
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glColor3f(0.0, 0.0, 0.0);
	renderBitmapString(x_networkAddressText, y_networkAddressText, 0.0,
						GLUT_BITMAP_HELVETICA_18, network_address_display_string);
	
	// Swap back buffer to screen
	glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y)
{
	if (key == 'q') program_exiting = 1; // Set exiting flag
	if (key == '1') initialise_scene(1);
	if (key == '2') initialise_scene(2);
	if (key == '3') initialise_scene(3);
	if (key == '4') initialise_scene(4);
	if (key == 'v') orthographic_projection = (orthographic_projection) ? 0 : 1;
}

void keyboardSpecial(int key, int x, int y)
{
	if (key == GLUT_KEY_UP) camera_distance -= 0.1;
	if (key == GLUT_KEY_DOWN) camera_distance += 0.1;
	if (key == GLUT_KEY_LEFT) camera_longitude -= 3.0;
	if (key == GLUT_KEY_RIGHT) camera_longitude += 3.0;
}

int mouse_previous_x, mouse_previous_y;

void mouse(int button, int state, int x, int y)
{
	if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN))
	{
		mouse_previous_x = x;
		mouse_previous_y = y;
	}
}

void mouseDrag(int x, int y)
{
	camera_latitude -= (y - mouse_previous_y)/2.0;
	camera_longitude -= (x - mouse_previous_x)/2.0;
	mouse_previous_x = x;
	mouse_previous_y = y;
	
	if (camera_latitude > 90.0) camera_latitude = 90.0;
	if (camera_latitude < 0.0) camera_latitude = 0.0;
	if (camera_longitude > 360.0) camera_longitude -= 360.0;
	if (camera_longitude < 0.0) camera_longitude += 360.0;
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
