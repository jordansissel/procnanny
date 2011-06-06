#include "procnanny.h"
#include "program.h"
#include "process.h"
#include "pn_api.h"

#include <signal.h>
#include <time.h>
#include <ev.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <msgpack.h>

#define msgpack_pack_string(packer, string, length) \
  msgpack_pack_raw(packer, length), msgpack_pack_raw_body(packer, string, length)

void api_cb(EV_P_ ev_io *watcher, int revents);

void api_request_dance(procnanny_t *pn, msgpack_object *request, char *client,
                       msgpack_packer *packer);
void api_request_restart(procnanny_t *pn, msgpack_object *request, char *client,
                       msgpack_packer *packer);

#define API_IP6 1

void start_api(procnanny_t *pn) {
  int fd;
  int ret;

#ifndef API_IP6
  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(3000);
  inet_pton(addr.sin_family, "0.0.0.0", &addr.sin_addr);
  fd = socket(AF_INET, SOCK_DGRAM, 0);
  printf("socket: %d\n", fd);
  ret = bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
#else
  struct sockaddr_in6 addr;
  bzero(&addr, sizeof(addr));
  addr.sin6_family = AF_INET6;
  addr.sin6_port = htons(3000);
  inet_pton(addr.sin6_family, "::1", &addr.sin6_addr);
  int inet_pton(int af, const char *src, void *dst);
  fd = socket(AF_INET6, SOCK_DGRAM, 0);
  printf("socket: %d\n", fd);
  ret = bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in6));
#endif
  printf("bind: %d\n", ret);
  if (ret) { perror("bind"); }

  int nonblock = 0;
  ret = fcntl(fd, F_SETFL, O_NONBLOCK);
  printf("fcntl: %d\n", ret);

  ev_io *udpio = calloc(1, sizeof(ev_io));
  ev_io_init(udpio, api_cb, fd, EV_READ);
  udpio->data = pn;
  ev_io_start(pn->loop, udpio);
} /* start_api */

void api_cb(EV_P_ ev_io *watcher, int revents) {
  static char client[96];
  static char buf[8192];
  size_t off = 0;
  ssize_t bytes;
  int ret = 0;
  msgpack_object *request;
  procnanny_t *pn = watcher->data;
#ifdef API_IP6
  struct sockaddr_in6 addr;
  socklen_t addrlen = sizeof(struct sockaddr_in6);
#else
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(struct sockaddr_in);
#endif

  msgpack_sbuffer *sbuf = msgpack_sbuffer_new();
  msgpack_unpacked msg;
  msgpack_unpacked_init(&msg);

  while (1) {
    char stringaddr[48];

    bytes = recvfrom(watcher->fd, buf, 8192, 0, (struct sockaddr *)&addr, &addrlen); 
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

    /* Find the method name */
    ret = obj_get(request, "request", MSGPACK_OBJECT_RAW, &method, &method_len);
    if (ret != 0) {
      fprintf(stderr, "Message had no 'request' field. Ignoring: ");
      msgpack_object_print(stderr, *request);
    } else {
      /* Found request */
      printf("The method is: '%.*s'\n", (int)method_len, method);

      msgpack_sbuffer *buffer = msgpack_sbuffer_new();
      msgpack_packer *packer = msgpack_packer_new(buffer, msgpack_sbuffer_write);

      int family;
#ifdef API_IP6
      family = addr.sin6_family;
#else
      family = addr.sin6_family;
#endif
      switch (family) {
        case AF_INET:
          inet_ntop(family, &(((struct sockaddr_in *)&addr)->sin_addr),
                    client, addrlen);
          break;
        case AF_INET6:
          inet_ntop(family, &(((struct sockaddr_in6 *)&addr)->sin6_addr),
                    client, addrlen);
          break;
        case AF_UNIX:
        default:
          printf("Unknown family: %d\n", family);
      }

      msgpack_pack_map(packer, 2);
      msgpack_pack_string(packer, "results", 7);

      struct timespec start, end;
      clock_gettime(CLOCK_MONOTONIC, &start);
      if (!strncmp("dance", method, method_len)) {
        api_request_dance(pn, request, client, packer);
      } else if (!strncmp("restart", method, method_len)) {
        api_request_restart(pn, request, client, packer);
      }
      clock_gettime(CLOCK_MONOTONIC, &end);

      double duration = (end.tv_sec - start.tv_sec) * 1000000000L;
      double duration_nsec = (end.tv_nsec - start.tv_nsec);
      if (duration_nsec < 0) {
        duration += duration_nsec + 1000000000L;
      } else {
        duration += duration_nsec;
      }

      msgpack_pack_string(packer, "stats", 5);
      msgpack_pack_map(packer, 1),
      msgpack_pack_string(packer, "duration", 8);
      msgpack_pack_double(packer, duration / 1000000000.);

      int bytes;
      bytes = sendto(watcher->fd, buffer->data, buffer->size, 0,
                     (struct sockaddr *)&addr, addrlen);
      if (bytes <= 0) {
        perror("sendto failed");
      }

      msgpack_sbuffer_free(buffer);
      msgpack_packer_free(packer);

    }
    msgpack_unpacked_destroy(&msg);
  } /* while (1) */

  msgpack_unpacked_destroy(&msg);
  msgpack_sbuffer_free(sbuf);
} /* api_cb */

void api_request_dance(procnanny_t *pn, msgpack_object *request, char *client,
                       msgpack_packer *packer) {
  printf("%s: Dance dance dance. La la la.\n", client);

  msgpack_pack_map(packer, 1);
  msgpack_pack_string(packer, "danced", 6);
  msgpack_pack_true(packer);
}

void api_request_restart(procnanny_t *pn, msgpack_object *request,
                         char *client, msgpack_packer *packer) {
  int ret;
  char *program_name;
  size_t program_len;
  ret = obj_get(request, "program", MSGPACK_OBJECT_RAW, &program_name, &program_len);

  int64_t instance = -1;
  ret = obj_get(request, "instance", MSGPACK_OBJECT_NEGATIVE_INTEGER, &instance, NULL);

  printf("Request to restart '%.*s' instance '%ld'\n", (int)program_len,
         program_name, instance);

  msgpack_pack_map(packer, 1);
  msgpack_pack_string(packer, "signal-results", 4);

  pn_prog_each(pn, i, program, {
    if (!strncmp(program->name, program_name, program_len)) {
      msgpack_pack_map(packer, 1);
      msgpack_pack_string(packer, program_name, program_len);
      msgpack_pack_map(packer, instance == -1 ? program->nprocs : instance);
      if (instance >= 0) {
        printf("instance >= 0 not supported yet: %ld \n", instance);
      } else {
        pn_prog_proc_each(program, i, process, {
          ret = pn_proc_signal(process, SIGTERM);
          printf("kill[%d] => %d\n", i, ret);
          msgpack_pack_int(packer, i);

          if (ret == PN_OK) {
            msgpack_pack_true(packer);
          } else {
            msgpack_pack_string(packer, "process wasn't running", 22);
          }
        })
      }
    }
  });
}
