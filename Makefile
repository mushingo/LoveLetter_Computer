


CC = gcc
CFLAGS = -Wall -pedantic -std=gnu99
DEBUG = -g
TARGETS = hub player

.DEFAULT: all

.PHONY: all debug clean

all: $(TARGETS)

debug: CFLAGS += $(DEBUG)
debug: clean $(TARGETS)

shared.o: shared.c shared.h
	$(CC) $(CFLAGS) -c shared.c -o shared.o

player: player.c shared.o
	$(CC) $(CFLAGS) player.c shared.o -o player

hub: hub.c shared.o
	$(CC) $(CFLAGS) hub.c shared.o -o hub

clean:
	rm -f $(TARGETS) *.o
