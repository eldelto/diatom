#include <stdio.h>
#include <stdbool.h>

#include "diatom.h"
#include "util.h"

#define STACK_SIZE  10
#define MEMORY_SIZE 8000
#define IO_BUFFER_SIZE 4096

/* Stacks */
struct stack {
  word pointer;
  word data[STACK_SIZE];
};

inline static void stack_push(struct stack* s, word value) {
  const word index = s->pointer++;
  if (index >= STACK_SIZE) dlt_fatal_error("stack overflow");
  s->data[index] = value;
}

inline static word stack_pop(struct stack* s) {
  const word index = --s->pointer;
  if (index < 0) dlt_fatal_error("stack underflow");
  return s->data[index];
}

inline static word stack_peek(struct stack* s) {
  const word index = (s->pointer - 1) % STACK_SIZE;
  return s->data[index];
}

/* I/O functions */
struct input {
  char buffer[IO_BUFFER_SIZE];
  size_t len;
  size_t cursor;
};

byte next_char(struct input *i) {
  if (i->cursor >= i->len) {
    size_t len = fread(i->buffer, sizeof(i->buffer[0]), 1, stdin);
    if (len == 0) {
      if (feof(stdin)) return '\0';
      dlt_fatal_error("failed to read from stdin");
    }

    i->len = len;
    i->cursor = 0;
  }

  return (byte)i->buffer[i->cursor++];
}

/* VM State */

// Registers
word instruction_pointer = 0;

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
byte memory[MEMORY_SIZE] = { EXIT };
struct input input_buffer = (struct input) {
  .buffer = { '\0' },
  .len = 0,
  .cursor = 0,
};

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

  word memory_offset = 0;
  int err = 0;
  while (fread(&memory[memory_offset], 1, sizeof(byte), input_file)) {
    if (++memory_offset >= MEMORY_SIZE) {
      err = dlt_error("exceeded available memory");
      break;
    }
  }

  fclose(input_file);
  return err;
}

static byte fetch_byte(word addr) {
  return memory[addr];
}

static void store_byte(word addr, byte b) {
  memory[addr] = b;
}

static word fetch_word(word addr) {
  word w = 0;
  for (unsigned int i = 0; i < WORD_SIZE; ++i) {
    word b = (word)fetch_byte(addr + i);
    w |= (b << (WORD_SIZE - (i+1)) * 8);
  }

  return w;
}

static void store_word(word addr, word w) {
  byte buf[WORD_SIZE] = { 0 };
  word_to_bytes(w, buf);

  for (unsigned int i = 0; i < WORD_SIZE; ++i)
    store_byte(addr + i, buf[i]);
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

    printf("ds -> ");
    for (int i = data_stack->pointer - 1; i >= 0; --i)
      printf("%d ", data_stack->data[i]);
    
    printf("| rs -> %d | ip = %d | instr = %s\n",
	   rpeek(), instruction_pointer, instruction_names[instruction]);

    switch (instruction) {
    case EXIT: {
      puts("\nVM exited normally");
      return 0;
    } 
    case NOP: {
      break;
    }
    case CONST: {
      ++instruction_pointer;
      push(fetch_word(instruction_pointer));

      instruction_pointer += WORD_SIZE;
      continue;
    }
    case FETCH: {
      const word address = pop();
      push(fetch_word(address));
      break;
    }
    case STORE: {
      const word address = pop();
      store_word(address, pop());
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
      push(pop() / value);
      break;
    }
    case MOD: {
      const word value = pop();
      push(pop() % value);
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
    case CJUMP: {
      ++instruction_pointer;
      if ((int)pop() == -1)
	instruction_pointer = fetch_word(instruction_pointer);
      else
	instruction_pointer += WORD_SIZE;
      
      continue;
    }
    case CALL: {
      ++instruction_pointer;
      rpush(instruction_pointer + WORD_SIZE);
      instruction_pointer = fetch_word(instruction_pointer);
      continue;
    }
    case RETURN: {
      instruction_pointer = rpop();
      continue;
    }
    case KEY: {
      char c = next_char(&input_buffer);
      push(c);
      break;
    }
    case EMIT: {
      //putchar((char)pop());
      printf("\n-->'%c'\n\n", (char)pop());
      break;
    }
    case EQUALS: {
      if (pop() == pop()) push(-1);
      else push(0);
      break;
    }
    case NOT: {
      push(~pop());
      break;
    }
    case LT: {
      if ((int)pop() > (int)pop()) push(-1);
      else push(0);
      break;
    }
    case GT: {
      if ((int)pop() < (int)pop()) push(-1);
      else push(0);
      break;
    }
    case RPOP: {
      push(rpop());
      break;
    }
    case RPUT: {
      rpush(pop());
      break;
    }
    case RPEEK: {
      push(rpeek());
      break;
    }
    case BFETCH: {
      const word address = pop();
      push(fetch_byte(address));
      break;
    }
    case BSTORE: {
      const word address = pop();
      store_byte(address, pop() & 0xFF);
      break;
    }
//    case NATIVE: {
//      const pointer_t function = native_functions[index];
//      function();
//      break;
//    }
    default: {
      printf("Unknown instruction '%d' at memory location %d - aborting.",
	     instruction, instruction_pointer);
      return EXIT_FAILURE;
    }
    }

    ++instruction_pointer;
  }

  return 0;
}
