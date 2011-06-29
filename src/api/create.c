#include "api.h"
#include "insist.h"

/* from procnanny.c */
extern void start_process(procnanny_t *pn, process_t *process);
extern void pn_proc_state_cb(process_t *process, process_state from,
                             process_state to);

void api_request_create(rpc_io *rpcio, msgpack_object *request_obj,
                        msgpack_packer *response_msg) {
  procnanny_t *pn = rpcio->procnanny;
  program_t *program = pn_prog_new();
  char *program_name = NULL;
  size_t program_len = 0;
  char *command = NULL;
  size_t command_len = 0;
  msgpack_object *args_obj;
  char **args = NULL;
  size_t args_len = 0;
  msgpack_object_kv *limits;
  size_t limits_len;

  int rc;
  int i = 0;

  /* TODO(sissel): Break these into separate methods */

  /* program name */
  obj_get_or_fail(request_obj, "program", MSGPACK_OBJECT_RAW, &program_name,
                  &program_len, response_msg)
  pn_prog_set(program, PROGRAM_NAME,
              strndup(program_name, program_len), program_len);


  /* command */
  obj_get_or_fail(request_obj, "command", MSGPACK_OBJECT_RAW, &command,
                  &command_len, response_msg)
  pn_prog_set(program, PROGRAM_COMMAND,
              strndup(command, command_len), command_len);

  /* args */
  obj_get(request_obj, "args", MSGPACK_OBJECT_ARRAY, &args_obj, &args_len);
  args = malloc(args_len);
  for (i = 0; i < args_len; i++) {
    msgpack_object *element = args_obj + i;
    const char *arg;
    size_t arglen;
    arg = element->via.raw.ptr;
    arglen = element->via.raw.size;
    args[i] = strndup(arg, arglen);
  }
  pn_prog_set(program, PROGRAM_ARGS, args, args_len);

  /* ulimits */
#if 0
  obj_get(request_obj, "limits", MSGPACK_OBJECT_MAP, &limits, &limits_len);
  for (i = 0; i < limits_len; i++) {
    msgpack_object *key = &(limits + i)->key;
    msgpack_object *value = &(limits + i)->val;

    if (!strncmp("max files", key->via.raw.ptr, key->via.raw.size)) {
      switch (value->type) {
        case MSGPACK_OBJECT_POSITIVE_INTEGER:
          pn_prog_set(program, PROGRAM_LIMIT_FILES,
                      &value->via.u64, sizeof(uint64_t));
          break;
        case MSGPACK_OBJECT_NEGATIVE_INTEGER:
          pn_prog_set(program, PROGRAM_LIMIT_FILES,
                      &value->via.i64, sizeof(int64_t));
          break;
        default:
          api_respond_error(request_obj, response_msg, 
                            "'limits/max files' must be a number");
          return;
      } /* switch value->type */
    } else if (!strncmp("...", key->via.raw.ptr, key->via.raw.size)) {
      /* ... */
    } else {
      api_respond_error(request_obj, response_msg, "Unknown field in 'limits'");
      return;
    }
  } /* for each limit */
#endif

  /* Other parameters:
   * name
   * command
   * args (array of strings)
   * ulimits (??)
   * user
   * group
   * nice
   * ionice
   * instance count
   * minimum_run_duration 
   * flap_max
   */
  
  program->data = pn;
  //pn->programs_len += 1;
  //pn->programs = realloc(pn->programs, pn->programs_len * sizeof(program_t));
  //pn->programs[pn->programs_len - 1] = program;
  //memcpy(pn->programs + pn->programs_len - 1, program, sizeof(program_t));

  program->state_cb = pn_proc_state_cb;
  pn_prog_proc_each(program, i, process, {
    printf("starting instance %d\n", i);
    pn_proc_move_state(process, PROCESS_STATE_NEW);
    start_process(pn, process);
  });

  msgpack_pack_string(response_msg, "created", -1);
} /* api_request_create */
