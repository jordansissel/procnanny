#ifndef _API_H_
#define _API_H_

#include <ev.h>
#include "procnanny.h"
#include <msgpack.h>

typedef struct rpc_io {
  ev_io io;
  procnanny_t *procnanny;
  void *socket;
} rpc_io;

#define msgpack_pack_string(packer, string, length) \
  (length == -1) \
    ? msgpack_pack_raw(packer, strlen(string)), msgpack_pack_raw_body(packer, string, strlen(string)) \
    : msgpack_pack_raw(packer, length), msgpack_pack_raw_body(packer, string, length)

#endif /* _API_H_ */
