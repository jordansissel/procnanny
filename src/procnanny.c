#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ev.h>

#include "program.h"
#include "process.h"
#include "procnanny.h"
#include "pn_api.h"

void start(program_t *program);
void api_cb(EV_P_ ev_io *watcher, int revents);
void child_cb(EV_P_ ev_child *watcher, int revents);
void new_child_timer(process_t *process, double delay);
void child_timer_cb(EV_P_ ev_timer *watcher, int revents);

void child_timer_cb(EV_P_ ev_timer *watcher, int revents) {
  ev_timer_stop(EV_A_ watcher);
  process_t *process = watcher->data;

  /* Restart it. */
  pn_proc_start(process);
  ev_child *newwatcher = calloc(1, sizeof(ev_child));
  ev_child_init(newwatcher, child_cb, pn_proc_pid(process), 0);
  ev_child_start(pn_proc_program(process)->data, newwatcher);
  newwatcher->data = process;

  free(watcher); /* was created by new_child_timer() */
} /* child_timer_cb */

void restart_child(process_t *process, double delay) {
  ev_timer *timer;
  timer = calloc(1, sizeof(ev_timer));

  timer->data = process;
  ev_timer_init(timer, child_timer_cb, delay, delay);
  ev_timer_start(pn_proc_program(process)->data, timer);
} /* restart_child */

void child_cb(EV_P_ ev_child *watcher, int revents) {
  ev_child_stop(EV_A_ watcher);
  process_t *process = watcher->data;

  pn_proc_exited(process, watcher->rstatus);
  printf("process %s[%d] - ", pn_proc_program(process)->name, 
         pn_proc_pid(process));
  pn_proc_print(stdout, process, 0, 0);

  restart_child(process, 2.0);
  /* We allocate a new watcher for each pid, and this one is done. */
  free(watcher);
} /* child_cb */

int main() {
  program_t *program;
  struct ev_loop *loop = EV_DEFAULT;

  procnanny_t pn;
  pn.loop = loop;
  pn.programs_len = 1;
  pn.programs = calloc(pn.programs_len, sizeof(program_t));;

  program = &pn.programs[0];
  program->data = loop;
  start(program);
  printf("loop: %x\n", pn.loop);
  start_api(&pn);

  ev_child *pidwatcher;
  pn_prog_proc_each(program, i, process, {
    pidwatcher = calloc(1, sizeof(ev_child));
    ev_child_init(pidwatcher, child_cb, pn_proc_pid(process), 0);
    ev_child_start(loop, pidwatcher);
    pidwatcher->data = process;
  });

  pn_prog_print(stdout, &pn.programs[0]);

  ev_run(loop, 0);

  return 0;
} /* main */

void start(program_t *program) {
  pn_prog_init(program);

  int nprocs = 3;
  pn_prog_set(program, PROGRAM_NAME, "hello world", 11);
  pn_prog_set(program, PROGRAM_USER, "jls", -1);
  pn_prog_set(program, PROGRAM_NUMPROCS, &nprocs, sizeof(int));

  //const char *command = "/bin/bash";
  const char *command = "/usr/bin/env";
  const char **args = calloc(3, sizeof(char *));

  args[0] = "ruby";
  args[1] = "-e";
  args[2] = "sleep(rand * 10); puts :OK";

  program->nice = 5;

  pn_prog_set(program, PROGRAM_COMMAND, command, strlen(command));
  pn_prog_set(program, PROGRAM_ARGS, args, 3);
  pn_prog_start(program);
} /* start */

