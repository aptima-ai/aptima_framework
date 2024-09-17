//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include "core_protocols/msgpack/msg/cmd/cmd.h"

#include <string.h>

#include "core_protocols/msgpack/msg/cmd/field/field_info.h"
#include "core_protocols/msgpack/msg/field/field_info.h"
#include "core_protocols/msgpack/msg/msg.h"
#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/ten_runtime/msg/field/properties.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "ten_runtime/msg/cmd/custom/cmd.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/value/value.h"

bool ten_msgpack_cmd_serialize_through_json(ten_shared_ptr_t *self,
                                            msgpack_packer *pck,
                                            ten_error_t *err) {
  TEN_ASSERT(self && ten_cmd_base_check_integrity(self), "Invalid argument.");

  bool result = true;
  ten_json_t *json = NULL;
  const char *json_str = NULL;
  ten_shared_ptr_t *custom_cmd = NULL;

  json = ten_msg_to_json(self, err);
  if (!json) {
    result = false;
    goto done;
  }

  TEN_ASSERT(ten_json_check_integrity(json), "Invalid argument.");

  bool must_free = false;
  json_str = ten_json_to_string(json, NULL, &must_free);
  if (!json_str) {
    result = false;
    goto done;
  }
  TEN_ASSERT(json_str, "Invalid argument.");

  custom_cmd = ten_cmd_custom_create(TEN_STR_SPECIAL_CMD_FOR_SERIALIZATION);
  TEN_ASSERT(custom_cmd && ten_cmd_base_check_integrity(custom_cmd),
             "Invalid argument.");

  ten_msg_set_property(custom_cmd, TEN_STR_MSGPACK_SERIALIZATION_HACK,
                       ten_value_create_string(json_str), NULL);

  if (!ten_msgpack_msg_serialize(custom_cmd, pck, err)) {
    result = false;
  }

done:
  if (json_str && must_free) {
    TEN_FREE(json_str);
  }
  if (custom_cmd) {
    ten_shared_ptr_destroy(custom_cmd);
  }
  if (json) {
    ten_json_destroy(json);
  }

  return result;
}

ten_shared_ptr_t *ten_msgpack_cmd_deserialize_through_json(
    ten_shared_ptr_t *msg) {
  TEN_ASSERT(msg && ten_msg_check_integrity(msg), "Invalid argument.");

  if (ten_msg_get_type(msg) == TEN_MSG_TYPE_CMD &&
      ten_c_string_is_equal(ten_msg_get_name(msg),
                            TEN_STR_SPECIAL_CMD_FOR_SERIALIZATION) &&
      ten_msg_is_property_exist(msg, TEN_STR_MSGPACK_SERIALIZATION_HACK,
                                NULL)) {
    const char *json_str = ten_value_peek_c_str(
        ten_msg_peek_property(msg, TEN_STR_MSGPACK_SERIALIZATION_HACK, NULL));

    ten_json_t *json = ten_json_from_string(json_str, NULL);
    TEN_ASSERT(ten_json_check_integrity(json), "Invalid argument.");

    ten_shared_ptr_t *original_msg = ten_msg_create_from_json(json, NULL);
    TEN_ASSERT(ten_msg_check_integrity(original_msg), "Invalid argument.");

    ten_shared_ptr_destroy(msg);
    ten_json_destroy(json);

    return original_msg;
  }

  return msg;
}

void ten_msgpack_cmd_base_hdr_serialize(ten_msg_t *self, msgpack_packer *pck) {
  TEN_ASSERT(
      self && ten_raw_cmd_base_check_integrity((ten_cmd_base_t *)self) && pck,
      "Invalid argument.");

  for (size_t i = 0; i < ten_cmd_base_fields_info_size; ++i) {
    ten_msg_field_serialize_func_t serialize =
        ten_cmd_base_fields_info[i].serialize;
    if (serialize) {
      serialize(self, pck);
    }
  }
}

bool ten_msgpack_cmd_base_hdr_deserialize(ten_msg_t *self,
                                          msgpack_unpacker *unpacker,
                                          msgpack_unpacked *unpacked) {
  TEN_ASSERT(self && ten_raw_cmd_base_check_integrity((ten_cmd_base_t *)self) &&
                 unpacker && unpacked,
             "Invalid argument.");

  ten_cmd_base_t *raw_cmd = (ten_cmd_base_t *)self;
  TEN_ASSERT(ten_raw_cmd_base_check_integrity(raw_cmd), "Invalid argument.");

  for (size_t i = 0; i < ten_cmd_base_fields_info_size; ++i) {
    ten_msg_field_deserialize_func_t deserialize =
        ten_cmd_base_fields_info[i].deserialize;
    if (deserialize) {
      if (!deserialize(self, unpacker, unpacked)) {
        return false;
      }
    }
  }

  return true;
}
