#include <stdio.h>
#include <stdbool.h>

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
  FETCH,
  STORE,
  EXIT,
  ADD,
  SUBTRACT,
  MULTIPLY,
  DIVIDE,
  CONST,
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
};

int main(void) {

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
      stack_push(data_stack, value - stack_pop(data_stack));
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
