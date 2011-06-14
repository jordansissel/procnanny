#include "program.h"
#include "insist.h"
#include <stdlib.h>
#include <string.h> /* for strlen and friends */

/* TODO(sissel): make copies of name, args, command, and any other strings */
/* TODO(sissel): split the pn_proc (process) methods into process.c */

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

  program->processes = calloc(program->nprocs, sizeof(process_t));
  pn_prog_proc_each(program, i, process, {
    pn_proc_init(process, program, i);
  });

  program->is_running = 0;
}

void pn_prog_destroy(program_t *program, int options) {
  /* TODO(sissel): kill all processes
   * wait until they are all dead
   * then free any resources (program->processes, etc) 
   */
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
      //insist_return(program->args[option_len - 1] == NULL, PN_OPTION_BAD_VALUE,
                    //"PROGRAM_ARGS array must have last element as NULL. Got: '%s'",
                    //program->args[option_len - 1]);
      break;
    case PROGRAM_NUMPROCS:
      check_size(option_len, int, "PROGRAM_NUMPROCS");
      insist_return(!pn_prog_running(program), PN_OPTION_BAD_VALUE,
                    "This program (%s) is running. To change the number of "
                    "instances, stop the program first.", program->name);
      program->nprocs = *(int *)option_value;
      program->processes = calloc(program->nprocs, sizeof(process_t));
      pn_prog_proc_each(program, i, process, {
        pn_proc_init(process, program, i);
      });
      break;
    case PROGRAM_UID:
      check_size(option_len, uid_t, "PROGRAM_UID");
      program->uid = *(uid_t *)option_value;
      break;
    case PROGRAM_USER:
      pn_util_uid((char *)option_value, &program->uid);
      break;
    case PROGRAM_GID:
      check_size(option_len, gid_t, "PROGRAM_GID");
      program->uid = *(uid_t *)option_value;
      break;
    case PROGRAM_GROUP:
      pn_util_gid((char *)option_value, &program->gid);
      break;
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

  return PN_OK;
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
    case PROGRAM_GID:
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

  return PN_OK;
} /* int pn_prog_get */

int pn_prog_start(program_t *program) {
  program->is_running = PN_TRUE;

  /* TODO(sissel): This should use pn_prog_proc_each */
  pn_prog_proc_each(program, i,  process, {
    pn_proc_start(process);
  });

  return PN_OK;
}

int pn_prog_wait(program_t *program) {
  int i = 0;
  for (i = 0; i < program->nprocs; i++) {
    pn_prog_proc_wait(program, i);
  } /* for each process */

  return PN_OK;
} /* int pn_prog_wait */

int pn_prog_proc_wait(program_t *program, int instance) {
  insist_return(pn_prog_running(program), PN_BAD_REQUEST,
                "This program (%s) is not running. Nothing to wait on.",
                program->name);
  insist_return(program->nprocs > instance, PN_BAD_REQUEST,
                "There are only %d instances, you asked for %d.",
                program->nprocs, instance);

  process_t *process = &program->processes[instance];
  return pn_proc_wait(process);
} /* int pn_prog_proc_wait */

void pn_prog_print(FILE *fp, program_t *program) {
  fprintf(fp, "Program: %s (%d instance%s)\n", program->name, program->nprocs,
          program->nprocs == 1 ? "" : "s");
  
  pn_prog_proc_each(program, i, process, {
    pn_proc_print(fp, process, i, 2);
  });
} /* int pn_prog_print */

int pn_prog_proc_running(program_t *program, int instance) {
  process_t *process = &program->processes[instance];

  return pn_proc_running(process);
}

int pn_proc_running(process_t *process) {
  return process->state != PROCESS_STATE_EXITED \
         && process->state != PROCESS_STATE_BACKOFF;
}

pid_t pn_prog_proc_pid(program_t *program, int instance) {
  insist_return(pn_prog_running(program), PN_BAD_REQUEST,
                "This program (%s) is not running. No pid to get.",
                program->name);
  insist_return(program->nprocs > instance, PN_BAD_REQUEST,
                "There are only %d instances, you asked for %d.",
                program->nprocs, instance);

  return pn_proc_pid(&program->processes[instance]);
} /* int pn_prog_proc_pid */

/* pn_prog_proc_each  is defined in program.h */

int pn_prog_running(program_t *program) {
  return program->is_running;
}

int pn_prog_signal(program_t *program, int signal) {
  pn_prog_proc_each(program, i, process, {
    pn_proc_signal(process, signal);
  });
}
