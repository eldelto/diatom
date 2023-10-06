#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "diatom.h"
#include "util.h"

#define TOKEN_MAX 16
#define LABEL_MAX TOKEN_MAX + 10
#define LINE_MAX 90
#define LABELS_MAX 200

struct label {
  char name[LABEL_MAX];
  unsigned int address;
};

// TODO: Allocate on the heap.
struct label labels[LABELS_MAX] = {
  (struct label) {
    .name = "",
    .address = 0,
  }
};

static size_t label_offset = 0;
static int append_label(char name[LABEL_MAX], unsigned int address) {
  struct label l = {
    .name = "",
    .address = address,
  };
  strlcpy(l.name, name, LABEL_MAX);
  labels[label_offset] = l;

  if (label_offset == (LABELS_MAX - 1))
    return dlt_error("maximum number of labels reached");

  ++label_offset;
  return 0;
}

static const struct label * find_label(char *name) {
  for (size_t i = 0; i < label_offset; ++i) {
    const struct label *const label = &labels[i];
    if (dlt_string_equals(name, (char *)label->name)) {
      return label;
    }
  }

  return NULL;
}

struct tokenizer {
  FILE *file;
  char token[TOKEN_MAX];
  char line_buffer[LINE_MAX];
  unsigned int line_number;
};

