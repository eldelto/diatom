# Use GCC as compiler.
CC := gcc
# Set additional compiler flags.
CFLAGS  := -Wall -Werror -Wextra -pedantic-errors -std=c99 -MMD -MP

.PHONY: all
all: bin bin/runtime

bin:
	mkdir bin

bin/runtime: runtime.o
	$(CC) $^ -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f bin/*
	rm -f *.o
	rm -f *.d

-include *.d
