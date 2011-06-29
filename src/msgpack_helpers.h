#ifndef _MSGPACK_HELPERS_H
#define _MSGPACK_HELPERS_H

#include <msgpack.h>

#define msgpack_pack_string(packer, string, length) \
  (length == -1) \
    ? msgpack_pack_raw(packer, strlen(string)), msgpack_pack_raw_body(packer, string, strlen(string)) \
    : msgpack_pack_raw(packer, length), msgpack_pack_raw_body(packer, string, length)


enum obj_get_return_codes {
  OBJ_GET_SUCCESS = 0,        /* success! */
  OBJ_GET_NOT_FOUND = 1,      /* key name not found */
  OBJ_GET_WRONG_TYPE = 2,     /* requested type is not correct */
  OBJ_GET_NOT_A_MAP = 3,      /* object given is not a msgpack map */
  OBJ_GET_NOT_AN_ARRAY = 4,   /* object given is not a msgpack array */
  ARRAY_GET_OUT_OF_RANGE = 5  /* array index out of range */
};

#define MSGPACK_OBJECT_ARRAY_OF_STRING \
  ((MSGPACK_OBJECT_RAW << 8) | MSGPACK_OBJECT_ARRAY
#define MSGPACK_OBJECT_ARRAY_TYPE(type) (type >> 8)

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

int array_get(const msgpack_object *obj, int index,
              msgpack_object_type type, void *value, size_t *value_len);
/**
 * Helper method for use with zmq_msg_t
 */
void free_msgpack_buffer(void *data, void *hint);

#endif /* _MSGPACK_HELPERS_H */
