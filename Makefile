.DELETE_ON_ERROR:

# Set default compiler
CC := clang

# Set additional compiler flags.
CFLAGS  := -Wall -Werror -Wextra -pedantic-errors \
	-ftrapv \
	-Wno-macro-redefined \
        -D_FORTIFY_SOURCE=2 \
        -fsanitize=address \
	-g3 \
        -O2 \
        -std=c17 -MMD -MP

.PHONY: all
all: bin bin/runtime bin/assembler bin/assembler-v2

bin:
	mkdir bin

bin/runtime: runtime.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

bin/assembler: assembler.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

bin/assembler-v2: assembler_v2.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f bin/*
	rm -f *.o
	rm -f *.d

HEADER_DEPS := $(shell find . -name '*.d')
-include $(HEADER_DEPS)
