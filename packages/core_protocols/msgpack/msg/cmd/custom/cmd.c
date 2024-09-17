//
// This file is part of the TEN Framework project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include "include_internal/ten_runtime/msg/cmd_base/cmd/cmd.h"

#include "core_protocols/msgpack/msg/cmd/custom/cmd.h"
#include "core_protocols/msgpack/msg/cmd/custom/field/field_info.h"
#include "core_protocols/msgpack/msg/field/field_info.h"
#include "ten_utils/macro/check.h"
#include "ten_runtime/msg/cmd/custom/cmd.h"

static ten_cmd_t *get_raw_cmd(ten_shared_ptr_t *self) {
  TEN_ASSERT(self && ten_cmd_base_check_integrity(self), "Invalid argument.");

  return (ten_cmd_t *)ten_shared_ptr_get_data(self);
}

bool ten_msgpack_cmd_custom_serialize(ten_shared_ptr_t *self,
                                      msgpack_packer *pck, ten_error_t *err) {
  TEN_ASSERT(self && ten_cmd_base_check_integrity(self) && pck,
             "Invalid argument.");

  ten_cmd_t *raw_cmd = get_raw_cmd(self);

  for (size_t i = 0; i < ten_cmd_custom_fields_info_size; ++i) {
    ten_msg_field_serialize_func_t serialize =
        ten_cmd_custom_fields_info[i].serialize;
    if (serialize) {
      serialize(&raw_cmd->cmd_base_hdr.msg_hdr, pck);
    }
  }

  return true;
}

bool ten_msgpack_cmd_custom_deserialize(ten_shared_ptr_t *self,
                                        msgpack_unpacker *unpacker,
                                        msgpack_unpacked *unpacked) {
  TEN_ASSERT(self && ten_cmd_base_check_integrity(self) && unpacker && unpacked,
             "Invalid argument.");

  ten_cmd_t *raw_cmd = get_raw_cmd(self);

  for (size_t i = 0; i < ten_cmd_custom_fields_info_size; ++i) {
    ten_msg_field_deserialize_func_t deserialize =
        ten_cmd_custom_fields_info[i].deserialize;
    if (deserialize) {
      if (!deserialize(&raw_cmd->cmd_base_hdr.msg_hdr, unpacker, unpacked)) {
        return false;
      }
    }
  }

  return true;
}
