#ifndef _PROCESS_H_
#define _PROCESS_H_
//#include "pn-objects.h"

struct process;
typedef struct process process_t;

#include "program.h"

struct program;

struct process {
  pid_t pid; /** the pid of the process */
  int admin_state; /** the administrative state, up or down, etc */
  int state; /** current running state */

  int exit_status;
  int exit_signal;
  struct timespec start_time;
  struct timespec end_time;

  /* pointer to parent program */
  struct program *program;
  /* the instance (program->processes[instance] etc) */
  int instance;

  /* arbitrary data you can assign to this process */
  void *data;
};

/**
 * Pretty-print the process. 
 */
void pn_proc_print(FILE *fp, process_t *process, int procnum, int indent);

/** This is a way for external tools, like libev, to be used
 * and still notify a program of a pid exit.
 */
void pn_proc_exited(process_t *process, int status);

/**
 * Start the process
 */
int pn_proc_start(process_t *process);

/**
 * Get the pid of this process
 */

pid_t pn_proc_pid(process_t *process);

/**
 * Wait for this process to exit
 */
int pn_proc_wait(process_t *process);

/**
 * Is this process running?
 */
int pn_proc_running(process_t *process);

/**
 * initialize this process
 */
int pn_proc_init(process_t *process, program_t *program, int instance);

/**
 * Get the program_t that owns this process.
 */
program_t *pn_proc_program(process_t *process);

#endif /* _PROCESS_H_ */
