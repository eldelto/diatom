#define STACK_SIZE (10)

/* Stacks */
typedef struct {
  unsigned int pointer;
  int data[STACK_SIZE];
} stack;

static void stack_push(stack* s, int value) {
  const unsigned int index = s->pointer++ % STACK_SIZE;
  s->data[index] = value;
}

static int stack_pop(stack* s) {
  const unsigned int index = --s->pointer % STACK_SIZE;
  return s->data[index];
}

static int stack_peek(stack* s) {
  const unsigned int index = (s->pointer - 1) % STACK_SIZE;
  return s->data[index];
}

/* VM State */
unsigned int instruction_pointer = 0;

stack* data_stack = &(stack) {
  .pointer = 0,
};

stack* call_stack = &(stack) {
  .pointer = 0,
};

int memory[100];

/* Instructions */
static inline void _next(void) {
  instruction_pointer = memory[instruction_pointer];
  ++instruction_pointer;
  ((void (*)(void))instruction_pointer)();
}

static void _nest(void) {
  stack_push(call_stack, instruction_pointer);
  instruction_pointer = memory[instruction_pointer];
}

static void _call_native(void) {
  stack_push(call_stack, instruction_pointer);
  ((void (*)(void))instruction_pointer)();
}

static void _return(void) {
  instruction_pointer = stack_pop(call_stack);
  _next();
}

static void _add(void) {
  stack_push(data_stack, stack_pop(data_stack) + stack_pop(data_stack));
}

static void _fetch(void) {
  stack_push(data_stack, memory[stack_pop(data_stack)]);
}

static void _store(void) {
  const int value = stack_pop(data_stack);
  memory[stack_pop(data_stack)] = value;
}

int main(void) {

next:
  instruction_pointer = memory[instruction_pointer];
  ++instruction_pointer;
  const int code_segment = memory[instruction_pointer];
  if (code_segment == _next) {
    goto next;
  }
  // Native code call
  ((void (*)(void))memory[instruction_pointer])();

nest:
  stack_push(call_stack, instruction_pointer);
  instruction_pointer = memory[instruction_pointer];
  goto next;



  return 0;
}
