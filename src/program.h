#ifndef _PN_PROGRAM_H_
#define _PN_PROGRAM_H_

/** TODO(sissel): Maybe track 'health' states? */

#include <unistd.h> /* for pid_t */
#include <sys/time.h> /* for struct timespec */
#include <time.h> /* for struct timespec */
#include <sys/resource.h> /* for struct rlimit */
#include <stdio.h> /* for FILE */

struct program;
typedef struct program program_t;

/** 
 * State combinations
 *
 * UP + STARTING - just started, not declared healthy yet.
 * UP + RUNNING - normal happy state
 * UP + STOPPING - stopping for requested termination
 * UP + EXITED - program died, will restart
 * UP + BACKOFF - flapping, delayed restart.
 * DOWN + STARTING - should immediately go to 'stopping'
 * DOWN + RUNNING - should immediately go to 'stopping'
 * DOWN + STOPPING - shutting down
 * DOWN + EXITED - normal happy state
 * DOWN + BACKOFF - should immediately go to 'exited' state
 */

typedef enum {
  ADMIN_STATE_UP = 1, /** want program to be 'up' */
  ADMIN_STATE_DOWN = 2, /** want program to be 'down' */
} admin_state;

typedef enum {
  PROCESS_STATE_STARTING = 1,
  PROCESS_STATE_RUNNING = 2,
  PROCESS_STATE_STOPPING = 3,
  PROCESS_STATE_EXITED = 4,
  PROCESS_STATE_BACKOFF = 5,
  PROCESS_STATE_NEW = 6,
} process_state;

typedef enum {
  PROGRAM_NAME = 1,
  PROGRAM_COMMAND = 2,
  PROGRAM_ARGS = 3,
  PROGRAM_NUMPROCS = 4,
  PROGRAM_UID = 5,
  PROGRAM_USER = 6,
  PROGRAM_GID = 7,
  PROGRAM_GROUP = 8,
  PROGRAM_NICE = 9,
  PROGRAM_IONICE = 10,
  PROGRAM_DATA = 11,

  /* resource limits */
  PROGRAM_LIMIT_FILES = 20,
  //PROGRAM_LIMIT_CORE = 21,
  //PROGRAM_LIMIT_PROCESSES = 22,
} program_options;

enum pn_return_codes {
  PN_OK = 0,
  PN_FALSE = 0,
  PN_TRUE = 1,
  PN_OPTION_INVALID = 2,
  PN_OPTION_BAD_VALUE = 3,
  PN_BAD_REQUEST = 4,
  PN_PROCESS_NOT_RUNNING = 5,
  PN_PROGRAM_NOT_RUNNING = 6,
};


#include "process.h"

struct process;
struct program;
struct program {
  char *name; /** the name of the program */
  size_t name_len;
  char *command; /** the command to run */
  size_t command_len;
  char **args; /** the arguments to the command */
  size_t args_len; /** the arguments to the command */

  struct ulimit *limits; /** array of resource limits to use with setrlimit */
  uid_t uid; /** the uid to run this program as */
  gid_t gid; /** the gid to run this progrma as */
  int nice; /** the nice level */
  int ionice; /** the ionice level, requires linux and kernel >= 2.6.13 */

  /* The length of the 'processes' array is the 'nprocs' value */
  struct process *processes; /** Array of process instances */
  int nprocs; /** number of instances to run */

  /* XXX We probably don't need this. */
  int is_running; /** is this program active? */

  /** The delay that defines the time required to spend in 'starting' before
   * transition to 'running'. A value of '0' here means no minimum and the
   * process will start immediately in the 'running' state. */
  double minimum_run_duration;

  /** This setting controls how many flaps are permitted before the state
   * changes to BACKOFF.
   */
  int flap_max;

  void (*state_cb)(struct process *process, process_state from, process_state to);

  /* arbitrary data you can assign to this process */
  void *data;
}; /* struct program */

struct ulimit {
  struct rlimit rlimit;
  int resource;
}; /* struct ulimit */

void pn_prog_init(program_t *program);
program_t *pn_prog_new(void);

/**
 * Set an option on this program.
 */
int pn_prog_set(program_t *program, int option_name, const void *option_value,
                 size_t option_len);
int pn_prog_get(program_t *program, int option_name, void *option_value,
                 size_t *option_len);

int pn_prog_start(program_t *program);
void pn_prog_print(FILE *fp, program_t *program);
int pn_prog_running(program_t *program);

int pn_prog_wait(program_t *program);
int pn_prog_proc_wait(program_t *program, int instance);
int pn_prog_proc_running(program_t *program, int instance);
int pn_prog_signal(program_t *program, int signal);

/** Iterate over processes for this program.
 *
 * @param program - a pointer to a program_t
 * @param varname - the variable name to hold the 'process_t' for each
 *   iteration
 * @param block - a block of code.
 */
#define pn_prog_proc_each(program, iname, varname, block) \
{ \
  int iname = 0; \
  for (iname = 0; iname < (program)->nprocs; iname++) { \
    process_t *varname = &(program)->processes[iname]; \
    { \
      block \
    } \
  } \
}

#endif /* _PN_PROGRAM_H_ */
