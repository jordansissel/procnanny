#ifndef _PN_PROGRAM_H_
#define _PN_PROGRAM_H_

/** TODO(sissel): Maybe track 'health' states? */

#include <unistd.h> /* for pid_t */
#include <sys/time.h> /* for struct timespec */
#include <time.h> /* for struct timespec */
#include <sys/resource.h> /* for struct rlimit */
#include <stdio.h> /* for FILE */

struct process {
  pid_t pid; /** the pid of the process */
  int admin_state; /** the administrative state, up or down, etc */
  int state; /** current running state */

  int exit_status;
  struct timespec start_time;
  struct timespec end_time;
};

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

  int is_running; /** is this program active? */
};

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

enum admin_states {
  ADMIN_STATE_UP = 1, /** want program to be 'up' */
  ADMIN_STATE_DOWN = 2, /** want program to be 'down' */
};

enum process_states {
  PROCESS_STATE_STARTING = 1,
  PROCESS_STATE_RUNNING = 2,
  PROCESS_STATE_STOPPING = 3,
  PROCESS_STATE_EXITED = 4,
  PROCESS_STATE_BACKOFF = 5,
};

enum program_options {
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

  /* resource limits */
  PROGRAM_LIMIT_FILES = 20,
  //PROGRAM_LIMIT_CORE = 21,
  //PROGRAM_LIMIT_PROCESSES = 22,
};

enum pn_error_codes {
  PN_OPTION_INVALID = 2,
  PN_OPTION_BAD_VALUE = 3,
};

struct ulimit {
  struct rlimit rlimit;
  int resource;
};

typedef struct program program_t;

void pn_prog_init(program_t *program);

/**
 * Set an option on this program.
 */
int pn_prog_set(program_t *program, int option_name, const void *option_value,
                 size_t option_len);
int pn_prog_get(program_t *program, int option_name, void *option_value,
                 size_t *option_len);

int pn_prog_start(program_t *program);
int pn_prog_print(FILE *fp, program_t *program);

#endif /* _PN_PROGRAM_H_ */
