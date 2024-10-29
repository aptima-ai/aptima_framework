//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include <assert.h>
#include <msgpack/unpack.h>

#include "cmake/msgpackc/install/include/msgpack/object.h"
#include "core_protocols/msgpack/common/common.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd_result/cmd.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd_result/field/status_code.h"
#include "include_internal/ten_utils/value/value_set.h"
#include "msgpack/object.h"
#include "ten_runtime/msg/cmd_result/cmd_result.h"
#include "ten_utils/value/value_get.h"

void ten_msgpack_cmd_result_is_final_serialize(ten_msg_t *self,
                                               msgpack_packer *pck) {
  TEN_ASSERT(
      self && ten_raw_msg_get_type(self) == TEN_MSG_TYPE_CMD_RESULT && pck,
      "Invalid argument.");

  bool is_final =
      ten_value_get_bool(&((ten_cmd_result_t *)self)->is_final, NULL);

  if (is_final) {
    int rc = msgpack_pack_true(pck);
    TEN_ASSERT(rc == 0, "Should not happen.");
  } else {
    int rc = msgpack_pack_false(pck);
    TEN_ASSERT(rc == 0, "Should not happen.");
  }
}

bool ten_msgpack_cmd_result_is_final_deserialize(ten_msg_t *self,
                                                 msgpack_unpacker *unpacker,
                                                 msgpack_unpacked *unpacked) {
  TEN_ASSERT(self && unpacker && unpacked, "Invalid argument.");

  msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
  if (rc == MSGPACK_UNPACK_SUCCESS) {
    if (MSGPACK_DATA_TYPE == MSGPACK_OBJECT_BOOLEAN) {
      ten_value_set_bool(&((ten_cmd_result_t *)self)->is_final,
                         MSGPACK_DATA_BOOL);
      return true;
    } else {
      TEN_ASSERT(0, "Should not happen.");
    }
  } else {
    TEN_ASSERT(0, "Should not happen.");
  }

  return false;
}
