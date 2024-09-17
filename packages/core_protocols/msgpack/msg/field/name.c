//
// This file is part of the TEN Framework project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include "core_protocols/msgpack/msg/cmd/field/cmd_id.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "ten_utils/macro/check.h"
#include "msgpack/object.h"

void ten_msgpack_msg_name_serialize(ten_msg_t *self, msgpack_packer *pck) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) && pck,
             "Invalid argument.");

  const char *msg_name = ten_raw_msg_get_name(self);

  if (msg_name) {
    int rc = msgpack_pack_str_with_body(pck, msg_name, strlen(msg_name));
    TEN_ASSERT(rc == 0, "Should not happen.");
  }
}

bool ten_msgpack_msg_name_deserialize(ten_msg_t *self,
                                      msgpack_unpacker *unpacker,
                                      msgpack_unpacked *unpacked) {
  TEN_ASSERT(self && unpacker && unpacked, "Invalid argument.");

  msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
  if (rc == MSGPACK_UNPACK_SUCCESS) {
    if (MSGPACK_DATA_TYPE == MSGPACK_OBJECT_STR) {
      ten_raw_msg_set_name_with_size(self, MSGPACK_DATA_STR_PTR,
                                     MSGPACK_DATA_STR_SIZE, NULL);
      return true;
    } else {
      TEN_ASSERT(0, "Should not happen.");
    }
  } else {
    TEN_ASSERT(0, "Should not happen.");
  }

  return false;
}
