#include <stdio.h>
#include <stdbool.h>

#include "diatom.h"
#include "util.h"

#define STACK_SIZE  10
#define MEMORY_SIZE 1000

/* Stacks */
struct stack {
  word pointer;
  word data[STACK_SIZE];
};

inline static void stack_push(struct stack* s, word value) {
  const word index = s->pointer++ % STACK_SIZE;
  s->data[index] = value;
}

inline static word stack_pop(struct stack* s) {
  const word index = --s->pointer % STACK_SIZE;
  if (index < 0) dlt_fatal_error("stack underflow");
  return s->data[index];
}

inline static word stack_peek(struct stack* s) {
  const word index = (s->pointer - 1) % STACK_SIZE;
  return s->data[index];
}

/* VM State */

// Registers
word instruction_pointer = 0;
word r0 = 0;

// Stacks
struct stack* data_stack = &(struct stack) {
  .pointer = 0,
    .data = { 0 },
};

struct stack* return_stack = &(struct stack) {
  .pointer = 0,
    .data = { 0 },
};

// Memory
word memory[MEMORY_SIZE] = { EXIT };

// Helper functions
inline static void push(word value) {
  stack_push(data_stack, value);
}

inline static word pop(void) {
  return stack_pop(data_stack);
}

inline static word peek(void) {
  return stack_peek(data_stack);
}

inline static void rpush(word value) {
  stack_push(return_stack, value);
}

inline static word rpop(void) {
  return stack_pop(return_stack);
}

inline static word rpeek(void) {
  return stack_peek(return_stack);
}

static int init_memory(char *filename) {
  FILE* input_file = fopen(filename, "r");
  if (input_file == NULL) {
    return dlt_error("failed to open input file");
  }

  unsigned int memory_offset = 0;
  int err = 0;
  while (fread(&memory[memory_offset], 1, sizeof(word), input_file)) {
    if (++memory_offset >= MEMORY_SIZE) {
      err = dlt_error("exceeded available memory");
      break;
    }
  }

  fclose(input_file);
  return err;
}

static void usage(void) {
  puts("Usage: dvm [dopc-file]\n");
  puts("Flags:");
  puts("  -h - Displays this usage message.");
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    usage();
    dlt_fatal_error("invalid arguments");
  }

  char *dopc_filename = argv[1];
  if (init_memory(dopc_filename)) dlt_panic();

  while (instruction_pointer < MEMORY_SIZE) {
    const word instruction = memory[instruction_pointer];

    printf("ds -> %d | rs -> %d | ip = %d | r0 = %d | instr = %s\n",
	   peek(), rpeek(), instruction_pointer, r0, instruction_names[instruction]);

    switch (instruction) {
    case EXIT: {
      puts("VM exited normally");
      return 0;
    } 
    case NOP: {
      break;
    }
    case CONST: {
      ++instruction_pointer;
      const word value = memory[instruction_pointer];
      push(value);
      break;
    }
    case FETCH: {
      const word address = pop();
      push(memory[address]);
      break;
    }
    case STORE: {
      const word address = pop();
      memory[address] = pop();
      break;
    }
    case ADD: {
      push(pop() + pop());
      break;
    }
    case SUBTRACT: {
      const word value = pop();
      push(pop() - value);
      break;
    }
    case MULTIPLY: {
      push(pop() * pop());
      break;
    }
    case DIVIDE: {
      const word value = pop();
      push(value / pop());
      break;
    }
    case DUP: {
      push(peek());
      break;
    }
    case DROP: {
      pop();
      break;
    }
    case SWAP: {
      const word x = pop();
      const word y = pop();
      push(x);
      push(y);
      break;
    }
    case OVER: {
      const word x = pop();
      const word y = pop();
      const word z = pop();
      push(x);
      push(y);
      push(z);
      break;
    }
    case JUMP: {
      instruction_pointer = pop();
      continue;
    }
    case CALL: {
      rpush(instruction_pointer + 2);
      instruction_pointer = memory[instruction_pointer + 1];
      continue;
    }
    case RETURN: {
      instruction_pointer = rpop();
      continue;
    }
    default: {
      printf("Unknown instruction at memory location %d - aborting.", instruction_pointer);
      return -1;
    }
    }

    ++instruction_pointer;
  }

  return 0;
}
