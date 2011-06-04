#include "program.h"
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>

int main() {
  program_t program;
  pn_prog_init(&program);

  int val;
  pn_prog_set(&program, PROGRAM_NAME, "Hello world", sizeof(char *));

  val = 3;
  pn_prog_set(&program, PROGRAM_NICE, &val, sizeof(int));

  val = 4;
  pn_prog_set(&program, PROGRAM_NUMPROCS, &val, sizeof(int));

  const char *command = "/bin/bash";
  const char *args[] = { "-c", "sleep $((3 + $RANDOM % 5)); echo 'Hello world'; [ $(($RANDOM % 3)) -eq 0 ] && kill -INT $$; exit $(($RANDOM % 10))", NULL };

  pn_prog_set(&program, PROGRAM_COMMAND, command, strlen(command));
  pn_prog_set(&program, PROGRAM_ARGS, args, 3);

  pn_prog_start(&program);

  int status;

  pn_prog_wait(&program);
  pn_prog_print(stdout, &program);

  return 0;
}
