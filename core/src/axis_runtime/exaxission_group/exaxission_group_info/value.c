//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension_group/extension_group_info/value.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/extension/extension_info/extension_info.h"
#include "include_internal/axis_runtime/extension_group/extension_group_info/extension_group_info.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value_merge.h"
#include "axis_utils/value/value_object.h"

axis_shared_ptr_t *axis_extension_group_info_from_value(
    axis_value_t *value, axis_list_t *extension_groups_info, axis_error_t *err) {
  axis_ASSERT(value && extension_groups_info, "Should not happen.");

  const char *app_uri = axis_value_object_peek_string(value, axis_STR_APP);
  const char *graph_id = axis_value_object_peek_string(value, axis_STR_GRAPH);
  const char *addon_name = axis_value_object_peek_string(value, axis_STR_ADDON);
  const char *instance_name = axis_value_object_peek_string(value, axis_STR_NAME);

  axis_shared_ptr_t *self = get_extension_group_info_in_extension_groups_info(
      extension_groups_info, app_uri, graph_id, addon_name, instance_name, NULL,
      err);
  if (!self) {
    return NULL;
  }

  axis_extension_group_info_t *extension_info = axis_shared_ptr_get_data(self);
  axis_ASSERT(axis_extension_group_info_check_integrity(extension_info),
             "Should not happen.");

  // Parse 'prop'
  axis_value_t *props_value = axis_value_object_peek(value, axis_STR_PROPERTY);
  if (props_value) {
    if (!axis_value_is_object(props_value)) {
      // Indicates an error.
      axis_ASSERT(0,
                 "Failed to parse 'prop' in 'start_graph' command, it's not an "
                 "object.");
      return NULL;
    }

    axis_value_object_merge_with_clone(extension_info->property, props_value);
  }

  return NULL;
}
