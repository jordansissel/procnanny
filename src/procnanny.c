#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ev.h>
#include <zmq.h>
#include <zmq_utils.h>
#include "program.h"
#include "process.h"
#include "procnanny.h"
#include "pn_api.h"
#include "insist.h"

#include <msgpack.h>
#include "msgpack_helpers.h"

struct proc_io_channel {
  ev_io **child_io;
};

void start(program_t *program);
void api_cb(EV_P_ ev_io *watcher, int revents);
void child_proc_cb(EV_P_ ev_child *watcher, int revents);
void new_child_timer(process_t *process, double delay);
void child_timer_cb(EV_P_ ev_timer *watcher, int revents);
void child_io_cb(EV_P_ ev_io *watcher, int revents);
void pn_proc_watch_io(struct ev_loop *loop, process_t *process);
void publish_output(process_t *process, struct proc_io_channel *iochan,
                    char *data, ssize_t length);
void publish_proc_event(process_t *process, const char *name);

void child_timer_cb(EV_P_ ev_timer *watcher, int revents) {
  ev_timer_stop(EV_A_ watcher);
  process_t *process = watcher->data;
  procnanny_t *pn = pn_proc_program(process)->data;

  /* TODO(sissel): publish event about this state change
   * we are starting the process */

  /* Restart it. */
  pn_proc_start(process);
  ev_child *newwatcher = calloc(1, sizeof(ev_child));
  ev_child_init(newwatcher, child_proc_cb, pn_proc_pid(process), 0);
  ev_child_start(pn->loop, newwatcher);
  newwatcher->data = process;
  pn_proc_watch_io(pn->loop, process);
  publish_proc_event(process, "started");

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
  struct proc_io_channel *iochan = process->data;

  /* TODO(sissel): formalize protocol for reads from a process.
   * Should include: program name, process instance, data
   */
  while ((bytes = read(watcher->fd, buf, 4096)) > 0) {
    publish_output(process, iochan, buf, bytes);
  }

  if (bytes == 0) {
    /* EOF */
    ev_io_stop(EV_A_ watcher);
    close(watcher->fd);
    free(watcher);
  }
} /* child_io_cb */

void publish_output(process_t *process, struct proc_io_channel *iochan,
                    char *data, ssize_t length) {
  zmq_msg_t event;
  int rc;
  size_t msgsize;
  program_t *program = pn_proc_program(process);
  procnanny_t *pn = program->data;

  fprintf(stdout, "%s[%d]: (%d bytes) %.*s\n", pn_proc_program(process)->name,
          pn_proc_instance(process), length, length, data);

  /* Fields:
   *  - data (the string read)
   *  - program name
   *  - process instance
   *  - stdout or stderr
   */

  msgpack_sbuffer *buffer = msgpack_sbuffer_new();
  msgpack_packer *output_msg = msgpack_packer_new(buffer, msgpack_sbuffer_write);

  msgpack_pack_map(output_msg, 4);

  msgpack_pack_string(output_msg, "event", -1);
  msgpack_pack_string(output_msg, "data", -1);

  msgpack_pack_string(output_msg, "program", -1);
  msgpack_pack_string(output_msg, program->name, program->name_len);

  msgpack_pack_string(output_msg, "instance", -1);
  msgpack_pack_int(output_msg, process->instance);

  msgpack_pack_string(output_msg, "data", -1);
  msgpack_pack_string(output_msg, data, length);

  zmq_msg_init_data(&event, buffer->data, buffer->size, free_msgpack_buffer, buffer); 
  zmq_send(pn->eventsocket, &event, 0);
  zmq_msg_close(&event);

  msgpack_packer_free(output_msg);
} /* publish_output */

