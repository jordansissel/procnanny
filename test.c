#include <msgpack.h>
#include <stdio.h>
 
int main(void) {
  /* creates buffer and serializer instance. */
  msgpack_sbuffer* buffer = msgpack_sbuffer_new();
  msgpack_packer* pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);

  /* serializes ["Hello", "MessagePack"]. */
  //msgpack_pack_array(pk, 2);
  //msgpack_pack_raw(pk, 5);
  //msgpack_pack_raw_body(pk, "Hello", 5);
  //msgpack_pack_raw(pk, 11);
  //msgpack_pack_raw_body(pk, "MessagePack", 11);
  msgpack_pack_map(pk, 1);
  msgpack_pack_raw(pk, 5);
  msgpack_pack_raw_body(pk, "Hello", 5);
  msgpack_pack_raw(pk, 11);
  msgpack_pack_raw_body(pk, "MessagePack", 11);
  //kv.key = calloc(1, sizeofkk
  //kv.val

  /* deserializes it. */
  msgpack_unpacked msg;
  msgpack_unpacked_init(&msg);
  while (1) {
    bool success = msgpack_unpack_next(&msg, buffer->data, buffer->size, NULL);

    /* prints the deserialized object. */
    msgpack_object obj = msg.data;
    msgpack_object_print(stdout, obj);  /*=> ["Hello", "MessagePack"] */
    printf("\n");

    msgpack_unpacked_destroy(&msg);
  }
  msgpack_packer_free(pk);
  msgpack_sbuffer_free(buffer);
}
