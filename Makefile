all: robosimo.exe

robosimo.exe: main.o
	gcc -o robosimo.exe main.o `pkg-config --libs gtk+-2.0`

main.o: main.c
	gcc -c main.c `pkg-config --cflags gtk+-2.0`
