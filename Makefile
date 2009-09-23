all: robosimo.exe gluttest.exe

gluttest.exe: gluttest.o
	gcc -o gluttest.exe gluttest.o glut32.lib -lopengl32

gluttest.o: gluttest.c
	gcc -c gluttest.c

robosimo.exe: main.o
	gcc -o robosimo.exe main.o `pkg-config --libs gtk+-2.0`

main.o: main.c
	gcc -c main.c `pkg-config --cflags gtk+-2.0`
