#include <msgpack.h>
#include "msgpack_helpers.h"

int obj_get(const msgpack_object *obj, const char *name,
            msgpack_object_type type, void *value, size_t *value_len) {

  if (obj->type != MSGPACK_OBJECT_MAP) {
    printf("invalid message type %d, expected MSGPACK_OBJECT_MAP(%d)\n", 
           obj->type, MSGPACK_OBJECT_MAP);
    return OBJ_GET_NOT_A_MAP;
  }

  /* find the key 'request', then fire off that api request */
  int i = 0;
  for (i = 0; i < obj->via.map.size; i++) {
    msgpack_object_kv *kv = obj->via.map.ptr;
    msgpack_object *key = (msgpack_object *)&(obj->via.map.ptr[i].key);
    if (strncmp(name, key->via.raw.ptr, key->via.raw.size)) {
      continue;
    }

    msgpack_object *curvalue = (msgpack_object *)&(obj->via.map.ptr[i].val);
    if (type != curvalue->type) {
      fprintf(stderr, 
              "Type requested for key '%s' was %d, but value is of type %d\n",
              name, type, curvalue->type);
      return OBJ_GET_WRONG_TYPE;
    }

    switch (curvalue->type) {
      case MSGPACK_OBJECT_RAW:
        *(const char **)value = curvalue->via.raw.ptr;
        *value_len = curvalue->via.raw.size;
        //printf("request: (type=%d, size=%zd) %.*s\n", curvalue->type,
               //*value_len, (int)*value_len, *(char **)value);
        break;
      case MSGPACK_OBJECT_DOUBLE:
        *(double *)value = curvalue->via.dec;
        if (value_len != NULL) {
          *value_len = sizeof(double);
        }
        //printf("request: (type=%d, size=%zd) %lf\n", curvalue->type, *value_len,
               //*(double *)value);
        break;
      case MSGPACK_OBJECT_POSITIVE_INTEGER:
        *(uint64_t *)value = curvalue->via.u64;
        if (value_len != NULL) {
          *value_len = sizeof(uint64_t);
        }
        printf("request: (type=%d) %lu\n", curvalue->type, *(uint64_t *)value);
        break;
      case MSGPACK_OBJECT_NEGATIVE_INTEGER:
        *(int64_t *)value = curvalue->via.i64;
        if (value_len != NULL) {
          *value_len = sizeof(int64_t);
        }
        printf("request: (type=%d) %ld\n", curvalue->type, *(int64_t *)value);
        break;
      case MSGPACK_OBJECT_BOOLEAN:
        *(bool *)value = curvalue->via.boolean;
        if (value_len != NULL) {
          *value_len = sizeof(bool);
        }
        //printf("request: (type=%d, size=%zd) %s\n", curvalue->type, *value_len,
               //*(bool *)value ? "true" : "false");
        break;
      default:
        fprintf(stderr, "Unsupported type %d.\n", curvalue->type);
        break;
    }

    return OBJ_GET_SUCCESS;
  } /* for each map entry */

  return OBJ_GET_NOT_FOUND;
} /* int obj_get */
