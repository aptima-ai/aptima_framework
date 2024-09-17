//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include "core_protocols/msgpack/msg/cmd/close_app/cmd.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "core_protocols/msgpack/msg/cmd/close_app/field/field_info.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd/close_app/cmd.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "ten_runtime/ten.h"
#include "ten_utils/lib/smart_ptr.h"

bool ten_msgpack_cmd_close_app_serialize(ten_shared_ptr_t *self,
                                         msgpack_packer *pck,
                                         ten_error_t *err) {
  TEN_ASSERT(
      self && ten_msg_get_type(self) == TEN_MSG_TYPE_CMD_CLOSE_APP && pck,
      "Invalid argument.");

  ten_cmd_close_app_t *raw_cmd =
      (ten_cmd_close_app_t *)ten_msg_get_raw_msg(self);

  for (size_t i = 0; i < ten_cmd_close_app_fields_info_size; ++i) {
    ten_msg_field_serialize_func_t serialize =
        ten_cmd_close_app_fields_info[i].serialize;
    if (serialize) {
      serialize(&raw_cmd->cmd_hdr.cmd_base_hdr.msg_hdr, pck);
    }
  }

  return true;
}

bool ten_msgpack_cmd_close_app_deserialize(ten_shared_ptr_t *self,
                                           msgpack_unpacker *unpacker,
                                           msgpack_unpacked *unpacked) {
  TEN_ASSERT(self && unpacker && unpacked, "Invalid argument.");

  ten_cmd_close_app_t *raw_cmd =
      (ten_cmd_close_app_t *)ten_msg_get_raw_msg(self);

  for (size_t i = 0; i < ten_cmd_close_app_fields_info_size; ++i) {
    ten_msg_field_deserialize_func_t deserialize =
        ten_cmd_close_app_fields_info[i].deserialize;
    if (deserialize) {
      if (!deserialize(&raw_cmd->cmd_hdr.cmd_base_hdr.msg_hdr, unpacker,
                       unpacked)) {
        return false;
      }
    }
  }

  return true;
}
