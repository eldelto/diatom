#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  int pointer;
  int data[10];
} stack;

void stack_push(stack* s, int value) {
  s->data[s->pointer++] = value;
}

int stack_pop(stack* s) {
  return s->data[--s->pointer];
}

int stack_peek(stack* s) {
  return s->data[s->pointer - 1];
}

#define WORD_NAME_LEN (10)
typedef struct dict_entry {
  struct dict_entry* next;
  char name[WORD_NAME_LEN];
  bool is_native;
  void(*code)(void);
  unsigned int words_len;
  struct dict_entry* words[];
} dict_entry;

int dict_append(dict_entry* dict, dict_entry* new_word) {
  // TODO: Check for duplicate words.
  dict_entry* current_word = dict;
  while (current_word->next != NULL)
    current_word = current_word->next;

  current_word->next = new_word;
  return 0;
}

dict_entry* dict_lookup(dict_entry* dict, const char* const name) {
  dict_entry* current_word = dict;
  while (current_word != NULL) {
    if (strncmp(current_word->name, name, WORD_NAME_LEN) == 0)
      return current_word;

    current_word = current_word->next;
  }

  return NULL;
}

stack* data_stack = &(stack) {
  .pointer = 0
};

stack* return_stack = &(stack) {
  .pointer = 0
};

unsigned int program_counter = 0;

void add(void) {
  stack_push(data_stack, stack_pop(data_stack) + stack_pop(data_stack));
}

dict_entry add_word = {
  .next = NULL,
  .name = "+",
  .code = add,
  .is_native = true,
};

void subtract(void) {
  int b = stack_pop(data_stack);
  stack_push(data_stack, stack_pop(data_stack) - b);
}

dict_entry subtract_word = {
  .next = NULL,
  .name = "-",
  .code = subtract,
  .is_native = true,
};

void peek(void) {
  printf("%d", stack_peek(data_stack));
}

dict_entry peek_word = {
  .next = NULL,
  .name = ".",
  .code = peek,
  .is_native = true,
};

void dup(void) {
  int val = stack_pop(data_stack);
  stack_push(data_stack, val);
  stack_push(data_stack, val);
}

dict_entry dup_word = {
  .next = NULL,
  .name = "dup",
  .code = dup,
  .is_native = true,
};

void _call(void) {
  stack_push(return_stack, program_counter);
  program_counter = stack_pop(data_stack);
}

void _return(void) {
  program_counter = stack_pop(return_stack);
}

// TODO: Remove
dict_entry double_word = {
  .next = NULL,
  .name = "double",
  .is_native = false,
  .words_len = 2,
  .words = {&dup_word, &add_word},
};

dict_entry* dict = &add_word;

// TODO: Handle the return stack
void execute_word(dict_entry* word) {
  if (word->is_native) {
    word->code();
    return;
  }

  for (unsigned int i = 0; i < word->words_len; ++i) {
    execute_word(word->words[i]);
  }
}

int handle_token(const char* const token) {
  char* remainder;
  int literal = strtol(token, &remainder, 10);
  if (*remainder == '\0') {
    stack_push(data_stack, literal);
    return 0;
  }

  dict_entry* word = dict_lookup(dict, token);
  if (word == NULL) {
    printf("Word '%s' not found.\n", token);
    return -1;
  }

  execute_word(word);
  return 0;
}

#define INPUT_BUFFER_LEN (100)
int main(void) {
  dict_append(dict, &subtract_word);
  dict_append(dict, &peek_word);
  dict_append(dict, &dup_word);
  dict_append(dict, &double_word);

  char input_buffer[INPUT_BUFFER_LEN];
  char* token;

  while (true) {
    printf(">");
    if (fgets(input_buffer, INPUT_BUFFER_LEN, stdin) == NULL) {
      puts("Error while reading user input");
      return -1;
    }
    if (input_buffer[0] == '\n') {
      continue;
    }

    input_buffer[strcspn(input_buffer, "\n")] = '\0';

    token = strtok(input_buffer, " ");
    do {
      if (handle_token(token) < 0) break;
    } while ((token = strtok(NULL, " ")) != NULL);

    puts(" ok");
  }

  return 0;
}
