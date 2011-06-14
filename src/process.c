#include "process.h"
#include "program.h"
#include "insist.h"
#include "stringhelper.h"

#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h> /* for waitpid, WIFSIGNALED, etc */

int pn_proc_init(process_t *process, program_t *program, int instance) {
  memset(process, 0, sizeof(process_t));
  process->program = program;
  process->instance = instance;
  return PN_OK;
} /* int pn_proc_init */

int pn_proc_start(process_t *process) {
  const program_t *program;
  program = pn_proc_program(process);

  /* set start clock */
  clock_gettime(CLOCK_REALTIME, &process->start_time);
  process->state = PROCESS_STATE_STARTING;

  printf("Starting '%.*s'[%d]: %s ...\n", (int)program->name_len, program->name,
         process->instance, program->command);
  //printf("command: %s\n", program->command);
  //printf("args: %s %s %s\n", program->args[0], program->args[1], program->args[2]);

  process->pid = fork();
  if (process->pid == 0) {
    char **args = calloc(program->args_len + 2, sizeof(char *));
    int rc = 0;
    int i = 0;
    args[0] = program->command;

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

program_t *pn_proc_program(process_t *process) {
  return process->program;
} /* program_t *pn_proc_program */

void pn_proc_exited(process_t *process, int status) { 
  process->state = PROCESS_STATE_EXITED;
  if (WIFSIGNALED(status)) {
    process->exit_status = -1;
    process->exit_signal = WTERMSIG(status);
  } else {
    process->exit_status = WEXITSTATUS(status);
    process->exit_signal = 0;
  }
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
