//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/msg/field/dest.h"

#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/common/loc.h"
#include "include_internal/ten_runtime/extension/extension_info/extension_info.h"
#include "include_internal/ten_runtime/msg/loop_fields.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_ptr.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"
#include "ten_utils/value/value.h"

// Set dest information of msg from JSON content if there are any.
bool ten_raw_msg_dest_from_json(ten_msg_t *self, ten_json_t *json,
                                TEN_UNUSED ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");

  ten_json_t *ten_json =
      ten_json_object_peek_object(json, TEN_STR_UNDERLINE_TEN);
  if (!ten_json) {
    return true;
  }

  ten_json_t *dest_array_json =
      ten_json_object_peek_array(ten_json, TEN_STR_DEST);
  if (!dest_array_json) {
    return true;
  }

  if (ten_json_array_get_size(dest_array_json)) {
    // If there is destination information in JSON, then use it to replace the
    // destination information in the origin msg.
    ten_raw_msg_clear_dest(self);

    size_t i = 0;
    ten_json_t *dest_json = NULL;
    ten_json_array_foreach(dest_array_json, i, dest_json) {
      ten_raw_msg_add_dest(
          self, ten_json_object_peek_string(dest_json, TEN_STR_APP),
          ten_json_object_peek_string(dest_json, TEN_STR_GRAPH),
          ten_json_object_peek_string(dest_json, TEN_STR_EXTENSION_GROUP),
          ten_json_object_peek_string(dest_json, TEN_STR_EXTENSION));
    }
  }

  return true;
}

bool ten_raw_msg_dest_to_json(ten_msg_t *self, ten_json_t *json,
                              ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) && json,
             "Should not happen.");
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");

  ten_json_t *ten_json =
      ten_json_object_peek_object_forcibly(json, TEN_STR_UNDERLINE_TEN);
  ten_json_t *dests_json =
      ten_json_object_peek_array_forcibly(ten_json, TEN_STR_DEST);

  ten_list_foreach (&self->dest_loc, iter) {
    ten_loc_t *dest = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(dest, "Should not happen.");

    ten_json_t *dest_json = ten_loc_to_json(dest);
    TEN_ASSERT(dest_json, "Should not happen.");

    ten_json_array_append_new(dests_json, dest_json);
  }

  return true;
}

void ten_raw_msg_dest_copy(ten_msg_t *self, ten_msg_t *src,
                           TEN_UNUSED ten_list_t *excluded_field_ids) {
  TEN_ASSERT(src && ten_raw_msg_check_integrity(src), "Should not happen.");

  ten_list_clear(&self->dest_loc);
  ten_list_foreach (&src->dest_loc, iter) {
    ten_loc_t *dest_loc = ten_ptr_listnode_get(iter.node);
    ten_loc_t *cloned_dest = ten_loc_clone(dest_loc);
    ten_list_push_ptr_back(&self->dest_loc, cloned_dest,
                           (ten_ptr_listnode_destroy_func_t)ten_loc_destroy);
  }
}

bool ten_raw_msg_dest_process(ten_msg_t *self,
                              ten_raw_msg_process_one_field_func_t cb,
                              void *user_data, ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");

  ten_list_t dest_value_list = TEN_LIST_INIT_VAL;
  ten_list_foreach (&self->dest_loc, iter) {
    ten_loc_t *loc = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(loc, "Should not happen.");

    ten_value_t *loc_value = ten_loc_to_value(loc);
    TEN_ASSERT(loc_value, "Should not happen.");

    ten_list_push_ptr_back(&dest_value_list, loc_value,
                           (ten_ptr_listnode_destroy_func_t)ten_value_destroy);
  }

  ten_value_t *dest_value = ten_value_create_array_with_move(&dest_value_list);
  TEN_ASSERT(dest_value, "Should not happen.");

  ten_msg_field_process_data_t dest_field;
  ten_msg_field_process_data_init(&dest_field, TEN_STR_DEST, dest_value, false);

  bool rc = cb(self, &dest_field, user_data, err);

  if (dest_field.value_is_changed_after_process) {
    ten_list_clear(&self->dest_loc);

    ten_value_array_foreach(dest_field.field_value, iter) {
      ten_value_t *loc_value = ten_ptr_listnode_get(iter.node);
      TEN_ASSERT(loc_value, "Should not happen.");

      ten_list_push_ptr_back(&self->dest_loc,
                             ten_loc_create_from_value(loc_value),
                             (ten_ptr_listnode_destroy_func_t)ten_loc_destroy);
    }
  }

  ten_value_destroy(dest_value);
  ten_list_clear(&dest_value_list);

  return rc;
}
