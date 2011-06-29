#include "api.h"

void api_request_restart(rpc_io *rpcio, msgpack_object *request_obj,
                       msgpack_packer *response_msg) {
  int rc;
  char *program_name;
  size_t program_len;

  obj_get_or_fail(request_obj, "program", MSGPACK_OBJECT_RAW, &program_name,
                  &program_len, response_msg)
  //rc = obj_get(request_obj, "program", MSGPACK_OBJECT_RAW, &program_name, &program_len);
  //if (rc != 0) {
    //api_respond_error(request_obj, response_msg, "Message had no 'program' field");
    //return;
  //}

  int64_t instance = -1;
  rc = obj_get(request_obj, "instance", MSGPACK_OBJECT_NEGATIVE_INTEGER, &instance, NULL);
  /* A missing 'instance' field is assumed ot mean "all" instances */

  printf("Request to restart '%.*s' instance '%ld'\n", (int)program_len,
         program_name, instance);

  msgpack_pack_map(response_msg, 1);
  msgpack_pack_string(response_msg, "results", 7);

  pn_prog_each(rpcio->procnanny, i, program, {
    if (strncmp(program->name, program_name, program_len) != 0) {
      continue;
    }
    msgpack_pack_map(response_msg, 1);
    printf("Program name: %.*s\n", program_len, program_name);
    msgpack_pack_string(response_msg, program_name, program_len);
    msgpack_pack_map(response_msg, instance == -1 ? program->nprocs : 1);
    pn_prog_proc_each(program, i, process, {
      if (instance >= 0 && instance != i) {
        continue;
      }
      rc = pn_proc_signal(process, SIGTERM);
      printf("kill[%d] => %d\n", i, rc);
      msgpack_pack_int(response_msg, i);

      if (rc == PN_OK) {
        msgpack_pack_true(response_msg);
      } else {
        msgpack_pack_string(response_msg, "process wasn't running", 22);
      }
    }); /* pn_prog_proc_each */
  }); /* pn_prog_each */
} /* api_request_restart */

