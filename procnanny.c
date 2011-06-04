#include "program.h"
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>

int main() {
  program_t program;
  pn_prog_init(&program);

  int val = 3;
  pn_prog_set(&program, PROGRAM_NAME, "Hello world", sizeof(char *));
  pn_prog_set(&program, PROGRAM_NICE, &val, sizeof(int));

  const char *command = "/bin/sh";
  const char *args[] = { "-c", "echo hello world", NULL };

  pn_prog_set(&program, PROGRAM_COMMAND, command, strlen(command));
  pn_prog_set(&program, PROGRAM_ARGS, args, 3);

  pn_prog_start(&program);

  int status;
  pn_prog_print(stdout, &program);
  waitpid(0, &status, 0);

  return 0;
}
