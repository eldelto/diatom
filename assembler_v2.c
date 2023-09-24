#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "diatom.h"
#include "util.h"

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


  return EXIT_SUCCESS;
}
