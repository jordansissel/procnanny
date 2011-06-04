#include "program.h"
#include "process.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ev.h>

void start(program_t *program);

void child_cb(EV_P_ ev_child *watcher, int revents) {
  ev_child_stop(EV_A_ watcher);
  process_t *process = watcher->data;

  pn_proc_exited(process, watcher->rstatus);
  printf("process %s[%d] - ", pn_proc_program(process)->name, 
         pn_proc_pid(process));
  pn_proc_print(stdout, process, 0, 0);

  /* Restart it. */
  pn_proc_start(process);
  ev_child *newwatcher = calloc(1, sizeof(ev_child));
  ev_child_init(newwatcher, child_cb, pn_proc_pid(process), 0);

  /* TODO(sissel): this should be a specific loop. */
  ev_child_start(pn_proc_program(process)->data, newwatcher);
  newwatcher->data = process;

  /* We allocate a new watcher for each pid, and this one is done. */
  free(watcher);
}

int main() {
  program_t program;
  struct ev_loop *loop = EV_DEFAULT;

  ev_child *pidwatcher;

  program.data = loop;

  start(&program);

  pn_prog_proc_each(&program, i, process, {
    pidwatcher = calloc(1, sizeof(ev_child));
    ev_child_init(pidwatcher, child_cb, pn_proc_pid(process), 0);
    ev_child_start(loop, pidwatcher);
    pidwatcher->data = process;
  });

  pn_prog_print(stdout, &program);

  ev_run(loop, 0);

  return 0;
}

void start(program_t *program) {
  pn_prog_init(program);

  int nprocs = 1;
  pn_prog_set(program, PROGRAM_NAME, "hello world", sizeof(char *));
  pn_prog_set(program, PROGRAM_NUMPROCS, &nprocs, sizeof(int));

  const char *command = "/bin/bash";
  const char **args = calloc(3, sizeof(char *));

  args[0] = "-c";
  args[1] = "sleep $((3 + $RANDOM % 5)); echo 'Hello world'; [ $(($RANDOM % 3)) -eq 0 ] && kill -INT $$; exit $(($RANDOM % 10))";
  args[2] = NULL;

  pn_prog_set(program, PROGRAM_COMMAND, command, strlen(command));
  pn_prog_set(program, PROGRAM_ARGS, args, 3);

  pn_prog_start(program);
}
