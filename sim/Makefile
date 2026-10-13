CC ?= cc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2 -g -I../common $(shell sdl2-config --cflags)
LDLIBS = $(shell sdl2-config --libs) -lm
COMMON = $(wildcard ../common/*.c)
HEADERS = $(wildcard ../common/*.h) sim.h

deskfish_sim: main.c platform_sdl.c $(COMMON) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ main.c platform_sdl.c $(COMMON) $(LDLIBS)

clean:
	rm -rf deskfish_sim deskfish_sim.dSYM

.PHONY: clean
