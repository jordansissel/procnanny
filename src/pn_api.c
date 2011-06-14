#include "procnanny.h"
#include "program.h"
#include "process.h"
#include "pn_api.h"
#include "insist.h"
#include <zmq.h>
#include <zmq_utils.h>

#include <signal.h>
#include <time.h>
#include <ev.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
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


void rpc_receive(rpc_io *rpcio);
void rpc_handle(rpc_io *rpcio, zmq_msg_t *request);
void api_request_dance(rpc_io *rpcio, msgpack_object *request,
                       msgpack_packer *packer);
void api_request_restart(rpc_io *rpcio, msgpack_object *request,
                       msgpack_packer *packer);

void free_msgpack_buffer(void *data, void *hint);

void rpc_cb(EV_P_ ev_io *watcher, int revents) {
  rpc_io *rpcio = (rpc_io *)watcher;
  int rc;
  int zmqevents;
  size_t len = sizeof(zmqevents);

  rc = zmq_getsockopt(rpcio->socket, ZMQ_EVENTS, &zmqevents, &len);
  insist(rc == 0, "zmq_getsockopt(ZMQ_EVENTS) expected to return 0, "
         "but got %d", rc);

  /* Check for zmq events */
  //printf("Ready\n");

  if (zmqevents & ZMQ_POLLIN == 0) {
    /* No messages to receive */
    return;
  }

  rpc_receive(rpcio);
  //ev_io_stop(EV_A_ watcher);
} /* rpc_cb */

void rpc_receive(rpc_io *rpcio) {
  zmq_msg_t request;
  int rc;
  size_t msgsize;

  rc = zmq_msg_init(&request);
  rc = zmq_recv(rpcio->socket, &request, ZMQ_NOBLOCK);
  
  insist_return(rc == 0 || errno == EAGAIN, (void)(0),
                "zmq_recv: expected success or EAGAIN, got errno %d:%s",
                errno, strerror(errno));

  if (rc == -1 && errno == EAGAIN) {
    /* nothing to do, would block */
    zmq_msg_close(&request);
    return;
  }

  msgsize = zmq_msg_size(&request);
  rpc_handle(rpcio, &request);
  zmq_msg_close(&request);
} /* rpc_receive */

void rpc_handle(rpc_io *rpcio, zmq_msg_t *request) {
  /* Parse the msgpack */
  zmq_msg_t response;
  int rc;
  msgpack_unpacked request_msg;
  msgpack_unpacked_init(&request_msg);
  rc = msgpack_unpack_next(&request_msg, zmq_msg_data(request),
                           zmq_msg_size(request), NULL);
  insist_return(rc, (void)(0), "Failed to unpack message '%.*s'",
                zmq_msg_size(request), zmq_msg_data(request));

  msgpack_object request_obj = request_msg.data;
  printf("Object: ");
  msgpack_object_print(stdout, request_obj);  /*=> ["Hello", "MessagePack"] */
  printf("\n");

  /* Find the method name */
  char *method = NULL;
  size_t method_len = -1;
  rc = obj_get(&request_obj, "request", MSGPACK_OBJECT_RAW, &method, &method_len);
  msgpack_sbuffer *buffer = msgpack_sbuffer_new();
  msgpack_packer *response_msg = msgpack_packer_new(buffer, msgpack_sbuffer_write);

  printf("Method: %.*s\n", method_len, method);

  if (rc != 0) {
    fprintf(stderr, "Message had no 'request' field. Ignoring: ");
    msgpack_object_print(stderr, request_obj);
    msgpack_pack_map(response_msg, 2);
    msgpack_pack_string(response_msg, "error", -1);
    msgpack_pack_string(response_msg, "Message had no 'request' field", -1);
    msgpack_pack_string(response_msg, "request", -1);
    msgpack_pack_object(response_msg, request_obj);
  } else {
    /* Found request */
    printf("The method is: '%.*s'\n", (int)method_len, method);

    msgpack_pack_map(response_msg, 2);
    msgpack_pack_string(response_msg, "results", 7);

    void *clock = zmq_stopwatch_start();
    /* TODO(sissel): Use gperf here */
    if (!strncmp("dance", method, method_len)) {
      api_request_dance(rpcio, &request_obj, response_msg);
    } else if (!strncmp("restart", method, method_len)) {
      api_request_restart(rpcio, &request_obj, response_msg);
    } else if (!strncmp("status", method, method_len)) {
      api_request_status(rpcio, &request_obj, response_msg);
    }
    double duration = zmq_stopwatch_stop(clock) / 1000000.;

    msgpack_pack_string(response_msg, "stats", 5);
    msgpack_pack_map(response_msg, 1),
    msgpack_pack_string(response_msg, "duration", 8);
    msgpack_pack_double(response_msg, duration);
  }

  zmq_msg_init_data(&response, buffer->data, buffer->size, free_msgpack_buffer, buffer); 
  zmq_send(rpcio->socket, &response, 0);
  zmq_msg_close(&response);

  //msgpack_sbuffer_free(buffer);
  msgpack_packer_free(response_msg);
  msgpack_unpacked_destroy(&request_msg);
} /* rpc_handle */

void free_msgpack_buffer(void *data, void *hint) {
  msgpack_sbuffer *buffer = hint;
  msgpack_sbuffer_free(buffer);
}

void start_api(procnanny_t *pn) {
  void *zmq = zmq_init(1);
  int rc;
  static char endpoint[] = "tcp://*:3333";

  void *socket = zmq_socket(zmq, ZMQ_REP);
  insist(socket != NULL, "zmq_socket returned NULL. Unexpected (%x).", socket);

  rc = zmq_bind(socket, endpoint);
  insist(rc == 0, "zmq_bind(\"%s\") returned %d (I wanted 0).", endpoint, rc);

  /* TODO(sissel): Turn this 'get fd' into a method */
  int socket_fd;
  size_t len = sizeof(socket_fd);
  rc = zmq_getsockopt(socket, ZMQ_FD, &socket_fd, &len);
  insist(rc == 0, "zmq_getsockopt(ZMQ_FD) expected to return 0, but got %d",
         rc);

  rpc_io *rpcio = calloc(1, sizeof(rpc_io));
  rpcio->procnanny = pn;
  rpcio->socket = socket;
  ev_io_init(&rpcio->io, rpc_cb, socket_fd, EV_READ);
  ev_io_start(pn->loop, &rpcio->io);

  printf("RPC/API started\n");
} /* start_api */

void api_request_dance(rpc_io *rpcio, msgpack_object *request,
                       msgpack_packer *response_msg) {
  printf("Dance dance dance. La la la.\n");

  msgpack_pack_map(response_msg, 1);
  msgpack_pack_string(response_msg, "danced", 6);
  msgpack_pack_true(response_msg);
}

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
