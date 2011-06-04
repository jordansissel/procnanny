#include "program.h"
#include "insist.h"
#include <string.h> /* for strlen and friends */

#define check_size(size, type, name) \
  insist_return(size == sizeof(type), PN_OPTION_BAD_VALUE, \
                name "is a " __STRING(type) " (size=%zd), got size %zd", \
                sizeof(type), size)

void pn_prog_init(program_t *program) {
  program->name = NULL;
  program->name_len = 0;
  program->command = NULL;
  program->command_len = 0;
  program->args = NULL;
  program->args_len = 0;
  program->nprocs = 1;
  program->limits = NULL;
  program->uid = -1;
  program->gid = -1;

  program->nice = 0;
  program->ionice = 0;

  program->processes = calloc(program->nprocs, sizeof(struct process));
}

int pn_prog_set(program_t *program, int option_name, const void *option_value,
                size_t option_len) {

  /* TODO(sissel): Allow folks to do:
   *   pn_prog_set(prog, name, (void *)123, -1)
   *   and make '123' work mostlyproperly.
   * TODO(sissel): Should make sure this works well in all cases.
   * We're skirting around type length checking of the compiler here,
   * so this might not be really smart.
   */
  //if (option_len == -1) {
    //option_len = sizeof(int);
    //option_value = &option_value;
  //}

  switch (option_name) {
    case PROGRAM_NAME:
      program->name = (char *) option_value;
      program->name_len = option_len;
      break;
    case PROGRAM_COMMAND:
      program->command = (char *) option_value;
      program->command_len = option_len;
      break;
    case PROGRAM_ARGS:
      program->args = (char **) option_value;
      program->args_len = option_len;
      insist_return(program->args[option_len - 1] == NULL, PN_OPTION_BAD_VALUE,
                    "PROGRAM_ARGS array must have last element as NULL. Got: '%s'",
                    program->args[option_len - 1]);
      break;
    case PROGRAM_NUMPROCS:
      check_size(option_len, int, "PROGRAM_NUMPROCS");
      insist_return(!program->is_running, PN_OPTION_BAD_VALUE,
                    "This program (%s) is running. To change the number of "
                    "instances, stop the program first.", program->name);
      program->nprocs = *(int *)option_value; program->processes = calloc(program->nprocs, sizeof(struct process));
      break;
    case PROGRAM_UID:
      check_size(option_len, uid_t, "PROGRAM_UID");
      program->uid = *(uid_t *)option_value;
      break;
    case PROGRAM_USER: break;
    case PROGRAM_GID: break;
      check_size(option_len, gid_t, "PROGRAM_GID");
      program->uid = *(uid_t *)option_value;
      break;
    case PROGRAM_GROUP: break;
    case PROGRAM_NICE:
      check_size(option_len, int, "PROGRAM_NICE");
      program->nice = *(int *)option_value;
      break;
    case PROGRAM_IONICE:
      check_size(option_len, int, "PROGRAM_IONICE");
      program->nice = *(int *)option_value;
      break;
    case PROGRAM_LIMIT_FILES:
      check_size(option_len, int, "PROGRAM_LIMIT_FILES");
      /* TODO(sissel): Not supported yet. */
      fprintf(stderr, "PROGRAM_LIMIT_FILES not supported yet\n");
      break;
    default:
      return PN_OPTION_INVALID;
  } /* switch (option_name) */
} /* int pn_prog_set */

int pn_prog_get(program_t *program, int option_name, void *option_value,
                size_t *option_len) {

  switch (option_name) {
    case PROGRAM_NAME:
      *(char **)option_value = program->name;
      *option_len = program->name_len;
      break;
    case PROGRAM_COMMAND:
      *(char **)option_value = program->command;
      *option_len = program->command_len;
      break;
    case PROGRAM_ARGS:
      *(char ***)option_value = program->args;
      *option_len = program->args_len;
      /* how many args? */
      break;
    case PROGRAM_NUMPROCS:
      *(int *)option_value = program->nprocs;
      *option_len = sizeof(program->nprocs);
      break;
    case PROGRAM_UID:
      *(uid_t *)option_value = program->uid;
      *option_len = sizeof(program->uid);
      break;
    case PROGRAM_USER:
      fprintf(stderr, "PROGRAM_USER not supported yet\n");
      break;
    case PROGRAM_GID: break;
      *(gid_t *)option_value = program->gid;
      *option_len = sizeof(program->gid);
      break;
    case PROGRAM_GROUP:
      fprintf(stderr, "PROGRAM_GROUP not supported yet\n");
      return PN_OPTION_INVALID;
      break;
    case PROGRAM_NICE:
      *(int *)option_value = program->nice;
      *option_len = sizeof(program->nice);
      break;
    case PROGRAM_IONICE:
      *(int *)option_value = program->ionice;
      *option_len = sizeof(program->ionice);
      break;
    case PROGRAM_LIMIT_FILES:
      fprintf(stderr, "PROGRAM_LIMIT_FILES not supported yet\n");
      return PN_OPTION_INVALID;
      break;
    default:
      return PN_OPTION_INVALID;
  } /* switch (option_name) */
} /* int pn_prog_get */

int pn_prog_start(program_t *program) {
  int i = 0;
  for (i = 0; i < program->nprocs; i++) {
    _pn_prog_spawn(program, i);
  }
}

int _pn_prog_spawn(program_t *program, int instance) {
  struct process *process;
  process = &program->processes[instance];

  /* set start clock */
  clock_gettime(CLOCK_REALTIME, &process->start_time);
  process->state = PROCESS_STATE_STARTING;

  process->pid = fork();
  if (process->pid == 0) {
    char **args = calloc(program->args_len + 2, sizeof(char *));
    int ret = 0;
    args[0] = program->command;
    memcpy(args + 1, program->args, program->args_len * sizeof(char *));
    args[program->args_len + 2] = NULL;
    ret = execvp(program->command, args);
    /* if we get here, something went wrong.
     * Maybe send a message that execvp failed using zeromq? */
    exit(255);
  }
}

int pn_prog_print(FILE *fp, program_t *program) {
  int i = 0;
  printf("Program: %s (%d instance%s)\n", program->name, program->nprocs,
         program->nprocs == 1 ? "" : "s");
  
  for (i = 0; i < program->nprocs; i++) {
    struct process *process = &program->processes[i];
    printf("  %d: pid:%d exitcode:%d\n", i, process->pid, process->exit_status);
  }
}
