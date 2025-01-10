//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension/msg_dest_info/value.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/extension/extension_info/extension_info.h"
#include "include_internal/axis_runtime/extension/extension_info/value.h"
#include "include_internal/axis_runtime/extension/msg_dest_info/msg_dest_info.h"
#include "include_internal/axis_runtime/msg_conversion/msg_and_result_conversion.h"
#include "include_internal/axis_runtime/msg_conversion/msg_conversion_context.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/log/log.h"

axis_value_t *axis_msg_dest_info_to_value(
    axis_msg_dest_info_t *self, axis_extension_info_t *src_extension_info,
    axis_error_t *err) {
  axis_ASSERT(self && axis_msg_dest_info_check_integrity(self),
             "Should not happen.");

  axis_list_t value_object_kv_list = axis_LIST_INIT_VAL;

  axis_list_push_ptr_back(
      &value_object_kv_list,
      axis_value_kv_create(
          axis_STR_NAME,
          axis_value_create_string(axis_string_get_raw_str(&self->name))),
      (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);

  axis_list_t dests_list = axis_LIST_INIT_VAL;

  axis_list_foreach (&self->dest, iter) {
    axis_weak_ptr_t *dest = axis_smart_ptr_listnode_get(iter.node);
    axis_ASSERT(dest, "Invalid argument.");

    axis_extension_info_t *extension_info = axis_smart_ptr_get_data(dest);
    axis_ASSERT(extension_info, "Should not happen.");

    axis_list_t dest_kv_list = axis_LIST_INIT_VAL;

    axis_list_push_ptr_back(
        &dest_kv_list,
        axis_value_kv_create(axis_STR_APP,
                            axis_value_create_string(axis_string_get_raw_str(
                                &extension_info->loc.app_uri))),
        (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);

    axis_list_push_ptr_back(
        &dest_kv_list,
        axis_value_kv_create(axis_STR_GRAPH,
                            axis_value_create_string(axis_string_get_raw_str(
                                &extension_info->loc.graph_id))),
        (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);

    axis_list_push_ptr_back(
        &dest_kv_list,
        axis_value_kv_create(axis_STR_EXTENSION_GROUP,
                            axis_value_create_string(axis_string_get_raw_str(
                                &extension_info->loc.extension_group_name))),
        (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);

    axis_list_push_ptr_back(
        &dest_kv_list,
        axis_value_kv_create(axis_STR_EXTENSION,
                            axis_value_create_string(axis_string_get_raw_str(
                                &extension_info->loc.extension_name))),
        (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);

    bool found = false;

    axis_list_foreach (&extension_info->msg_conversion_contexts,
                      msg_conversion_iter) {
      axis_msg_conversion_context_t *msg_conversion_context =
          axis_ptr_listnode_get(msg_conversion_iter.node);
      axis_ASSERT(
          msg_conversion_context && axis_msg_conversion_context_check_integrity(
                                        msg_conversion_context),
          "Should not happen.");

      if (axis_loc_is_equal(&src_extension_info->loc,
                           &msg_conversion_context->src_loc) &&
          axis_string_is_equal(&msg_conversion_context->msg_name, &self->name)) {
        axis_ASSERT(found == false, "Should not happen.");
        found = true;

        axis_value_t *msg_and_result_conversion_operation_value =
            axis_msg_and_result_conversion_to_value(
                msg_conversion_context->msg_and_result_conversion, err);

        if (!msg_and_result_conversion_operation_value) {
          axis_LOGE("Failed to convert msg_and_result_conversion_operation.");
          continue;
        }

        axis_list_push_ptr_back(
            &dest_kv_list,
            axis_value_kv_create(axis_STR_MSG_CONVERSION,
                                msg_and_result_conversion_operation_value),
            (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);
      }
    }

    axis_value_t *dest_value = axis_value_create_object_with_move(&dest_kv_list);
    axis_list_push_ptr_back(&dests_list, dest_value,
                           (axis_ptr_listnode_destroy_func_t)axis_value_destroy);
    axis_list_clear(&dest_kv_list);
  }

  axis_value_t *dests_value = axis_value_create_array_with_move(&dests_list);
  axis_list_push_ptr_back(&value_object_kv_list,
                         axis_value_kv_create(axis_STR_DEST, dests_value),
                         (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);
  axis_value_t *value = axis_value_create_object_with_move(&value_object_kv_list);
  axis_list_clear(&value_object_kv_list);

  return value;
}

// Parse the following snippet.
//
// ------------------------
// "name": "...",
// "dest": [{
//   "app": "...",
//   "extension_group": "...",
//   "extension": "...",
//   "msg_conversion": {
//   }
// }]
// ------------------------
axis_shared_ptr_t *axis_msg_dest_info_from_value(
    axis_value_t *value, axis_list_t *extensions_info,
    axis_extension_info_t *src_extension_info, axis_error_t *err) {
  axis_ASSERT(value && extensions_info, "Should not happen.");
  axis_ASSERT(src_extension_info,
             "src_extension must be specified in this case.");

  axis_msg_dest_info_t *self = NULL;

  // "name": "...",
  axis_value_t *name_value = axis_value_object_peek(value, axis_STR_NAME);

  const char *name = "";
  if (name_value) {
    name = axis_value_peek_raw_str(name_value, err);
  }

  self = axis_msg_dest_info_create(name);
  axis_ASSERT(self, "Should not happen.");

  // "dest": [{
  //   "app": "...",
  //   "extension_group": "...",
  //   "extension": "...",
  //   "msg_conversion": {
  //   }
  // }]
  axis_value_t *dests_value = axis_value_object_peek(value, axis_STR_DEST);
  if (dests_value) {
    if (!axis_value_is_array(dests_value)) {
      goto error;
    }

    axis_value_array_foreach(dests_value, iter) {
      axis_value_t *dest_value = axis_ptr_listnode_get(iter.node);
      if (!axis_value_is_object(dest_value)) {
        goto error;
      }

      axis_shared_ptr_t *dest =
          axis_extension_info_parse_connection_dest_part_from_value(
              dest_value, extensions_info, src_extension_info, name, err);
      if (!dest) {
        goto error;
      }

      // We need to use weak_ptr here to prevent the circular shared_ptr problem
      // in the case of loop graph.
      axis_weak_ptr_t *weak_dest = axis_weak_ptr_create(dest);
      axis_list_push_smart_ptr_back(&self->dest, weak_dest);
      axis_weak_ptr_destroy(weak_dest);
    }
  }

  goto done;

error:
  if (self) {
    axis_msg_dest_info_destroy(self);
    self = NULL;
  }

done:
  if (self) {
    return axis_shared_ptr_create(self, axis_msg_dest_info_destroy);
  } else {
    return NULL;
  }
}
