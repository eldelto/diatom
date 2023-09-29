#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "diatom.h"
#include "util.h"

#define TOKEN_MAX 16
#define LINE_MAX 90

struct tokenizer {
  FILE *file;
  char token[TOKEN_MAX];
  char line_buffer[LINE_MAX];
  unsigned int line_number;
};

static struct tokenizer new_tokenizer(FILE *f) {
  return (struct tokenizer) {
    .file = f,
    .token = {'\0'},
    .line_buffer = {'\0'},
    .line_number = 0,
  };
}

// next_line reads the next line from the input file. It returns the
// length of the read line, 0 if EOF has been reached or -1 if an
// error occured.
static int next_line(struct tokenizer *t) {
  assert(t != NULL);

  int err = 0;
  char *line = NULL;
  size_t line_cap = 0;
  ssize_t line_len = 0;

    t->line_number++;
    line_len = getline(&line, &line_cap, t->file);
    if (line_len == -1) {
      if (feof(t->file)) return 0;

      return dlt_errorf("line %d: error while reading line",
			t->line_number);
    }


  if ((unsigned long)line_len > sizeof(t->line_buffer)) {
    err = dlt_errorf("line %d: identifier '%s' exceeds max length",
		     t->line_number, line);
    goto cleanup;
  }

  size_t copy_len = strlcpy(t->line_buffer, line, sizeof(t->line_buffer));
  if (copy_len < (unsigned long)line_len) {
    err = dlt_errorf("line %d: failed to copy line to buffer",
		     t->line_number);
    goto cleanup;
  }

 cleanup:
  free(line);
  if (err) return err;
  
  return copy_len;
}

// next_token reads the token from the input file. It returns the
// length of the read token, 0 if EOF has been reached or -1 if an
// error occured.
static int next_token(struct tokenizer *t) {
  assert(t != NULL);
  
  if (t->token[0] != '\0') return 0;

  char *token = "\0";
  static char *line = NULL;
  
  while (true) {
    if (line == NULL) {
      int line_len = next_line(t);
      if (line_len <= 0) return line_len;

      line = t->line_buffer;
      continue;
    }
    token = strsep(&line, " \t\n\0");

    size_t token_len = strnlen(token, TOKEN_MAX);
    if (token_len > 0) {
      size_t copy_len = strlcpy(t->token, token, sizeof(t->token));
      if (copy_len < (unsigned long)token_len)
	return dlt_errorf("line %d: failed to copy token", t->line_number);

      return token_len;
    }
  }
}

static void consume_token(struct tokenizer *t) {
  t->token[0] = '\0';
}

//typedef int (*handler)(struct tokenizer *t, FILE *output_file);
//
//static int translate_file(FILE *in, FILE *out, handler h) {
//  struct tokenizer t = new_tokenizer(in);
//
//  int err = 0;
//  while ((err = next_token(&t)) > 0) {
//    err = h(&t, out);
//    if (err) return err;
//  }
//
//  return err;
//}

//static int create_output_file(char *input_filename,
//			      char *output_filename,
//			      handler handler,
//			      char *write_mode) {
//  int err = 0;
//
//  FILE* in = fopen(input_filename, "r");
//  if (in == NULL) {
//    return dlt_error("failed to open input file");
//  }
//
//  FILE* out = fopen(output_filename, write_mode);
//  if (out == NULL) {
//    err = dlt_error("failed to open input file");
//    goto close_input_file;
//  }
//
//  err = translate_file(in, out, handler);
//  if (err) {
//    goto close_files;
//  }
//
// close_files:
//  fclose(out);
// close_input_file:
//  fclose(in);
//
//  return err;
//}

static void usage(void) {
  puts("Usage: dasm [dasm-file]\n");
  puts("Flags:");
  puts("  -h - Displays this usage message.");
}

int main(int argc, char* argv[]) {
  int ch = 0;
  while ((ch = getopt(argc, argv, "h")) != -1) {
    switch (ch) {
    case 'h':
      usage();
      return EXIT_SUCCESS;
    default:
      usage();
      return EXIT_FAILURE;
    }
  }

  if (argc != 2) {
    usage();
    dlt_fatal_error("invalid arguments");
  }

  char *dasm_filename = argv[1];
  FILE* input_file = fopen(dasm_filename, "r");
  if (input_file == NULL)
    dlt_fatal_error("failed to open input file");
  
  struct tokenizer t = new_tokenizer(input_file);
  
  while (next_token(&t) > 0) {
    printf("line %d: '%s'\n", t.line_number, t.token);
    consume_token(&t);
  }

  dlt_panic_on_error();

  fclose(input_file);

  return EXIT_SUCCESS;
}
