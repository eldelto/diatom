#ifndef UTIL
#define UTIL

#include <stdlib.h>
#include <string.h>

char error_msg[100] = "";
static int error(char msg[100]) {
  memcpy(error_msg, msg, sizeof(error_msg));
  return -1;
}

void panic(void) {
  printf("\033[31mError: %s\033[0m\n", error_msg);
  exit(EXIT_FAILURE);
}

void fatal_error(char msg[100]) {
  error(msg);
  panic();
}

#endif
