#ifndef _PROCNANNY_H_
#define _PROCNANNY_H_

struct program;
#include "program.h"

struct procnanny {
  struct ev_loop *loop;
  struct program *programs;
  size_t programs_len;
}; 

typedef struct procnanny procnanny_t;

#endif /* _PROCNANNY_H_ */
