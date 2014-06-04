
CC = gcc
LD = ld

#CFLAGS += -Wall -O2 `sdl-config --cflags`
CFLAGS += -O3 `sdl-config --cflags` -g

LIBS = -lm `sdl-config --libs` -lGL -lGLU



all: eye

eye: view.o process.o
	$(CC) -o eye view.o process.o $(CFLAGS) $(LDFLAGS) $(LIBS)

view.o: view.c
	$(CC) $(CFLAGS) -c -o view.o view.c

process.o: process.c
	$(CC) $(CFLAGS) -c -o process.o process.c

clean:
	rm -Rf *.o; rm -Rf eye 
