#include "process.h"
#include "program.h"

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

  //printf("command: %s\n", program->command);
  //printf("args: %s %s %s\n", program->args[0], program->args[1], program->args[2]);

  process->pid = fork();
  if (process->pid == 0) {
    char **args = calloc(program->args_len + 2, sizeof(char *));
    int ret = 0;
    args[0] = program->command;
    memcpy(args + 1, program->args, program->args_len * sizeof(char *));
    args[program->args_len + 1] = NULL;

    /* TODO(sissel): setrlimit
     * TODO(sissel): close fds
     * TODO(sissel): set nice
     * TODO(sissel): set ionice
     * TODO(sissel): setuid
     * TODO(sissel): setgid
     */
    ret = execvp(program->command, args);
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
