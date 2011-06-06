#include "procnanny.h"
#include "pn_api.h"

#include <ev.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <msgpack.h>

void api_cb(EV_P_ ev_io *watcher, int revents);

void start_api(procnanny_t *pn) {
  int fd;
  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));

  addr.sin_family = PF_INET;
  addr.sin_port = htons(3000);
  inet_aton("0.0.0.0", &addr.sin_addr);
  fd = socket(PF_INET, SOCK_DGRAM, 0);
  printf("socket: %d\n", fd);

  int nonblock = 0;
  int ret;
  ret = fcntl(fd, F_SETFL, O_NONBLOCK);
  printf("fcntl: %d\n", ret);

  ret = bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
  printf("bind: %d\n", ret);

  ev_io *udpio = calloc(1, sizeof(ev_io));
  ev_io_init(udpio, api_cb, fd, EV_READ);
  ev_io_start(pn->loop, udpio);
} /* start_api */

void api_cb(EV_P_ ev_io *watcher, int revents) {
  static char buf[8192];
  msgpack_sbuffer *sbuf = msgpack_sbuffer_new();
  msgpack_unpacked msg;
  msgpack_unpacked_init(&msg);

  while (1) {
    size_t off = 0;
    int bytes;
    int ret = 0;
    msgpack_object *obj;

    bytes = read(watcher->fd, buf, 8192);
    if (bytes <= 0) {
      break;
    }

    msgpack_sbuffer_write(sbuf, buf, bytes);

    ret = msgpack_unpack_next(&msg, sbuf->data, sbuf->size, &off);

    if (!ret) {
      printf("msgpack_unpack_next: %d\n", ret);
      /* Not a full message, or corrupt stream */
      break;
    }

    obj = (msgpack_object*)&msg.data;

    if (obj->type != MSGPACK_OBJECT_MAP) {
      printf("invalid message type %d, expected MSGPACK_OBJECT_MAP(%d)\n", 
             obj->type, MSGPACK_OBJECT_MAP);
      break;
    }

    /* find the key 'request', then fire off that api request */
    int i = 0;
    for (i = 0; i < obj->via.map.size; i++) {
      msgpack_object_kv *kv = obj->via.map.ptr;
      msgpack_object *key = (msgpack_object *)&(obj->via.map.ptr[i].key);
      if (!strncmp("request", key->via.raw.ptr, key->via.raw.size)) {
        msgpack_object *value = (msgpack_object *)&(obj->via.map.ptr[i].val);
        printf("request: (type=%d, size=%d) %.*s\n", value->type,
               value->via.raw.size, value->via.raw.size, value->via.raw.ptr);
        /* TODO(sissel): Look up the request in a dispatch tables */
      }
    }
    msgpack_unpacked_destroy(&msg);
  } /* while (1) */

  msgpack_unpacked_destroy(&msg);
  msgpack_sbuffer_free(sbuf);
}
