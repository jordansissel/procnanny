struct program {
  char *name; /** the name of the program */
  char *command; /** the command to run */
  char **args; /** the arguments to the command */
  int instances; /** number of instances to run */
  struct ulimit *limits; /** array of resource limits to use with setrlimit */
  uid_t uid; /** the uid to run this program as */
  gid_t gid; /** the gid to run this progrma as */
  int nice; /** the nice level */
  int ionice; /** the ionice level, requires linux and kernel >= 2.6.13 */

  /* TODO(sissel): These should be arrays, for each instance of the program */
  pid_t pid; /** the pid of the process */
  int admin_state; /** the administrative state, up or down, etc */
  int state; /** current running state */
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


struct ulimit {
  struct rlimit rlimit;
  int resource;
};
