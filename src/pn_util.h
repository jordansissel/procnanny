#ifndef _PN_UTIL_H_
#define _PN_UTIL_H_

#include <sys/types.h>

/**
 * Find the uid for a user.
 *
 * @return Zero on success. On error, the return value will be -1 (no such
 *   user) or an errno value you can use with strerror(3) and friends.
 */
int pn_util_uid(char *username, uid_t *uid);

/**
 * Find a gid for a group.
 *
 * @return Zero on success. On error, the return value will be -1 (no such
 * user) or an errno value you can use with strerror(3) and friends.
 */
int pn_util_gid(char *group, gid_t *gid);

#endif /* _PN_UTIL_H_ */
