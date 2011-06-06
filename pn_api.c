#include "procnanny.h"
#include "pn_api.h"

#include <ev.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <msgpack.h>

void api_cb(EV_P_ ev_io *watcher, int revents);

void api_request_dance(msgpack_object *request, struct sockaddr *addr, socklen_t addrlen);

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
    ssize_t bytes;
    int ret = 0;
    msgpack_object *request;
    struct sockaddr addr;
    socklen_t addrlen;

    bytes = recvfrom(watcher->fd, buf, 8192, 0, &addr, &addrlen); 
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

    request = (msgpack_object*)&msg.data;

    if (request->type != MSGPACK_OBJECT_MAP) {
      printf("invalid message type %d, expected MSGPACK_OBJECT_MAP(%d)\n", 
             request->type, MSGPACK_OBJECT_MAP);
      break;
    }

    char *method;
    size_t method_len;
    ret = obj_get(request, "request", MSGPACK_OBJECT_RAW, &method, &method_len);
    if (ret != 0) {
      fprintf(stderr, "Message had no 'request' field. Ignoring: ");
      msgpack_object_print(stderr, *request);
    } else {
      /* Found request */
      printf("The method is: '%.*s'\n", (int)method_len, method);

      if (!strncmp("dance", method, method_len)) {
        api_request_dance(request, &addr, addrlen);
      }
    }
    msgpack_unpacked_destroy(&msg);
  } /* while (1) */

  msgpack_unpacked_destroy(&msg);
  msgpack_sbuffer_free(sbuf);
} /* api_cb */

void api_request_dance(msgpack_object *request, struct sockaddr *addr, socklen_t addrlen) {
  switch (addrlen) {
    case sizeof(struct sockaddr_in):
      printf("Client: %s\n", inet_ntoa(((struct sockaddr_in *)addr)->sin_addr));
      break;
    default:
      printf("Unknown addrlen: %d\n", addrlen);
  }

  printf("Dance dance dance. La la la.\n");
}
