#include "pn_util.h"
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <stdio.h>

int pn_util_uid(char *username, uid_t *uid) {
  int ret;
  struct passwd pw;
  struct passwd *result = NULL;
  char buf[255]; /* shouldn't need more than this, should we? */
  int buflen = 255;

  ret = getpwnam_r(username, &pw, buf, buflen, &result);

  if (result == NULL) { /* lookup failed */
    *uid = -1;
    if (ret == 0) {
      fprintf(stderr, "getpwnam_r(%s) failed: no such user.\n", username);
      return -1;
    } else {
      fprintf(stderr, "getpwnam_r(%s) failed: %s\n", username, strerror(ret));
      return ret;
    }
  }
  *uid = pw.pw_uid;
  return 0;
} /* pn_util_uid */

int pn_util_gid(char *group, gid_t *gid) {
  int ret;
  struct group gr;
  struct group *result = NULL;
  char buf[255]; /* shouldn't need more than this, should we? */
  int buflen = 255;

  ret = getgrnam_r(group, &gr, buf, buflen, &result);

  if (result == NULL) { /* lookup failed */
    *gid = -1;
    if (ret == 0) {
      fprintf(stderr, "getgrnam_r(%s) failed: no such group.\n", group);
      return -1;
    } else {
      fprintf(stderr, "getgrnam_r(%s) failed: %s\n", group, strerror(ret));
      return ret;
    }
  }
  *gid = gr.gr_gid;
  return 0;
} /* pn_util_gid */
