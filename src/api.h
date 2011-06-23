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

#endif /* _API_H_ */
