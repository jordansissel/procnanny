#ifndef _PROCNANNY_H_
#define _PROCNANNY_H_

struct program;
#include "program.h"

struct procnanny {
  struct ev_loop *loop;
  struct program *programs;
  size_t programs_len;

  void *zmq;
  void *eventsocket; /* PUB socket */
};  /* struct procnanny */

typedef struct procnanny procnanny_t;

/** Iterate over programs.
 *
 * @param pn - a pointer to a procnanny_t
 * @param iname - the variable name to hold integer iterator value
 * @param varname - the variable name to hold the 'program_t' for each
 *   iteration
 * @param block - a block of code.
 */
#define pn_prog_each(pn, iname, varname, block) \
{ \
  int iname = 0; \
  for (iname = 0; iname < (pn)->programs_len; iname++) { \
    program_t *varname = &(pn)->programs[iname]; \
    { \
      block \
    } \
  } \
}
#endif /* _PROCNANNY_H_ */
