#ifndef _ZMQ_HELPERS_H
#define _ZMQ_HELPERS_H

#define zmq_msg_each(socket, msgvar, block) \
{ \
  uint32_t __events; \
  size_t __len; \
  while (1) { \
    zmq_msg_t msgvar; \
    if (zmq_getsockopt(socket, ZMQ_EVENTS, &__events, &__len) != 0) { \
      perror("zmq_getsockopt/ZMQ_EVENTS"); \
      break; \
    } \
    if ((events & ZMQ_POLLIN) == 0) { \
      break; \
    } \
    if (zmq_msg_init(&msgvar) != 0) { \
      perror("zmq_msg_init"); \
      break; \
    } \
    if (zmq_recv(socket, &msgvar, ZMQ_NOBLOCK) != 0) { \
      perror("zmq_recv"); \
      break; \
    } \
    { \
      block \
    } \
    zmq_msg_close(&__msg); \
  } \
} /* define zmq_msg_each */

#endif /* _ZMQ_HELPERS_H */
