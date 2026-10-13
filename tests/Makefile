CC ?= cc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -I../common
COMMON = $(wildcard ../common/*.c)
TESTS = $(wildcard test_*.c) stubs.c
HEADERS = $(wildcard ../common/*.h) $(wildcard *.h)

run: tests
	./tests

tests: $(TESTS) $(COMMON) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(TESTS) $(COMMON) -lm

clean:
	rm -rf tests tests.dSYM

.PHONY: run clean
