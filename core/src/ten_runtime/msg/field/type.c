//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/msg/field/type.h"

#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/macro/mark.h"

bool ten_raw_msg_type_from_json(ten_msg_t *self, ten_json_t *json,
                                TEN_UNUSED ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");

  ten_json_t *ten_json =
      ten_json_object_peek_object(json, TEN_STR_UNDERLINE_TEN);
  if (!ten_json) {
    return true;
  }

  ten_json_t *type_json = ten_json_object_peek(ten_json, TEN_STR_TYPE);
  if (!type_json) {
    return true;
  }

  ten_json_t *name_json = ten_json_object_peek(ten_json, TEN_STR_NAME);

  self->type = ten_msg_type_from_type_and_name_string(
      ten_json_peek_string_value(type_json),
      name_json ? ten_json_peek_string_value(name_json) : NULL);

  return true;
}

bool ten_raw_msg_type_to_json(ten_msg_t *self, ten_json_t *json,
                              ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) && json,
             "Should not happen.");

  ten_json_t *ten_json =
      ten_json_object_peek_object_forcibly(json, TEN_STR_UNDERLINE_TEN);
  TEN_ASSERT(ten_json, "Should not happen.");

  ten_json_t *type_json = ten_json_create_string(
      ten_msg_type_to_string(ten_raw_msg_get_type(self)));
  TEN_ASSERT(type_json, "Should not happen.");

  ten_json_object_set_new(ten_json, TEN_STR_TYPE, type_json);

  return true;
}

void ten_raw_msg_type_copy(ten_msg_t *self, ten_msg_t *src,
                           ten_list_t *excluded_field_ids) {
  TEN_ASSERT(src && ten_raw_msg_check_integrity(src), "Should not happen.");
  self->type = src->type;
}
