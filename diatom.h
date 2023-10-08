#ifndef DIATOM
#define DIATOM

#include "util.h"

#define INSTRUCTION_COUNT 23
#define INSTRUCTION_NAME_MAX 10
#define WORD_NAME_MAX 10

enum instructions {
  EXIT,
  NOP,
  CONST,
  FETCH,
  STORE,
  ADD,
  SUBTRACT,
  MULTIPLY,
  DIVIDE,
  DUP,
  DROP,
  SWAP,
  OVER,
  CJUMP,
  CALL,
  RETURN,
  KEY,
  EMIT,
  EQUALS,
  NOT,
  LT,
  GT,
  RPOP,
};

char instruction_names[INSTRUCTION_COUNT][INSTRUCTION_NAME_MAX] = {
  "exit",
  "nop",
  "const",
  "@",
  "!",
  "+",
  "-",
  "*",
  "/",
  "dup",
  "drop",
  "swap",
  "over",
  "cjmp",
  "call",
  "ret",
  "key",
  "emit",
  "=",
  "~",
  "<",
  ">",
  "rpop",
};

int name_to_opcode(char* name) {
    for (unsigned int i = 0; i < INSTRUCTION_COUNT; ++i)
      if (dlt_string_equals(instruction_names[i], name)) return i;

    return -1;
}

typedef unsigned int word;

#endif
