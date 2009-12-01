all: robosimo.exe

robosimo.exe: main.o shared.o socks.o
	gcc -o robosimo.exe main.o shared.o socks.o glut32.lib -lopengl32 -lglu32 -lwsock32 -lWs2_32

main.o: main.c
	gcc -c main.c

socks.o: socks.c
	gcc -c socks.c

shared.o: shared.c
	gcc -c shared.c
