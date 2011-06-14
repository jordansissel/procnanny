#ifndef _MSGPACK_HELPERS_H
#define _MSGPACK_HELPERS_H

#include <msgpack.h>

enum obj_get_return_codes {
  OBJ_GET_SUCCESS = 0,     /* success! */
  OBJ_GET_NOT_FOUND = 1,   /* key name not found */
  OBJ_GET_WRONG_TYPE = 2,  /* requested type is not correct */
  OBJ_GET_NOT_A_MAP = 3    /* object given is not a msgpack map */
};

/** 
 * Get a value by string name from a msgpack object
 *
 * If you want to read a string, use type=MSGPACK_OBJECT_RAW.
 *
 * @param obj - a msgpack_object instance. Must be a map type.
 * @param name - a string for the key name.
 * @param type - a msgpack object type (MSGPACK_OBJECT_DOUBLE, etc)
 * @param value - a pointer where the result will be written
 * @param value_len - a pointer to where the result's size will be written
 */
int obj_get(const msgpack_object *obj, const char *name, msgpack_object_type type, 
            void *value, size_t *value_len);

#endif /* _MSGPACK_HELPERS_H */
