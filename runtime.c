#include <stdio.h>

#define STACK_SIZE (10)
typedef long int word;

/* Stacks */
typedef struct {
  word pointer;
  word data[STACK_SIZE];
} stack;

static void stack_push(stack* s, word value) {
  const word index = s->pointer++ % STACK_SIZE;
  s->data[index] = value;
}

static word stack_pop(stack* s) {
  const word index = --s->pointer % STACK_SIZE;
  return s->data[index];
}

static word stack_peek(stack* s) {
  const word index = (s->pointer - 1) % STACK_SIZE;
  return s->data[index];
}

/* VM State */
word instruction_pointer = 0;

stack* data_stack = &(stack) {
  .pointer = 0,
};

stack* call_stack = &(stack) {
  .pointer = 0,
};

word memory[100];

/* Instructions */
static inline void _next(void) {
  instruction_pointer = memory[instruction_pointer];
  ++instruction_pointer;
  ((void (*)(void))instruction_pointer)();
}

static void _nest(void) {
}

static void _call_native(void) {
}

static void _unnest(void) {
}

static void _add(void) {
  stack_push(data_stack, stack_pop(data_stack) + stack_pop(data_stack));
}

static void _fetch(void) {
  stack_push(data_stack, memory[stack_pop(data_stack)]);
}

static void _store(void) {
  const word value = stack_pop(data_stack);
  memory[stack_pop(data_stack)] = value;
}

// TODO: Remove
static void test(void) {
  puts("test!");
}

int main(void) {
  word code_segment;

  memory[0] = (word)test;
  memory[1] = (word)_next;
  goto next;

nest:
  stack_push(call_stack, instruction_pointer);
  instruction_pointer = memory[instruction_pointer];
  goto next;

unnest:
  instruction_pointer = stack_pop(call_stack);
  goto next;

next:
  code_segment = memory[instruction_pointer];
  ++instruction_pointer;
  if (code_segment == (word)_next) {
    goto next;
  }
  else if (code_segment == (word)_nest) {
    goto nest;
  }
  else if (code_segment == (word)_unnest) {
    goto unnest;
  }
  // Native code call
  ((void (*)(void))code_segment)();
  goto next;

  return 0;
}
