#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "diatom.h"

#define IDENTIFIER_MAX 100
#define LABELS_MAX 100

char error_msg[100] = "";
static int error(char msg[100]) {
  memcpy(error_msg, msg, sizeof(error_msg));
  return -1;
}

static void panic() {
  printf("\033[31mError: %s\033[0m\n", error_msg);
  exit(EXIT_FAILURE);
}

static void fatal_error(char msg[100]) {
  error(msg);
  panic();
}

struct instruction {
  char name[IDENTIFIER_MAX];
  unsigned int size;
};

static void inst_append(struct instruction* inst, char c) {
  if (inst->size >= (IDENTIFIER_MAX - 1)) return;

  inst->name[inst->size] = c;
  inst->size++;
  inst->name[inst->size] = '\0';
}

static void inst_reset(struct instruction* inst) {
  inst->name[0] = '\0';
  inst->size = 0;
}

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
  memcpy(l.name, name + 1, IDENTIFIER_MAX);
  labels[label_offset] = l;

  if (label_offset == (LABELS_MAX - 1)) fatal_error("maximum number of labels");
  ++label_offset;
}

static const struct label* find_label(const char* const name) {
  for (size_t i = 0; i < label_offset; ++i) {
    const struct label* const label = &labels[i];
    if (strcmp(name, label->name) == 0) {
      return label;
    }
  }

  return NULL;
}

static int generate_output_filename(
  char* input_filename, 
  char* output_filename,
  size_t len
) {
  const size_t input_len = strnlen(input_filename, len);
  if (input_len >= len)
    return error("input filename exceeds max length");

  const char* match_ptr = strstr(input_filename, ".dasm");
  if (!match_ptr)
    return error("invalid input filename. Must end with '.dasm'");

  const int index = match_ptr - input_filename;
  memcpy(output_filename, input_filename, sizeof(char) * input_len);

  char extension[6] = ".dins";
  memcpy(output_filename+index, extension, sizeof(extension));

  return 0;
}

static void usage() {
  puts("Usage: dasm [input-file]\n");
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
    fatal_error("invalid arguments");
  }

  char* input_filename = argv[1];
  char output_filename[IDENTIFIER_MAX] = "";
  int err = generate_output_filename(input_filename, output_filename, IDENTIFIER_MAX);
  if (err) panic();

  const int input_file = open(input_filename, O_RDONLY);
  if (input_file < 0) fatal_error("failed to open input file");

  struct stat file_stats;
  err = fstat(input_file, &file_stats);
  if (err) {
    error("failed to read file stats from input file");
    goto close_input;
  }

  const char* mapped = mmap(NULL, file_stats.st_size, PROT_READ, MAP_PRIVATE, input_file, 0);
  if (mapped == MAP_FAILED) {
    error("failed to mmap input file");
    goto close_input;
  }

  FILE* output_file = fopen(output_filename, "w");
  if (input_file < 0) {
    error("failed to open output file");
    goto unmap_input;
  }

  struct instruction inst = {
    .name = "",
    .size = 0,
  };

  unsigned int address = 0;
  unsigned int line = 0;

  for (unsigned int i = 0; i < file_stats.st_size; ++i) {
    const char c = mapped[i];

    if (c == ' ' || c == '\t') continue;
    if (c == '\n' || c == '\r') {
      ++line;

      switch (inst.name[0]) {
      // Comment
      case '#': break;
      // Label creation
      case ':': {
        append_label(inst.name, address);
        fprintf(output_file, "# %s @ %d\n", inst.name, address);
        break;
      }
      // Jump to label
      case '!': {
        const struct label* const l = find_label(inst.name + 1);
        if (l == NULL) {
          char err_msg[100] = "";
          snprintf(err_msg, 100, "line %d: Label '%s' does not exist", line, inst.name);
          error(err_msg);
          goto close_files;
        }

        fputs("const\n", output_file);
        fprintf(output_file, "%d\n", l->address);
        fputs("jump\n", output_file);
        address += 3;
        break;
      }
      // Constant or VM instruction
      default: {
        if (isdigit(inst.name[0]) || inst.name[0] == '-') {
          int number = atoi(inst.name);
          fputs("const\n", output_file);
          fprintf(output_file, "%d\n", number);
          address += 2;
        } else {
          // TODO: Validate instruction name against enum
          if (name_to_opcode(inst.name) < 0) {
            char err_msg[100] = "";
            snprintf(err_msg, 100, "line %d: '%s' is not a valid instruction", line, inst.name);
            error(err_msg);
            goto close_files;
          }

          fprintf(output_file, "inst: %s\n", inst.name);
          ++address;
        }
        break;
      }
      }

      inst_reset(&inst);
    }
    else {
      inst_append(&inst, c);
    }
  }


close_files:
  fclose(output_file);
unmap_input:
  munmap((void*)mapped, file_stats.st_size);
close_input:
  close(input_file);

  if (error_msg[0]) panic();

  return EXIT_SUCCESS;
}

// TODO: Implement the second pass to generate the actual binary file
//       or just emit it directly