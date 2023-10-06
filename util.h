#ifndef DELTO_UTIL
#define DELTO_UTIL

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


/* Error handling */
#define ERR_MSG_MAX 256
static char error_msg[ERR_MSG_MAX] = "";

int dlt_error(char msg[ERR_MSG_MAX]) {
  strlcpy(error_msg, msg, sizeof(error_msg));
  return -1;
}

void dlt_panic(void) {
  fputs("\033[31mError: ", stderr);
  perror(error_msg);
  fputs("\033[0m", stderr);
  exit(EXIT_FAILURE);
}

void dlt_fatal_error(char msg[100]) {
  dlt_error(msg);
  dlt_panic();
}

void dlt_panic_on_error(void) {
  if (error_msg[0] != '\0') dlt_panic();
}

int dlt_errorf(const char * restrict format, ...) {
  va_list ap;
  va_start(ap, format);

  if (vsnprintf(error_msg, sizeof(error_msg), format, ap) < 0)
    dlt_fatal_error("Failed to create errorf message.");

  va_end(ap);
  return -1;
}

/* String functions */
#define STR_MAX 256

bool dlt_string_equals(char *a, char *b) {
  size_t a_len = strnlen(a, STR_MAX);
  size_t b_len = strnlen(b, STR_MAX);
  return a_len == b_len && strncmp(a, b, a_len) == 0;
}

bool dlt_string_starts_with(char *s, char *prefix) {
  return strncmp(s, prefix, strlen(prefix)) == 0;
}

/* Random utility functions */
double dlt_clamp_value(double value, double min, double max) {
  if (value < min) return min;
  else if (value > max) return max;
  else return value;
}

void dlt_trim_string(char *string) {
  if (string == NULL || strnlen(string, STR_MAX) == 0) return;

  const char* start = string;
  while(isspace(*start) || *start == '\n' || *start == '\r') ++start;

  size_t len = strnlen(start, STR_MAX);
  memmove(string, start, len + 1);

  char* end = string + len - 1;
  while(end >= string  && (isspace(*end) || *end == '\n' || *end == '\r')) --end ;

  *++end = '\0';
}

#endif
