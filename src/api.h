#ifndef _API_H_
#define _API_H_

#include <ev.h>
#include "procnanny.h"
#include "msgpack_helpers.h"
#include <msgpack.h>

typedef struct rpc_io {
  ev_io io;
  procnanny_t *procnanny;
  void *socket;
} rpc_io;

void api_respond_error(msgpack_object *request, msgpack_packer *response,
                       const char *error);

#define obj_get_or_fail(request_obj, field, type, program_name_ptr, \
                        program_len_ptr, response_msg) \
  rc = obj_get(request_obj, field, type, program_name_ptr, program_len_ptr); \
  if (rc != 0) { \
    api_respond_error(request_obj, response_msg, "Message is missing '" field "' field"); \
    return; \
  }

#endif /* _API_H_ */
