all: robosimo.exe socks.exe

robosimo.exe: main.o
	gcc -o robosimo.exe main.o glut32.lib -lopengl32

main.o: main.c
	gcc -c main.c

socks.exe: socks.o
	gcc -o socks.exe socks.o -lwsock32
	
socks.o: socks.c
	gcc -c socks.c
	
#opengltest.exe: opengltest.c
#	gcc -Wall -s -O2 -mwindows -o opengltest.exe opengltest.c -lopengl32 -lglu32
