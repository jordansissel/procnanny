#include <msgpack.h>
#include "msgpack_helpers.h"
#include "insist.h"

void free_msgpack_buffer(void *data, void *hint) {
  msgpack_sbuffer *buffer = hint;
  msgpack_sbuffer_free(buffer);
}

int obj_get(const msgpack_object *obj, const char *name,
            msgpack_object_type type, void *value, size_t *value_len) {
  if (obj->type != MSGPACK_OBJECT_MAP) {
    printf("invalid message type %d, expected MSGPACK_OBJECT_MAP(%d)\n", 
           obj->type, MSGPACK_OBJECT_MAP);
    return OBJ_GET_NOT_A_MAP;
  }

  int i = 0;
  for (i = 0; i < obj->via.map.size; i++) {
    msgpack_object_kv *kv = obj->via.map.ptr;
    msgpack_object *key = (msgpack_object *)&(obj->via.map.ptr[i].key);
    if (strncmp(name, key->via.raw.ptr, key->via.raw.size)) {
      continue;
    }

    msgpack_object *curvalue = (msgpack_object *)&(obj->via.map.ptr[i].val);
    if (type == MSGPACK_OBJECT_NEGATIVE_INTEGER 
        && curvalue->type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
      /* If we have a type of 'unsigned int' but asked for a signed one,
       * it's still ok. Just pretend we asked for whatever type it is now. */
      type = curvalue->type;
    }

    if (type != curvalue->type) {
      fprintf(stderr, 
              "Type requested for key '%s' was %d, but value is of type %d\n",
              name, type, curvalue->type);
      return OBJ_GET_WRONG_TYPE;
    }

    int j = 0;
    switch (curvalue->type & 255) {
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
      case MSGPACK_OBJECT_ARRAY:
        insist_return(value_len != NULL, OBJ_GET_WRONG_TYPE,
                      "value_len is required for ARRAY type to store length");
        *(size_t *)value_len = curvalue->via.array.size;
        *(msgpack_object **)value = curvalue->via.array.ptr;
        break;
      default:
        fprintf(stderr, "Unsupported type %d.\n", curvalue->type);
        break;
    }

    return OBJ_GET_SUCCESS;
  } /* for each map entry */

  return OBJ_GET_NOT_FOUND;
} /* int obj_get */

int array_get(const msgpack_object *obj, int index,
              msgpack_object_type type, void *value, size_t *value_len) {
  msgpack_object *element;
  element = obj->via.array.ptr + index;

  if (type == MSGPACK_OBJECT_NEGATIVE_INTEGER 
      && element->type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
    /* If we have a type of 'unsigned int' but asked for a signed one,
     * it's still ok. Just pretend we asked for whatever type it is now. */
    type = element->type;
  }

  //insist_return(element->type == type, OBJ_GET_WRONG_TYPE,
                //"array element[%d] is type %d, expected %d\n",
                //index, element->type, type);

  switch (type) {
    case MSGPACK_OBJECT_RAW:
      *(const char **)value = element->via.raw.ptr;
      *value_len = element->via.raw.size;
      break;
    case MSGPACK_OBJECT_DOUBLE:
      *(double *)value = element->via.dec;
      if (value_len != NULL) {
        *value_len = sizeof(double);
      }
      break;
    case MSGPACK_OBJECT_POSITIVE_INTEGER:
      *(uint64_t *)value = element->via.u64;
      if (value_len != NULL) {
        *value_len = sizeof(uint64_t);
      }
      printf("request: (type=%d) %lu\n", element->type, *(uint64_t *)value);
      break;
    case MSGPACK_OBJECT_NEGATIVE_INTEGER:
      *(int64_t *)value = element->via.i64;
      if (value_len != NULL) {
        *value_len = sizeof(int64_t);
      }
      printf("request: (type=%d) %ld\n", element->type, *(int64_t *)value);
      break;
    case MSGPACK_OBJECT_BOOLEAN:
      *(bool *)value = element->via.boolean;
      if (value_len != NULL) {
        *value_len = sizeof(bool);
      }
      //printf("request: (type=%d, size=%zd) %s\n", element->type, *value_len,
             //*(bool *)value ? "true" : "false");
      break;
    case MSGPACK_OBJECT_ARRAY:
      insist_return(value_len != NULL, OBJ_GET_WRONG_TYPE,
                    "value_len is required for ARRAY type to store length");
      *(size_t *)value_len = element->via.array.size;
      *(msgpack_object **)value = element->via.array.ptr;
      break;
    case MSGPACK_OBJECT_MAP:
      insist_return(value_len != NULL, OBJ_GET_WRONG_TYPE,
                    "value_len is required for ARRAY type to store length");
      *(size_t *)value_len = element->via.map.size;
      *(msgpack_object_kv **)value = element->via.map.ptr;
      break;
    default:
      fprintf(stderr, "Unsupported type %d.\n", element->type);
      break;
  } /* case element->type */

  return OBJ_GET_SUCCESS;
} /* int array_get */
