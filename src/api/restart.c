#include "api.h"

void api_request_restart(rpc_io *rpcio, msgpack_object *request_obj,
                       msgpack_packer *response_msg) {
  int rc;
  char *program_name;
  size_t program_len;
  rc = obj_get(request_obj, "program", MSGPACK_OBJECT_RAW, &program_name, &program_len);
  if (rc != 0) {
    msgpack_pack_map(response_msg, 2);
    msgpack_pack_string(response_msg, "error", -1);
    msgpack_pack_string(response_msg, "Message had no 'program' field", -1);
    msgpack_pack_string(response_msg, "request", -1);
    msgpack_pack_object(response_msg, *request_obj);
    return;
  }

  int64_t instance = -1;
  rc = obj_get(request_obj, "instance", MSGPACK_OBJECT_NEGATIVE_INTEGER, &instance, NULL);
  /* A missing 'instance' field is assumed ot mean "all" instances */

  printf("Request to restart '%.*s' instance '%ld'\n", (int)program_len,
         program_name, instance);

  msgpack_pack_map(response_msg, 1);
  msgpack_pack_string(response_msg, "signal-results", 14);

  pn_prog_each(rpcio->procnanny, i, program, {
    if (!strncmp(program->name, program_name, program_len)) {
      msgpack_pack_map(response_msg, 1);
      printf("Program name: %.*s\n", program_len, program_name);
      msgpack_pack_string(response_msg, program_name, program_len);
      msgpack_pack_map(response_msg, instance == -1 ? program->nprocs : instance);
      if (instance >= 0) {
        printf("instance >= 0 not supported yet: %ld \n", instance);
      } else {
        pn_prog_proc_each(program, i, process, {
          rc = pn_proc_signal(process, SIGTERM);
          printf("kill[%d] => %d\n", i, rc);
          msgpack_pack_int(response_msg, i);

          if (rc == PN_OK) {
            msgpack_pack_true(response_msg);
          } else {
            msgpack_pack_string(response_msg, "process wasn't running", 22);
          }
        })
      }
    }
  });
}

