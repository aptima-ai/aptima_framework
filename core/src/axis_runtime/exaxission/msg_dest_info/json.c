//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension/msg_dest_info/json.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/common/loc.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension/extension_info/extension_info.h"
#include "include_internal/axis_runtime/extension/extension_info/json.h"
#include "include_internal/axis_runtime/extension/msg_dest_info/msg_dest_info.h"
#include "include_internal/axis_runtime/msg_conversion/msg_and_result_conversion.h"
#include "include_internal/axis_runtime/msg_conversion/msg_conversion_context.h"
#include "axis_utils/container/list.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"

axis_json_t *axis_msg_dest_info_to_json(axis_msg_dest_info_t *self,
                                      axis_extension_info_t *src_extension_info,
                                      axis_error_t *err) {
  axis_ASSERT(self && axis_msg_dest_info_check_integrity(self),
             "Should not happen.");

  axis_json_t *json = axis_json_create_object();
  axis_ASSERT(json, "Should not happen.");
  axis_json_object_set_new(
      json, axis_STR_NAME,
      axis_json_create_string(axis_string_get_raw_str(&self->name)));

  axis_json_t *dests_json = axis_json_create_array();
  axis_ASSERT(dests_json, "Should not happen.");
  axis_json_object_set_new(json, axis_STR_DEST, dests_json);

  axis_list_foreach (&self->dest, iter) {
    axis_weak_ptr_t *dest = axis_smart_ptr_listnode_get(iter.node);
    axis_ASSERT(dest, "Invalid argument.");

    axis_extension_info_t *extension_info = axis_smart_ptr_get_data(dest);

    axis_json_t *dest_json = axis_json_create_object();
    axis_ASSERT(dest_json, "Should not happen.");

    axis_json_object_set_new(dest_json, axis_STR_APP,
                            axis_json_create_string(axis_string_get_raw_str(
                                &extension_info->loc.app_uri)));

    axis_json_object_set_new(dest_json, axis_STR_GRAPH,
                            axis_json_create_string(axis_string_get_raw_str(
                                &extension_info->loc.graph_id)));

    axis_json_t *extension_group_json = axis_json_create_string(
        axis_string_get_raw_str(&extension_info->loc.extension_group_name));
    axis_ASSERT(extension_group_json, "Should not happen.");
    axis_json_object_set_new(dest_json, axis_STR_EXTENSION_GROUP,
                            extension_group_json);

    axis_json_t *extension_json = axis_json_create_string(
        axis_string_get_raw_str(&extension_info->loc.extension_name));
    axis_ASSERT(extension_json, "Should not happen.");
    axis_json_object_set_new(dest_json, axis_STR_EXTENSION, extension_json);

    axis_list_foreach (&extension_info->msg_conversion_contexts,
                      msg_conversion_iter) {
      axis_msg_conversion_context_t *msg_conversion =
          axis_ptr_listnode_get(msg_conversion_iter.node);
      axis_ASSERT(msg_conversion &&
                     axis_msg_conversion_context_check_integrity(msg_conversion),
                 "Should not happen.");

      if (axis_loc_is_equal(&src_extension_info->loc,
                           &msg_conversion->src_loc) &&
          axis_string_is_equal(&msg_conversion->msg_name, &self->name)) {
        axis_json_t *msg_and_result_json = axis_msg_and_result_conversion_to_json(
            msg_conversion->msg_and_result_conversion, err);
        if (!msg_and_result_json) {
          axis_json_destroy(json);
          return NULL;
        }

        axis_json_object_set_new(dest_json, axis_STR_MSG_CONVERSION,
                                msg_and_result_json);
      }
    }

    axis_json_array_append_new(dests_json, dest_json);
  }

  return json;
}
