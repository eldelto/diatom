#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define IDENTIFIER_MAX 100
#define LABELS_MAX 100

static void panic(const char* const message) {
  printf("\033[31m%s\033[0m\n", message);
  exit(EXIT_FAILURE);
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

static const struct label* find_label(
  const char* const name,
  const struct label* const labels,
  const size_t label_size
) {
  for (size_t i = 0; i < label_size; ++i) {
    const struct label* const label = &labels[i];
    if (strcmp(name, label->name) == 0) {
      return label;
    }
  }

  return NULL;
}

static void generate_output_filename(char* input_filename, char output_filename[IDENTIFIER_MAX]) {
  if (strnlen(input_filename, IDENTIFIER_MAX) == IDENTIFIER_MAX) panic("Input filename exceeds max length - aborting.");

  const char* match_ptr = strstr(input_filename, ".dasm");
  if (!match_ptr) panic("Invalid input filename. Must end with '.dasm' - aborting.");

  const int index = match_ptr - input_filename;
  printf("Index: %i", index);
  memcpy(input_filename, output_filename, index - 1);
  memcpy(".dbc", output_filename, sizeof(".dbc"));
  // TODO: Fix
}

static void usage() {
  puts("Usage: dasm [input-file]\n");
  puts("Flags:");
  puts("  -h - Displays this usage message.");
}

int main(int argc, char* argv[]) {
  int ch = 0;
  while ((ch = getopt(argc, argv, "hc:")) != -1) {
    switch (ch) {
    case 'h':
      usage();
      exit(EXIT_SUCCESS);
    }
  }

  if (argc != 2) {
    usage();
    panic("Invalid arguments - aborting.");
  }

  char* input_filename = argv[1];
  char output_filename[IDENTIFIER_MAX] = "";
  generate_output_filename(input_filename, output_filename);
  // const char* output_filename = 

  puts(output_filename);

  return 0;

  const int input_file = open(input_filename, O_RDONLY);
  if (input_file < 0) panic("Failed to open input file - aborting.");

  struct stat file_stats;
  int err = fstat(input_file, &file_stats);
  if (err) {
    close(input_file);
    panic("Failed to read file stats from input file - aborting.");
  }

  const char* mapped = mmap(NULL, file_stats.st_size, PROT_READ, MAP_PRIVATE, input_file, 0);
  if (mapped == MAP_FAILED) {
    close(input_file);
    panic("Failed to mmap input file - aborting.");
  }

  struct instruction inst = {
    .name = "",
    .size = 0,
  };

  struct label labels[LABELS_MAX] = {
    (struct label) {
      .name = "",
      .address = 0,
    }
  };
  unsigned int label_size = 0;
  unsigned int address = 0;

  for (unsigned int i = 0; i < file_stats.st_size; ++i) {
    const char c = mapped[i];

    if (c == ' ' || c == '\t') continue;
    if (c == '\n' || c == '\r') {
      switch (inst.name[0]) {
      case ':': {
        struct label l = {
          .name = "",
          .address = address,
        };
        strlcpy(l.name, inst.name + 1, IDENTIFIER_MAX);
        labels[label_size] = l;
        ++label_size;
        break;
      }
      case '@': {
        const struct label* const l = find_label(inst.name + 1, labels, label_size);
        if (l == NULL) {
          char err_msg[100] = "";
          snprintf(err_msg, 100, "Error on line %d: Label '%s' does not exist.", i, inst.name);
          panic(err_msg);
        }

        printf("%d\n", l->address);
        ++address;
        break;
      }
      case '!': {
        const struct label* const l = find_label(inst.name + 1, labels, label_size);
        if (l == NULL) {
          char err_msg[100] = "";
          snprintf(err_msg, 100, "Error on line %d: Label '%s' does not exist.", i, inst.name);
          panic(err_msg);
        }

        puts("const");
        printf("%d\n", l->address);
        puts("next");
        address += 3;
        break;
      }
      default: {
        // TODO: Push int or resolve enum to int
        puts(inst.name);
        ++address;
        break;
      }
      }

      inst_reset(&inst);
    }
    else {
      inst_append(&inst, c);
    }
  }

  munmap((void*)mapped, file_stats.st_size);
  close(input_file);

  return EXIT_SUCCESS;
}
