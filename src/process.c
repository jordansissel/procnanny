#include "process.h"
#include "program.h"
#include "insist.h"

#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h> /* for waitpid, WIFSIGNALED, etc */
#include <unistd.h>
#include <fcntl.h>

int pn_proc_init(process_t *process, program_t *program, int instance) {
  memset(process, 0, sizeof(process_t));
  process->program = program;
  process->instance = instance;
  process->start_count = 0;
  process->flap_count = 0;
  process->data = NULL;
  return PN_OK;
} /* int pn_proc_init */

void pn_proc_move_state(process_t *process, process_state state) {
#define TRANSITION(from, to) (from << 16 | to)
  program_t *program = pn_proc_program(process);
  process_state old_state = process->state;

  switch (TRANSITION(old_state, state)) {
    case TRANSITION(PROCESS_STATE_STARTING, PROCESS_STATE_RUNNING):
      /* Healthy startup, reset flap count */
      process->flap_count = 0;
      break;
    case TRANSITION(PROCESS_STATE_STARTING, PROCESS_STATE_EXITED):
      process->flap_count++;
      if (process->flap_count > program->flap_max) {
        pn_proc_move_state(process, PROCESS_STATE_BACKOFF);
      }
      break;
  }

  if (state == PROCESS_STATE_STARTING) {
    process->start_count++;
  }

  process->state = state;

  if (program->state_cb != NULL) {
    program->state_cb(process, old_state, state);
  }
} /* pn_proc_move_state */

int pn_proc_start(process_t *process) {
  const program_t *program;
  int rc;
  program = pn_proc_program(process);

  /* set start clock */
  clock_gettime(CLOCK_REALTIME, &process->start_time);
  pn_proc_move_state(process, PROCESS_STATE_STARTING);

  printf("Starting '%.*s'[%d]: %s ...\n", (int)program->name_len, program->name,
         process->instance, program->command);
  //printf("command: %s\n", program->command);
  //printf("args: %s %s %s\n", program->args[0], program->args[1], program->args[2]);

  int pipe_stdin[2];
  int pipe_stdout[2];
  int pipe_stderr[2];

  /* TODO(sissel): error handling */
  pipe(pipe_stdin); 
  pipe(pipe_stdout);
  pipe(pipe_stderr);

  process->stdin = pipe_stdin[1];
  process->stdout = pipe_stdout[0];
  process->stderr = pipe_stderr[0];

  process->pid = fork();
  if (process->pid == 0) {
    char **args = calloc(program->args_len + 2, sizeof(char *));
    int i = 0;
    args[0] = program->command;

    close(pipe_stdin[1]);
    close(pipe_stdout[0]);
    close(pipe_stderr[0]);
    dup2(pipe_stdin[0], 0); /* redirect stdin */
    dup2(pipe_stdout[1], 1); /* redirect stdout */
    dup2(pipe_stdout[2], 2); /* redirect stderr */

    for (i = 3; i < 1024; i++) {
      close(i);
    }

    /* TODO(sissel): Replace things in the args:
     *   %i - instance number
     *   %p - pid
     *   %u - uid
     *   %g - gid
     *
     * Anything else?
     */
    memcpy(args + 1, program->args, program->args_len * sizeof(char *));
    args[program->args_len + 1] = NULL;

    /* TODO(sissel): setrlimit
     * TODO(sissel): close fds ?
     * TODO(sissel): set ionice
     */

    /* setgid if necessary */
    if (program->gid != getgid()) {
      /* setgid requested */
      rc = setgid(program->gid);
      insist(rc == 0, "setgid(%ld) failed. errno: %d: %s", program->gid,
             errno, strerror(errno));
    }

    /* setuid if necessary */
    if (program->uid != getuid()) {
      /* setuid requested */
      rc = setuid(program->uid);
      insist(rc == 0, "setuid(%ld) failed. errno: %d: %s", program->uid,
             errno, strerror(errno));
    }

    /* set nice if necessary */
    if (program->nice != 0) {
      rc = nice(program->nice);
      insist(rc != -1, "nice(%d) failed. errno: %d: %s", program->nice, errno,
             strerror(errno));
    }

    rc = execvp(program->command, args);
    /* if we get here, something went wrong.
     * Maybe send a message that execvp failed using zeromq? */
    exit(255);
  }

  /* Close the child parts of the pipes */
  close(pipe_stdin[0]);
  close(pipe_stdout[1]);
  close(pipe_stderr[1]);

  close(pipe_stdin[1]); /* We don't care about stdin anyway */

  /* set nonblock */
  fcntl(process->stdout, F_SETFL, O_NONBLOCK);
  fcntl(process->stderr, F_SETFL, O_NONBLOCK);

  return PN_OK;
} /* int pn_proc_start */

int pn_proc_wait(process_t *process) {
  if (!pn_proc_running(process)) {
    return PN_OK;
  }

  int status = 0;
  int rc = 0;
  rc = waitpid(pn_proc_pid(process), &status, 0);
  if (rc >= 0) {
    pn_proc_exited(process, status);
  } /* if rc >= 0 */
  return PN_OK;
} /* pn_proc_wait */

void pn_proc_print(FILE *fp, process_t *process, int procnum, int indent) {
  static const char spaces[] = "                                        ";
  if (process->exit_signal) {
    fprintf(fp, "%.*s%d: pid:%d exitsignal:%d\n", indent, spaces, procnum,
            process->pid, process->exit_signal);
  } else {
    fprintf(fp, "%.*s%d: pid:%d exitcode:%d\n", indent, spaces,  procnum,
            process->pid, process->exit_status);
  }
} /* void pn_proc_print */

pid_t pn_proc_pid(process_t *process) {
  return process->pid;
} /* pid_t pn_proc_pid */

int pn_proc_instance(process_t *process) {
  return process->instance;
} /* pid_t pn_proc_pid */

program_t *pn_proc_program(process_t *process) {
  return process->program;
} /* program_t *pn_proc_program */

void pn_proc_exited(process_t *process, int status) { 
  if (WIFSIGNALED(status)) {
    process->exit_status = -1;
    process->exit_signal = WTERMSIG(status);
  } else {
    process->exit_status = WEXITSTATUS(status);
    process->exit_signal = 0;
  }
  pn_proc_move_state(process, PROCESS_STATE_EXITED);
} /* void pn_proc_exited */

int pn_proc_signal(process_t *process, int signal) {
  if (process->pid > 0) {
    int rc = kill(process->pid, signal);
    if (rc == 0) {
      return PN_OK;
    }
    perror("KILL FAILED");
    return PN_PROCESS_NOT_RUNNING;
  }

  return PN_PROCESS_NOT_RUNNING;
}

int pn_proc_running(process_t *process) {
  return process->state != PROCESS_STATE_EXITED \
         && process->state != PROCESS_STATE_BACKOFF;
} /* int pn_proc_running */