static struct tokenizer new_tokenizer(FILE *f) {
  return (struct tokenizer) {
    .file = f,
    .token = "",
    .line_buffer = "",
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

  char *token = "";
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

static bool is_token_consumed(struct tokenizer *t) {
  return t->token[0] == '\0';
}

typedef int (*handler)(struct tokenizer *t, FILE *out);

static int translate_file(FILE *in, FILE *out, handler h) {
  struct tokenizer t = new_tokenizer(in);

  int err = 0;
  while ((err = next_token(&t)) > 0) {
    err = h(&t, out);
    if (err) return err;
  }

  return err;
}

static int create_output_file(char *input_filename,
			      char *output_filename,
			      handler handler,
			      char *write_mode) {
  int err = 0;

  FILE* in = fopen(input_filename, "r");
  if (in == NULL) {
    return dlt_error("failed to open input file");
  }

  FILE* out = fopen(output_filename, write_mode);
  if (out == NULL) {
    err = dlt_error("failed to open input file");
    goto close_input_file;
  }

  err = translate_file(in, out, handler);
  if (err) {
    goto close_files;
  }

 close_files:
  fclose(out);
 close_input_file:
  fclose(in);

  return err;
}

static int parse_error(struct tokenizer *t, char *expected) {
  return dlt_errorf("line %d: expected '%s' but got '%s'",
		    t->line_number, expected, t->token);
}

static int parse_comment(struct tokenizer *t, FILE *out) {
  (void)out;

  if (!dlt_string_equals(t->token, "(")) return 0;
  consume_token(t);

  int err = 0;
  while ((err = next_token(t)) > 0) {
    if (dlt_string_equals(t->token, ")")) {
      consume_token(t);
      return 0;
    }
    consume_token(t);
  }

  if (err >= 0) return parse_error(t, ")");
  return err;
}

static int parse_call(struct tokenizer *t, FILE *out) {
  char *token = t->token;
  if (!dlt_string_starts_with(token, "!") || strnlen(token, 2) < 2)
    return 0;

  token++;

  if (fprintf(out, "call @_dict%s\n", token) < 0)
    return dlt_error("failed to write to file");

  consume_token(t);
  return 0;
}

static int insert_dictionary_header(char word_name[TOKEN_MAX], FILE *out) {
  // Insert the start label.
  if (fprintf(out, ":%s\n", word_name) < 0)
    return dlt_error("failed to write to file");

  // Insert the address of the previous word.
  static char last_word_label[LABEL_MAX] = "";
  if (last_word_label[0] == '\0') {
    if (fputs("0\n", out) < 0)
      return dlt_error("failed to write to file");
  } else {
    char dict_label[LABEL_MAX] = "@";
    strlcat(dict_label, last_word_label, sizeof(dict_label));

    if (fputs(dict_label, out) == EOF)
      return dlt_error("failed to write to file");
    if (fputs("\n", out) == EOF)
      return dlt_error("failed to write to file");
  }
  memcpy(last_word_label, word_name, sizeof(last_word_label));

  // Insert the length and name of the word.
  const word word_len = strnlen(word_name, TOKEN_MAX);
  if (fprintf(out, "%d\n", word_len) < 0)
    return dlt_error("failed to write to file");

  for (unsigned int i = 0; i < word_len; ++i) {
    if (fprintf(out, "%d\n", (word)word_name[i]) < 0)
      return dlt_error("faield to write to file");
  }

  if (fprintf(out, ":_dict%s\n", word_name) < 0)
    return dlt_error("failed to write to file");

  return 0;
}

static int parse_codeword(struct tokenizer *t, FILE *out) {
  if (!dlt_string_equals(t->token, ".codeword")) return 0;
  consume_token(t);

  int err = 0;
  if (next_token(t) <= 0) return parse_error(t, "<codeword-name>");
  if ((err = insert_dictionary_header(t->token, out))) return err;
  consume_token(t);

  // Resolve the remaining entries.
  while ((err = next_token(t)) > 0) {
    if ((err = parse_comment(t, out))) return err;
    if ((err = parse_call(t, out))) return err;

    if (is_token_consumed(t)) continue;
    
    char *token = t->token;
    if (dlt_string_equals(token, ".end")) {
      // Return from the codeword.
      if (fputs("return\n", out) == EOF) return dlt_error("failed to write to file");

      consume_token(t);
      return 0;
    }

    if (fputs(token, out) == EOF) return dlt_error("failed to write to file");
    if (fputs("\n", out) == EOF) return dlt_error("failed to write to file");
    consume_token(t);
  }
  puts("DAMN");
  return parse_error(t, ".end");
}

static int parse_var(struct tokenizer *t, FILE *out) {
  if (!dlt_string_equals(t->token, ".var"))  return 0;
  consume_token(t);

  int err = 0;
  if (next_token(t) <= 0) return parse_error(t, "<var-name>");
  if ((err = insert_dictionary_header(t->token, out))) return err;

  // Put the variable's address on the data stack and return.
  if (fprintf(out,
	      "const\n"
	      "@_var%s\n"
	      "return\n", t->token) < 0)
    return dlt_error("failed to write to file");

  // Store the variable's value with a separate label.
  if (fprintf(out, ":_var%s\n", t->token) < 0)
    return dlt_error("failed to write to file");
  consume_token(t);

  if (next_token(t) <= 0) return parse_error(t, "<var-value>");
  if (fputs(t->token, out) == EOF) return dlt_error("failed to write to file");
  if (fputs("\n", out) == EOF) return dlt_error("failed to write to file");
  consume_token(t);

  // Check and consume .end token.
  if (next_token(t) <= 0) return parse_error(t, ".end");
  if (!dlt_string_equals(t->token, ".end")) return parse_error(t, ".end");
  consume_token(t);

  return 0;
}

static int parse_const(struct tokenizer *t, FILE *out) {
  if (!dlt_string_equals(t->token, ".const")) return 0;
  consume_token(t);

  int err = 0;
  if (next_token(t) <= 0) return parse_error(t, "<const-name>");
  if ((err = insert_dictionary_header(t->token, out))) return err;
  consume_token(t);

  if (next_token(t) <= 0) return parse_error(t, "<const-value>");
  if (fprintf(out, "const %s\nreturn\n", t->token) < 0)
    return dlt_error("failed to write to file");
  consume_token(t);

  // Check and consume .end token.
  if (next_token(t) <= 0) return parse_error(t, ".end");
  if (!dlt_string_equals(t->token, ".end")) return parse_error(t, ".end");
  consume_token(t);

  return 0;
}

static int macro_handler(struct tokenizer *t, FILE *out) {
  int err = 0;
  if ((err = parse_comment(t, out))) return err;
  if ((err = parse_call(t, out))) return err;
  if ((err = parse_codeword(t, out))) return err;
  if ((err = parse_var(t, out))) return err;
  if ((err = parse_const(t, out))) return err;

  // Pipe the token to the output file if nothing matches.
  if (is_token_consumed(t)) return 0;
  
  if (fputs(t->token, out) == EOF) return dlt_error("failed to write to file");
  if (fputs("\n", out) == EOF) return dlt_error("failed to write to file");
  consume_token(t);
  
  return 0;
}

static int read_label_handler(struct tokenizer *t, FILE *out) {
  // Suppress unused parameter errors.
  (void)out;

  static unsigned int address = 0;
  
  int err = 0;
  char *token = t->token;
  if (token[0] != ':') ++address;
  else err = append_label(token + 1, address);
  
  consume_token(t);
  return err;
}

static int resolve_label_handler(struct tokenizer *t, FILE *out) {
  static unsigned int address = 0;

  char *token = t->token;
  if (token[0] == ':') {
    fprintf(out, "( %s @ %d )\n", token, address);
  } else if (token[0] == '@' && strnlen(token, LABEL_MAX) > 1) {
    char *name = token + 1;
    const struct label *const l = find_label(name);
    if (l == NULL)
      return dlt_errorf("line %d: Label '%s' does not exist", t->line_number, name);

    if(fprintf(out, "%d\n", l->address) < 0)
      return dlt_error("failed to write to file");
    ++address;
  } else {
    // Pipe the token to the output file if nothing matches.
    if (fputs(token, out) == EOF) return dlt_error("failed to write to file");
    if (fputs("\n", out) == EOF) return dlt_error("failed to write to file");
    ++address;
  }

  consume_token(t);
  return 0;
}

static int opcode_handler(struct tokenizer *t, FILE *out) {
  int err = 0;
  if ((err = parse_comment(t, out))) return err;
  if (is_token_consumed(t)) return 0;
  
  char *token = t->token;
  int opcode = EXIT;
  if (isdigit(token[0]) || token[0] == '-') {
    opcode = atoi(token);
  } else {
    opcode = name_to_opcode(token);
    if (opcode < 0)
      return dlt_errorf("line %d: '%s' is not a valid instruction",
			t->line_number, token);
  }

  if (fwrite(&opcode, sizeof(opcode), 1, out) == 0)
    return dlt_error("failed to write binary data to .dopc file");

  consume_token(t);
  return 0;
}

static int replace_extension(char *in,
			     char *out,
			     size_t out_len,
			     char *extension) {
  const size_t in_len = strnlen(in, FILENAME_MAX);
  size_t extension_len = strnlen(extension, 100);
  if ((in_len + extension_len) >= out_len)
    return dlt_error("input filename exceeds buffer capacity");

  const char* match_ptr = strstr(in, ".dasm");
  if (!match_ptr)
    return dlt_errorf("invalid input filename: '%s' - must end with '.dasm'", in);

  const int index = match_ptr - in;
  memcpy(out, in, sizeof(char) * in_len);
  memcpy(out + index, extension, sizeof(char) * ++extension_len);

  return 0;
}

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
  char dexp_filename[FILENAME_MAX] = "";
  char dins_filename[FILENAME_MAX] = "";
  char dopc_filename[FILENAME_MAX] = "";
  if (replace_extension(dasm_filename, dexp_filename, FILENAME_MAX, ".dexp"))
    dlt_panic();
  if (replace_extension(dasm_filename, dins_filename, FILENAME_MAX, ".dins"))
    dlt_panic();
  if (replace_extension(dasm_filename, dopc_filename, FILENAME_MAX, ".dopc"))
    dlt_panic();

  if (create_output_file(dasm_filename, dexp_filename, macro_handler, "w"))
    dlt_panic();
  if (create_output_file(dexp_filename, dins_filename, read_label_handler, "w"))
    dlt_panic();
  if (create_output_file(dexp_filename, dins_filename, resolve_label_handler, "w"))
    dlt_panic();
  if (create_output_file(dins_filename, dopc_filename, opcode_handler, "wb"))
    dlt_panic();

  return EXIT_SUCCESS;
}
