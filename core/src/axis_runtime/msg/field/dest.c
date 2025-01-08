//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg/field/dest.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/common/loc.h"
#include "include_internal/axis_runtime/extension/extension_info/extension_info.h"
#include "include_internal/axis_runtime/msg/loop_fields.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/value/value.h"

void axis_raw_msg_dest_copy(axis_msg_t *self, axis_msg_t *src,
                           axis_UNUSED axis_list_t *excluded_field_ids) {
  axis_ASSERT(src && axis_raw_msg_check_integrity(src), "Should not happen.");

  axis_list_clear(&self->dest_loc);
  axis_list_foreach (&src->dest_loc, iter) {
    axis_loc_t *dest_loc = axis_ptr_listnode_get(iter.node);
    axis_loc_t *cloned_dest = axis_loc_clone(dest_loc);
    axis_list_push_ptr_back(&self->dest_loc, cloned_dest,
                           (axis_ptr_listnode_destroy_func_t)axis_loc_destroy);
  }
}

bool axis_raw_msg_dest_process(axis_msg_t *self,
                              axis_raw_msg_process_one_field_func_t cb,
                              void *user_data, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  axis_list_t dest_value_list = axis_LIST_INIT_VAL;
  axis_list_foreach (&self->dest_loc, iter) {
    axis_loc_t *loc = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(loc, "Should not happen.");

    axis_value_t *loc_value = axis_loc_to_value(loc);
    axis_ASSERT(loc_value, "Should not happen.");

    axis_list_push_ptr_back(&dest_value_list, loc_value,
                           (axis_ptr_listnode_destroy_func_t)axis_value_destroy);
  }

  axis_value_t *dest_value = axis_value_create_array_with_move(&dest_value_list);
  axis_ASSERT(dest_value, "Should not happen.");

  axis_msg_field_process_data_t dest_field;
  axis_msg_field_process_data_init(&dest_field, axis_STR_DEST, dest_value, false);

  bool rc = cb(self, &dest_field, user_data, err);

  if (dest_field.value_is_changed_after_process) {
    axis_list_clear(&self->dest_loc);

    axis_value_array_foreach(dest_field.field_value, iter) {
      axis_value_t *loc_value = axis_ptr_listnode_get(iter.node);
      axis_ASSERT(loc_value, "Should not happen.");

      axis_list_push_ptr_back(&self->dest_loc,
                             axis_loc_create_from_value(loc_value),
                             (axis_ptr_listnode_destroy_func_t)axis_loc_destroy);
    }
  }

  axis_value_destroy(dest_value);
  axis_list_clear(&dest_value_list);

  return rc;
}
