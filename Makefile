all: robosimo.exe

robosimo.exe: main.o
	gcc -o robosimo.exe main.o glut32.lib -lopengl32

main.o: main.c
	gcc -c main.c

#opengltest.exe: opengltest.c
#	gcc -Wall -s -O2 -mwindows -o opengltest.exe opengltest.c -lopengl32 -lglu32