void publish_proc_event(process_t *process, const char *name) {
  zmq_msg_t event;
  int rc;
  size_t msgsize;
  program_t *program = pn_proc_program(process);
  procnanny_t *pn = program->data;

  /* Fields:
   *  - program name
   *  - process instance
   *  - exit status
   *  - duration ?
   */

  msgpack_sbuffer *buffer = msgpack_sbuffer_new();
  msgpack_packer *output_msg = msgpack_packer_new(buffer, msgpack_sbuffer_write);

  msgpack_pack_map(output_msg, 5);
  msgpack_pack_string(output_msg, "event", -1);
  msgpack_pack_string(output_msg, name, -1);

  msgpack_pack_string(output_msg, "program", -1);
  msgpack_pack_string(output_msg, program->name, program->name_len);

  msgpack_pack_string(output_msg, "instance", -1);
  msgpack_pack_int(output_msg, process->instance);

  msgpack_pack_string(output_msg, "state", -1);
  switch (process->state) {
    case PROCESS_STATE_STARTING: msgpack_pack_string(output_msg, "starting", -1); break;
    case PROCESS_STATE_RUNNING: msgpack_pack_string(output_msg, "running", -1); break;
    case PROCESS_STATE_STOPPING: msgpack_pack_string(output_msg, "stopping", -1); break;
    case PROCESS_STATE_EXITED: msgpack_pack_string(output_msg, "exited", -1); break;
    case PROCESS_STATE_BACKOFF: msgpack_pack_string(output_msg, "backoff", -1); break;
    case PROCESS_STATE_NEW: msgpack_pack_string(output_msg, "new", -1); break;
    default: msgpack_pack_string(output_msg, "unknown", -1); break;
  }

  if (process->exit_signal == 0) {
    msgpack_pack_string(output_msg, "code", -1);
    msgpack_pack_int(output_msg, process->exit_status);
  } else {
    msgpack_pack_string(output_msg, "signal", -1);

    switch (process->exit_signal) {
      case SIGHUP: msgpack_pack_string(output_msg, "HUP", 3); break;
      case SIGINT: msgpack_pack_string(output_msg, "INT", 3); break;
      case SIGQUIT: msgpack_pack_string(output_msg, "QUIT", 4); break;
      case SIGILL: msgpack_pack_string(output_msg, "ILL", 3); break;
      case SIGTRAP: msgpack_pack_string(output_msg, "TRAP", 4); break;
      case SIGABRT: msgpack_pack_string(output_msg, "ABRT", 4); break;
      case SIGBUS: msgpack_pack_string(output_msg, "BUS", 3); break;
      case SIGFPE: msgpack_pack_string(output_msg, "FPE", 3); break;
      case SIGKILL: msgpack_pack_string(output_msg, "KILL", 4); break;
      case SIGUSR1: msgpack_pack_string(output_msg, "USR1", 4); break;
      case SIGSEGV: msgpack_pack_string(output_msg, "SEGV", 4); break;
      case SIGUSR2: msgpack_pack_string(output_msg, "USR2", 4); break;
      case SIGPIPE: msgpack_pack_string(output_msg, "PIPE", 4); break;
      case SIGALRM: msgpack_pack_string(output_msg, "ALRM", 4); break;
      case SIGTERM: msgpack_pack_string(output_msg, "TERM", 4); break;
      case SIGSTKFLT: msgpack_pack_string(output_msg, "STKFLT", 6); break;
      case SIGCHLD: msgpack_pack_string(output_msg, "CHLD", 4); break;
      case SIGCONT: msgpack_pack_string(output_msg, "CONT", 4); break;
      case SIGSTOP: msgpack_pack_string(output_msg, "STOP", 4); break;
      case SIGTSTP: msgpack_pack_string(output_msg, "TSTP", 4); break;
      case SIGTTIN: msgpack_pack_string(output_msg, "TTIN", 4); break;
      case SIGTTOU: msgpack_pack_string(output_msg, "TTOU", 4); break;
      case SIGURG: msgpack_pack_string(output_msg, "URG", 3); break;
      case SIGXCPU: msgpack_pack_string(output_msg, "XCPU", 4); break;
      case SIGXFSZ: msgpack_pack_string(output_msg, "XFSZ", 4); break;
      case SIGVTALRM: msgpack_pack_string(output_msg, "VTALRM", 5); break;
      case SIGPROF: msgpack_pack_string(output_msg, "PROF", 4); break;
      case SIGWINCH: msgpack_pack_string(output_msg, "WINCH", 5); break;
      case SIGPOLL: msgpack_pack_string(output_msg, "POLL", 4); break;
      case SIGPWR: msgpack_pack_string(output_msg, "PWR", 3); break;
      case SIGSYS: msgpack_pack_string(output_msg, "SYS", 3); break;
      default: msgpack_pack_string(output_msg, "unknown", -1); break;
    } /* switch process->exit_signal */
  } /* if process was killed by a signal */

  zmq_msg_init_data(&event, buffer->data, buffer->size, free_msgpack_buffer, buffer); 
  zmq_send(pn->eventsocket, &event, 0);
  zmq_msg_close(&event);

  msgpack_packer_free(output_msg);
} /* publish_output */

