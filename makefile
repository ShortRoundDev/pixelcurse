CC=gcc
LDFLAGS=-lncurses -lm
CFLAGS=-std=gnu11

all:
	$(CC) pixelcurse.c -o pixelcurse $(LDFLAGS) $(CFLAGS)
