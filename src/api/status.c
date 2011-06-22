#include "api.h"

void api_request_status(rpc_io *rpcio, msgpack_object *request_obj,
                        msgpack_packer *response_msg) {
  int rc;
  char *program_name = NULL;
  size_t program_len = 0;
  rc = obj_get(request_obj, "program", MSGPACK_OBJECT_RAW, &program_name, &program_len);
  /* A missing 'program' field is OK. It means we want all programs */

  msgpack_pack_map(response_msg, 1);
  msgpack_pack_string(response_msg, "programs", -1);

  /* programs is a map of:
   *   programname => {
   *     command: "string",
   *     args: "string",
   *     uid: uid,
   *     gid: gid,
   *     nice: nice,
   *     ionice: ionice,
   *     is_running: boolean
   *     instances: {
   *       pid: ...
   *       state: ...
   *       admin_state: ...
   *       start: ...
   *       duration: ...
   *     }
   *   }
   */
  msgpack_pack_map(response_msg, rpcio->procnanny->programs_len);

  pn_prog_each(rpcio->procnanny, i, program, {
    if (program_name != NULL && strncmp(program->name, program_name, program_len)) {
      continue;
    }

    msgpack_pack_string(response_msg, program->name, program->name_len);
    msgpack_pack_map(response_msg, 8);  /* 8 fields */

    msgpack_pack_string(response_msg, "command", -1);
    msgpack_pack_string(response_msg, program->command, program->command_len);

    msgpack_pack_string(response_msg, "args", -1);
    msgpack_pack_array(response_msg, program->args_len);
    int argind = 0;
    for (argind = 0; argind < program->args_len; argind++) {
      msgpack_pack_string(response_msg, program->args[argind], -1);
    }

    msgpack_pack_string(response_msg, "uid", -1);
    msgpack_pack_uint32(response_msg, program->uid);

    msgpack_pack_string(response_msg, "gid", -1);
    msgpack_pack_uint32(response_msg, program->gid);

    msgpack_pack_string(response_msg, "nice", -1);
    msgpack_pack_int32(response_msg, program->nice);

    msgpack_pack_string(response_msg, "ionice", -1);
    msgpack_pack_int32(response_msg, program->ionice);

    msgpack_pack_string(response_msg, "active", -1);
    msgpack_pack_true(response_msg);

    msgpack_pack_string(response_msg, "instances", -1);
    msgpack_pack_map(response_msg, program->nprocs);

    pn_prog_proc_each(program, instance, process, {
      msgpack_pack_uint32(response_msg, instance);
      msgpack_pack_map(response_msg, 5);

      msgpack_pack_string(response_msg, "pid", -1);
      msgpack_pack_uint32(response_msg, process->pid);

      msgpack_pack_string(response_msg, "state", -1);
      switch (process->state) {
        case PROCESS_STATE_STARTING:
          msgpack_pack_string(response_msg, "starting", -1); break;
        case PROCESS_STATE_RUNNING:
          msgpack_pack_string(response_msg, "running", -1); break;
        case PROCESS_STATE_STOPPING:
          msgpack_pack_string(response_msg, "stopping", -1); break;
        case PROCESS_STATE_EXITED:
          msgpack_pack_string(response_msg, "exited", -1); break;
        case PROCESS_STATE_BACKOFF:
          msgpack_pack_string(response_msg, "backoff", -1); break;
        case PROCESS_STATE_NEW:
          msgpack_pack_string(response_msg, "new", -1); break;
        default:
          msgpack_pack_string(response_msg, "unknown", -1); break;
      }

      msgpack_pack_string(response_msg, "exitcode", -1);
      msgpack_pack_uint8(response_msg, process->exit_status);

      msgpack_pack_string(response_msg, "exitsignal", -1);
      msgpack_pack_uint8(response_msg, process->exit_signal);

      msgpack_pack_string(response_msg, "admin_state", -1);
      switch (process->admin_state) {
        case ADMIN_STATE_DOWN:
          msgpack_pack_string(response_msg, "down", -1); break;
        case ADMIN_STATE_UP:
          msgpack_pack_string(response_msg, "up", -1); break;
        default:
          msgpack_pack_string(response_msg, "unknown", -1); break;
      }
    })
  });
}
