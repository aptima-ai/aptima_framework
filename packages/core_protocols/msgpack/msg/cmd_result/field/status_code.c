//
// This file is part of the TEN Framework project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include "include_internal/ten_runtime/msg/cmd_base/cmd_result/field/status_code.h"

#include <assert.h>
#include <msgpack/pack.h>
#include <msgpack/unpack.h>

#include "core_protocols/msgpack/common/common.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd_result/cmd.h"
#include "msgpack/object.h"
#include "ten_runtime/msg/cmd_result/cmd_result.h"

void ten_msgpack_cmd_result_code_serialize(ten_msg_t *self,
                                           msgpack_packer *pck) {
  TEN_ASSERT(
      self && ten_raw_msg_get_type(self) == TEN_MSG_TYPE_CMD_RESULT && pck,
      "Invalid argument.");

  int rc = msgpack_pack_int32(pck, ((ten_cmd_result_t *)self)->status_code);
  TEN_ASSERT(rc == 0, "Should not happen.");
}

bool ten_msgpack_cmd_result_code_deserialize(ten_msg_t *self,
                                             msgpack_unpacker *unpacker,
                                             msgpack_unpacked *unpacked) {
  TEN_ASSERT(self && unpacker && unpacked, "Invalid argument.");

  msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
  if (rc == MSGPACK_UNPACK_SUCCESS) {
    if (MSGPACK_DATA_TYPE == MSGPACK_OBJECT_POSITIVE_INTEGER) {
      ((ten_cmd_result_t *)self)->status_code = (int32_t)MSGPACK_DATA_I64;
      return true;
    } else {
      TEN_ASSERT(0, "Should not happen.");
    }
  } else {
    TEN_ASSERT(0, "Should not happen.");
  }

  return false;
}
