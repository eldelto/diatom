#ifndef DIATOM
#define DIATOM

#include "util.h"

#define INSTRUCTION_COUNT 11
#define INSTRUCTION_NAME_MAX 10
enum instructions {
  EXIT,
  CONST,
  FETCH,
  STORE,
  ADD,
  SUBTRACT,
  MULTIPLY,
  DIVIDE,
  DUP,
  DROP,
  JUMP
};

char instruction_names[INSTRUCTION_COUNT][INSTRUCTION_NAME_MAX] = {
  "exit",
  "const",
  "fetch",
  "store",
  "add",
  "subtract",
  "multiply",
  "divide",
  "dup",
  "drop",
  "jump"
};

int name_to_opcode(char* name) {
    for (unsigned int i = 0; i < INSTRUCTION_COUNT; ++i)
      if (dlt_string_equals(instruction_names[i], name)) return i;

    return -1;
}

#endif
