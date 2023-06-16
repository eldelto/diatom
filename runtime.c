#include <stdio.h>
#include <stdbool.h>

#include "util.h"

#define STACK_SIZE  10
#define MEMORY_SIZE 100
typedef int word;

/* Stacks */
struct stack {
  word pointer;
  word data[STACK_SIZE];
};

// TODO: Abort if index is out of bounds

inline static void stack_push(struct stack* s, word value) {
  const word index = s->pointer++ % STACK_SIZE;
  s->data[index] = value;
}

inline static word stack_pop(struct stack* s) {
  const word index = --s->pointer % STACK_SIZE;
  return s->data[index];
}

inline static word stack_peek(struct stack* s) {
  const word index = (s->pointer - 1) % STACK_SIZE;
  return s->data[index];
}

/* Instructions */
enum instructions {
  EXIT,
  CONST,
  FETCH,
  STORE,
  ADD,
  SUBTRACT,
  MULTIPLY,
  DIVIDE,
};

/* VM State */
word instruction_pointer = 0;

struct stack* data_stack = &(struct stack) {
  .pointer = 0,
    .data = { 0 },
};

struct stack* call_stack = &(struct stack) {
  .pointer = 0,
    .data = { 0 },
};

word memory[MEMORY_SIZE] = {
  CONST,
  10,
  CONST,
  5,
  ADD,
  EXIT,
  // NEXT: Increment instruction_pointer and set it to the current memory content
  // DROP
};

static int init_memory(char *filename) {
  FILE* input_file = fopen(filename, "r");
  if (input_file == NULL) {
    return error("failed to open input file");
  }

  unsigned int memory_offset = 0;
  while (fread(&memory[memory_offset], 1, sizeof(word), input_file)) {
    // TODO: Properly close file
    if (++memory_offset >= MEMORY_SIZE) fatal_error("exceeded available memory");
  }

  fclose(input_file);
  return 0;
}

static void usage(void) {
  puts("Usage: dvm [dopc-file]\n");
  puts("Flags:");
  puts("  -h - Displays this usage message.");
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    usage();
    fatal_error("invalid arguments");
  }

  char *dopc_filename = argv[1];
  init_memory(dopc_filename);

  while (instruction_pointer < MEMORY_SIZE) {
    const word instruction = memory[instruction_pointer];

    switch (instruction) {
    case FETCH: {
      const word address = stack_pop(data_stack);
      stack_push(data_stack, memory[address]);
      break;
    }
    case STORE: {
      const word address = stack_pop(data_stack);
      memory[address] = stack_pop(data_stack);
      break;
    }
    case EXIT: {
      puts("VM exited normally");
      return 0;
    }
    case ADD: {
      stack_push(data_stack, stack_pop(data_stack) + stack_pop(data_stack));
      break;
    }
    case SUBTRACT: {
      const word value = stack_pop(data_stack);
      stack_push(data_stack, stack_pop(data_stack) - value);
      break;
    }
    case MULTIPLY: {
      stack_push(data_stack, stack_pop(data_stack) * stack_pop(data_stack));
      break;
    }
    case DIVIDE: {
      const word value = stack_pop(data_stack);
      stack_push(data_stack, value / stack_pop(data_stack));
      break;
    }
    case CONST: {
      ++instruction_pointer;
      const word value = memory[instruction_pointer];
      stack_push(data_stack, value);
      break;
    }
    default: {
      printf("Unknown instruction at memory location %d - aborting.", instruction_pointer);
      return -1;
    }
    }

    printf("data stack -> %d\n", stack_peek(data_stack));

    ++instruction_pointer;
  }

  return 0;
}
