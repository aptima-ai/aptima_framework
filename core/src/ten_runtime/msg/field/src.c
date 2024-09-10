//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/msg/field/src.h"

#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/common/loc.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/macro/check.h"

bool ten_raw_msg_src_from_json(ten_msg_t *self, ten_json_t *json,
                               TEN_UNUSED ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");

  ten_json_t *ten_json =
      ten_json_object_peek_object(json, TEN_STR_UNDERLINE_TEN);
  if (!ten_json) {
    return true;
  }

  ten_json_t *src_json = ten_json_object_peek(ten_json, TEN_STR_SRC);
  if (!src_json) {
    return true;
  }

  ten_loc_init_from_json(&self->src_loc, src_json);

  return true;
}

bool ten_raw_msg_src_to_json(ten_msg_t *self, ten_json_t *json,
                             ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) && json,
             "Should not happen.");

  ten_json_t *ten_json =
      ten_json_object_peek_object_forcibly(json, TEN_STR_UNDERLINE_TEN);
  TEN_ASSERT(ten_json, "Should not happen.");

  ten_json_t *src_json = ten_loc_to_json(&self->src_loc);
  TEN_ASSERT(src_json, "Should not happen.");

  ten_json_object_set_new(ten_json, TEN_STR_SRC, src_json);

  return true;
}

void ten_raw_msg_src_copy(ten_msg_t *self, ten_msg_t *src,
                          ten_list_t *excluded_field_ids) {
  TEN_ASSERT(src && ten_raw_msg_check_integrity(src), "Should not happen.");
  ten_loc_init_from_loc(&self->src_loc, &src->src_loc);
}
