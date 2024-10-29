//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include "core_protocols/msgpack/msg/cmd/field/seq_id.h"

#include <assert.h>

#include "include_internal/ten_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "ten_utils/macro/check.h"

void ten_msgpack_cmd_seq_id_serialize(ten_msg_t *self, msgpack_packer *pck) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) && pck,
             "Invalid argument.");

  const char *seq_id =
      ten_value_peek_raw_str(&(((ten_cmd_base_t *)self)->seq_id));

  int rc = msgpack_pack_str_with_body(pck, seq_id, strlen(seq_id));
  TEN_ASSERT(rc == 0, "Should not happen.");
}

bool ten_msgpack_cmd_seq_id_deserialize(ten_msg_t *self,
                                        msgpack_unpacker *unpacker,
                                        msgpack_unpacked *unpacked) {
  TEN_ASSERT(self && unpacker && unpacked, "Invalid argument.");

  msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
  if (rc == MSGPACK_UNPACK_SUCCESS) {
    if (MSGPACK_DATA_TYPE == MSGPACK_OBJECT_STR) {
      ten_string_set_formatted(
          ten_value_peek_string(&((ten_cmd_base_t *)self)->seq_id), "%.*s",
          MSGPACK_DATA_STR_SIZE, MSGPACK_DATA_STR_PTR);
      return true;
    } else {
      TEN_ASSERT(0, "Should not happen.");
    }
  } else {
    TEN_ASSERT(0, "Should not happen.");
  }

  return false;
}
