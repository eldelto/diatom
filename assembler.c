#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "diatom.h"
#include "util.h"

#define IDENTIFIER_MAX 100
#define EXTENSION_MAX 6
#define LABELS_MAX 100

struct label {
  char name[IDENTIFIER_MAX];
  unsigned int address;
};

struct label labels[LABELS_MAX] = {
  (struct label) {
    .name = "",
    .address = 0,
  }
};

size_t label_offset = 0;
static void append_label(char name[IDENTIFIER_MAX], unsigned int address) {
  struct label l = {
    .name = "",
    .address = address,
  };
  strlcpy(l.name, name + 1, IDENTIFIER_MAX);
  labels[label_offset] = l;

  if (label_offset == (LABELS_MAX - 1)) dlt_fatal_error("maximum number of labels");
  ++label_offset;
}

static const struct label * find_label(const char *const name) {
  for (size_t i = 0; i < label_offset; ++i) {
    const struct label *const label = &labels[i];
    if (strcmp(name, label->name) == 0) {
      return label;
    }
  }

  return NULL;
}

static int replace_extension(
			     char *input_filename, 
			     char output_filename[IDENTIFIER_MAX],
			     char output_extension[EXTENSION_MAX]
			     ) {
  const size_t input_len = strnlen(input_filename, IDENTIFIER_MAX);
  if (input_len >= IDENTIFIER_MAX)
    return dlt_error("input filename exceeds max length");

  const char* match_ptr = strstr(input_filename, ".dasm");
  if (!match_ptr)
    return dlt_error("invalid input filename. Must end with '.dasm'");

  const int index = match_ptr - input_filename;
  memcpy(output_filename, input_filename, sizeof(char) * input_len);

  memcpy(output_filename+index, output_extension, sizeof(char) * EXTENSION_MAX);

  return 0;
}

static void trim_string(char* string){
  if (string == NULL || strnlen(string, IDENTIFIER_MAX) == 0) return;
  
  const char* start = string;
  while(isspace(*start) || *start == '\n' || *start == '\r') ++start;
  
  size_t len = strnlen(start, IDENTIFIER_MAX);
  memmove(string, start, len + 1);
  
  char* end = string + len - 1;
  while(end >= string  && (isspace(*end) || *end == '\n' || *end == '\r')) --end ;
  
  *++end = '\0';
}

typedef int (*line_handler)(
			    FILE *output_file, 
			    char instruction[IDENTIFIER_MAX], 
			    const unsigned int line_number);

static int translate_file(FILE *input_file, FILE *output_file, line_handler handler) {
  int err = 0;

  char *line = NULL;
  size_t line_len = 0;
  char instruction[IDENTIFIER_MAX];

  unsigned int line_number = 0;
  while(getline(&line, &line_len, input_file) != -1) {
    ++line_number;

    trim_string(line);
    if (line_len > IDENTIFIER_MAX) {
      char err_msg[100] = "";
      snprintf(err_msg, 100, "line %d: Identifier exceeds max length", line_number);
      err = dlt_error(err_msg);
      
      break;
    }
    strlcpy(instruction, line, sizeof(instruction) - 1);

    if((err = handler(output_file, instruction, line_number))) break;
  }

  free(line);

  return err;
}

static int label_handler(FILE *output_file, 
			 char instruction[IDENTIFIER_MAX], 
			 const unsigned int line_number) {

  // Start at 2 because we have to jump to the _start label.
  static unsigned int address = 2;

  switch (instruction[0]) {
    // Empty line
  case '\0': break;
    // Comment
  case '#': break;
    // Label creation
  case ':': {
    append_label(instruction, address);
    fprintf(output_file, "# %s @ %d\n", instruction, address);
    break;
  }
    // Jump to label
  case '!': {
    const struct label* const l = find_label(instruction + 1);
    if (l == NULL) {
      char err_msg[100] = "";
      snprintf(err_msg, 100, "line %d: Label '%s' does not exist", line_number, instruction);
      dlt_error(err_msg);
      return -1;
    }

    fputs("const\n", output_file);
    fprintf(output_file, "%d\n", l->address);
    fputs("jump\n", output_file);
    address += 3;
    break;
  }
  case '@': {
    const struct label* const l = find_label(instruction + 1);
    if (l == NULL) {
      char err_msg[100] = "";
      snprintf(err_msg, 100, "line %d: Label '%s' does not exist", line_number, instruction);
      dlt_error(err_msg);
      return -1;
    }

    fprintf(output_file, "%d\n", l->address);
    ++address;
    break;
  }
  default: {
    if (isdigit(instruction[0]) || instruction[0] == '-') {
      int number = atoi(instruction);
      fputs("const\n", output_file);
      fprintf(output_file, "%d\n", number);
      address += 2;
    } else {
      if (name_to_opcode(instruction) < 0) {
	char err_msg[100] = "";
	snprintf(err_msg, 100, "line %d: '%s' is not a valid instruction", line_number, instruction);
	dlt_error(err_msg);
	return -1;
      }

      fprintf(output_file, "%s\n", instruction);
      ++address;
    }
  }
  }

  return 0;
}

static int opcode_handler(FILE *output_file, 
			  char instruction[IDENTIFIER_MAX], 
			  const unsigned int line_number) {

  if (line_number == 1) {
    const struct label* const l = find_label("_start");
    if (l == NULL) return dlt_error("no '_start' label found");

    const int opcode = JUMPN;
    if (fwrite(&opcode, sizeof(opcode), 1, output_file) == 0) return dlt_error("failed to write initial jumpn to .dopc file");
    if (fwrite(&l->address, sizeof(l->address), 1, output_file) == 0) return dlt_error("failed to write '_start' address to .dopc file");
  }

  if (instruction[0] == '#') return 0;

  // Constant or VM instruction
  int opcode = EXIT;
  if (isdigit(instruction[0]) || instruction[0] == '-') {
    opcode = atoi(instruction);
  } else {
    opcode = name_to_opcode(instruction);
    if (opcode < 0) {
      char err_msg[100] = "";
      snprintf(err_msg, 100, "line %d: '%s' is not a valid instruction", line_number, instruction);
      return dlt_error(err_msg);
    }
  }

  if (fwrite(&opcode, sizeof(opcode), 1, output_file) == 0) return dlt_error("failed to write binary data to .dopc file");

  return 0;
}

static int create_output_file(char *input_filename, char output_filename[IDENTIFIER_MAX], line_handler handler, char *write_mode) {
  int err = 0;

  FILE* input_file = fopen(input_filename, "r");
  if (input_file == NULL) {
    return dlt_error("failed to open input file");
  }

  FILE* output_file = fopen(output_filename, write_mode);
  if (output_file == NULL) {
    err = dlt_error("failed to open input file");
    goto close_input_file;
  }

  err = translate_file(input_file, output_file, handler);
  if (err) {
    goto close_files;
  }

 close_files:
  fclose(output_file);
 close_input_file:
  fclose(input_file);

  return err;
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
  char dins_filename[IDENTIFIER_MAX] = "";
  char dopc_filename[IDENTIFIER_MAX] = "";
  if (replace_extension(dasm_filename, dins_filename, ".dins")) dlt_panic();
  if (replace_extension(dasm_filename, dopc_filename, ".dopc")) dlt_panic();

  if (create_output_file(dasm_filename, dins_filename, label_handler, "w")) dlt_panic();
  if (create_output_file(dins_filename, dopc_filename, opcode_handler, "wb")) dlt_panic();

  return EXIT_SUCCESS;
}
