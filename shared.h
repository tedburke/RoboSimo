// shared.h - Ted Burke - 28-9-2009
//
// This file contains shared object definitions.

#ifndef SHARED_H
#define SHARED_H

// Robot object
struct robot_state
{
	unsigned char LATA;
	unsigned char LATB;
	unsigned char LATC;
	unsigned char LATD;
	unsigned char CCPR1L;
	unsigned char CCPR2L;

	int active;
	char name[20];
	double w, l, h; // width, length, height
	double x;
	double y;
	double angle;
	double v1; // left_wheel_speed;
	double v2; // right_wheel_speed;
};

// Create two robot_state structures
extern struct robot_state robot[];

// This flag is to be set to 1 when the program is exiting
extern int program_exiting;

#endif // SHARED_H