void restart_child(process_t *process, double delay) {
  ev_timer *timer;
  timer = calloc(1, sizeof(ev_timer));
  procnanny_t *pn = pn_proc_program(process)->data;
  struct proc_io_channel *iochan = process->data;

  timer->data = process;
  ev_timer_init(timer, child_timer_cb, delay, delay);
  ev_timer_start(pn->loop, timer);
} /* restart_child */

void child_proc_cb(EV_P_ ev_child *watcher, int revents) {
  ev_child_stop(EV_A_ watcher);
  process_t *process = watcher->data;
  struct proc_io_channel *iochan = process->data;

  pn_proc_exited(process, watcher->rstatus);
  printf("process %s[%d] - ", pn_proc_program(process)->name, 
         pn_proc_pid(process));
  pn_proc_print(stdout, process, 0, 0);

  /* TODO(sissel): emit event indicating that this process exited */
  publish_proc_event(process, "exited");

  restart_child(process, 2.0);
  /* We allocate a new watcher for each pid, so this one is done. */
  free(watcher);
} /* child_proc_cb */

int main() {
  program_t *program;
  struct ev_loop *loop = EV_DEFAULT;

  procnanny_t pn;
  pn.loop = loop;
  pn.programs_len = 1;
  pn.programs = calloc(pn.programs_len, sizeof(program_t));;

  program = &pn.programs[0];
  program->data = &pn;
  pn.zmq = zmq_init(1);
  start(program);
  start_api(&pn);

  pn.eventsocket = zmq_socket(pn.zmq, ZMQ_PUB);
  int rc;
  const char *endpoint = "tcp://*:3344";
  insist(pn.eventsocket != NULL, "zmq_socket returned NULL. Unexpected (%x).",
         pn.eventsocket);
  rc = zmq_bind(pn.eventsocket, endpoint);
  insist(rc == 0, "zmq_bind(\"%s\") returned %d (I wanted 0).", endpoint, rc);

  ev_child *pidwatcher;
  pn_prog_proc_each(program, i, process, {
    pidwatcher = calloc(1, sizeof(ev_child));
    ev_child_init(pidwatcher, child_proc_cb, pn_proc_pid(process), 0);
    ev_child_start(loop, pidwatcher);
    pidwatcher->data = process;
    /* No need to free pidwatcher; we'll free it when the child dies in the
     * callback */

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
  args[2] = "sleep(rand * 3); puts Time.now;puts 'hello world'; exit((rand * 10).to_i)";
  //args[0] = "curl";
  //args[1] = "http://www.google.com/";

  program->nice = 5;

  pn_prog_set(program, PROGRAM_COMMAND, command, strlen(command));
  pn_prog_set(program, PROGRAM_ARGS, args, 3);
  pn_prog_start(program);
} /* start */

void pn_proc_watch_io(struct ev_loop *loop, process_t *process) {
  struct proc_io_channel *iochan = calloc(1, sizeof(struct proc_io_channel));;
  iochan->child_io = calloc(3, sizeof(ev_io*));
  iochan->child_io[0] = calloc(1, sizeof(ev_io));
  iochan->child_io[1] = calloc(1, sizeof(ev_io));
  iochan->child_io[2] = calloc(1, sizeof(ev_io));

  /* TODO(sissel): attach watchers to the stdout/stderr of the new processes */
  /* child_io[0] is unused, could be stdin, but not needed yet. */
  ev_io_init(iochan->child_io[1], child_io_cb, process->stdout, EV_READ); /* stdout */
  iochan->child_io[1]->data = process;
  ev_io_start(loop, iochan->child_io[1]);

  ev_io_init(iochan->child_io[2], child_io_cb, process->stderr, EV_READ); /* stderr */
  iochan->child_io[2]->data = process;
  ev_io_start(loop, iochan->child_io[2]);

  process->data = iochan;
}
