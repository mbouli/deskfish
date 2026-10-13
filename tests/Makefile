CC ?= cc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -I../common
COMMON = ../common/gfx.c
TESTS = test_main.c test_gfx.c

run: tests
	./tests

tests: $(TESTS) $(COMMON) test.h $(wildcard ../common/*.h)
	$(CC) $(CFLAGS) -o $@ $(TESTS) $(COMMON) -lm

clean:
	rm -rf tests tests.dSYM

.PHONY: run clean
