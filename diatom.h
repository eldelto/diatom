#ifndef DIATOM
#define DIATOM

#include "util.h"

#define INSTRUCTION_COUNT 30
#define INSTRUCTION_NAME_MAX 10
#define WORD_NAME_MAX 10

typedef unsigned char byte;
typedef int word;
#define WORD_SIZE (sizeof(word) / sizeof(byte))

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
  MOD,
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
  AND,
  OR,
  LT,
  GT,
  RPOP,
  RPUT,
  RPEEK,
  BFETCH,
  BSTORE,
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
  "%",
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
  "&",
  "|",
  "<",
  ">",
  "rpop",
  "rput",
  "r@",
  "b@",
  "b!",
};

byte name_to_opcode(char* name) {
    for (unsigned int i = 0; i < INSTRUCTION_COUNT; ++i)
      if (dlt_string_equals(instruction_names[i], name)) return i;

    return -1;
}

void word_to_bytes(word w, byte buf[WORD_SIZE]) {
  for (unsigned int i = 0; i < WORD_SIZE; ++i) {
    buf[WORD_SIZE - (i+1)] = (w >> (i * 8)) & 0xFFu;
  }
}

#endif
