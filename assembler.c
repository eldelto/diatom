#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define INSTRUCTION_MAX 20
#define LABELS_MAX 20

static void panic(const char* const message) {
  printf("\033[31m%s\033[0m\n", message);
  exit(-1);
}

struct instruction {
  char name[INSTRUCTION_MAX];
  unsigned int size;
};

static void inst_append(struct instruction* inst, char c) {
  if (inst->size >= (INSTRUCTION_MAX - 1)) return;

  inst->name[inst->size] = c;
  inst->size++;
  inst->name[inst->size] = '\0';
}

static void inst_reset(struct instruction* inst) {
  inst->name[0] = '\0';
  inst->size = 0;
}

struct label {
  char name[INSTRUCTION_MAX];
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

int main(void) {
  const int input_file = open("diatom.dasm", O_RDONLY);
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
        strlcpy(l.name, inst.name + 1, INSTRUCTION_MAX);
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

  return 0;
}
