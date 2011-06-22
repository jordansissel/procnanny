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
void child_proc_cb(EV_P_ ev_child *watcher, int revents);
void new_child_timer(process_t *process, double delay);
void child_timer_cb(EV_P_ ev_timer *watcher, int revents);
void child_io_cb(EV_P_ ev_io *watcher, int revents);
void pn_proc_watch_io(struct ev_loop *loop, process_t *process);

void child_timer_cb(EV_P_ ev_timer *watcher, int revents) {
  ev_timer_stop(EV_A_ watcher);
  process_t *process = watcher->data;

  /* Restart it. */
  pn_proc_start(process);
  ev_child *newwatcher = calloc(1, sizeof(ev_child));
  ev_child_init(newwatcher, child_proc_cb, pn_proc_pid(process), 0);
  ev_child_start(pn_proc_program(process)->data, newwatcher);
  newwatcher->data = process;

  free(watcher); /* was created by new_child_timer() */
} /* child_timer_cb */

void child_io_cb(EV_P_ ev_io *watcher, int revents) {
  /* TODO(sissel): Make structure to carry IO for each process.
   * Should include:
   * stdout + stderr ev_io's
   * small buffer for carrying read data
   */
  char buf[4096];
  process_t *process = (process_t *)watcher->data;
  ssize_t bytes;

  while ((bytes = read(watcher->fd, buf, 4096)) > 0) {
    fprintf(stdout, "%s[%d]: (%d bytes) %.*s\n", pn_proc_program(process)->name,
            pn_proc_instance(process), bytes, bytes, buf);
  }

  if (bytes == 0) {
    /* EOF */
    ev_io_stop(EV_A_ watcher);
    close(watcher->fd);
    free(watcher);
  }
}

void restart_child(process_t *process, double delay) {
  ev_timer *timer;
  timer = calloc(1, sizeof(ev_timer));
  struct ev_loop *loop = pn_proc_program(process)->data;

  timer->data = process;
  ev_timer_init(timer, child_timer_cb, delay, delay);
  ev_timer_start(loop, timer);
  pn_proc_watch_io(loop, process);
} /* restart_child */

void child_proc_cb(EV_P_ ev_child *watcher, int revents) {
  ev_child_stop(EV_A_ watcher);
  process_t *process = watcher->data;

  pn_proc_exited(process, watcher->rstatus);
  printf("process %s[%d] - ", pn_proc_program(process)->name, 
         pn_proc_pid(process));
  pn_proc_print(stdout, process, 0, 0);

  restart_child(process, 2.0);
  /* We allocate a new watcher for each pid, and this one is done. */
  free(watcher);
  //free(process->data); /* the IO watchers for this process */
} /* child_proc_cb */

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
    ev_child_init(pidwatcher, child_proc_cb, pn_proc_pid(process), 0);
    ev_child_start(loop, pidwatcher);
    pidwatcher->data = process;
    //process->data = child_io;
    /* No need to free pidwatcher; we'll free it when the child dies in the clalback */

    pn_proc_watch_io(loop, process);
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
  args[2] = "sleep(rand * 3); puts 'hello world!'";

  program->nice = 5;

  pn_prog_set(program, PROGRAM_COMMAND, command, strlen(command));
  pn_prog_set(program, PROGRAM_ARGS, args, 3);
  pn_prog_start(program);
} /* start */

void pn_proc_watch_io(struct ev_loop *loop, process_t *process) {
  ev_io **child_io;
  child_io = calloc(3, sizeof(ev_io*));
  child_io[0] = calloc(1, sizeof(ev_io));
  child_io[1] = calloc(1, sizeof(ev_io));
  child_io[2] = calloc(1, sizeof(ev_io));

  /* TODO(sissel): attach watchers to the stdout/stderr of the new processes */
  /* child_io[0] is unused, could be stdin, but not needed yet. */
  ev_io_init(child_io[1], child_io_cb, process->stdout, EV_READ); /* stdout */
  child_io[1]->data = process;
  ev_io_start(loop, child_io[1]);

  ev_io_init(child_io[2], child_io_cb, process->stderr, EV_READ); /* stderr */
  child_io[2]->data = process;
  ev_io_start(loop, child_io[2]);
}
