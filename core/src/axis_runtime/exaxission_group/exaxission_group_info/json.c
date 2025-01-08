//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension_group/extension_group_info/json.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/extension_group/extension_group_info/extension_group_info.h"
#include "axis_utils/container/list.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"

// NOLINTNEXTLINE(misc-no-recursion)
axis_json_t *axis_extension_group_info_to_json(axis_extension_group_info_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_group_info_check_integrity(self),
             "Should not happen.");

  axis_json_t *info = axis_json_create_object();
  axis_ASSERT(info, "Should not happen.");

  axis_json_t *type = axis_json_create_string(axis_STR_EXTENSION_GROUP);
  axis_ASSERT(type, "Should not happen.");
  axis_json_object_set_new(info, axis_STR_TYPE, type);

  axis_json_t *name = axis_json_create_string(
      axis_string_get_raw_str(&self->loc.extension_group_name));
  axis_ASSERT(name, "Should not happen.");
  axis_json_object_set_new(info, axis_STR_NAME, name);

  axis_json_t *addon = axis_json_create_string(
      axis_string_get_raw_str(&self->extension_group_addon_name));
  axis_ASSERT(addon, "Should not happen.");
  axis_json_object_set_new(info, axis_STR_ADDON, addon);

  axis_json_t *app_uri =
      axis_json_create_string(axis_string_get_raw_str(&self->loc.app_uri));
  axis_ASSERT(app_uri, "Should not happen.");
  axis_json_object_set_new(info, axis_STR_APP, app_uri);

  if (self->property) {
    axis_json_object_set_new(info, axis_STR_PROPERTY,
                            axis_value_to_json(self->property));
  }

  return info;
}

axis_shared_ptr_t *axis_extension_group_info_from_json(
    axis_json_t *json, axis_list_t *extension_groups_info, axis_error_t *err) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Should not happen.");
  axis_ASSERT(
      extension_groups_info && axis_list_check_integrity(extension_groups_info),
      "Should not happen.");

  const char *type = axis_json_object_peek_string(json, axis_STR_TYPE);
  if (!type || !strlen(type) || strcmp(type, axis_STR_EXTENSION_GROUP) != 0) {
    axis_ASSERT(0, "Invalid extension group info.");
    return NULL;
  }

  const char *app_uri = axis_json_object_peek_string(json, axis_STR_APP);
  const char *graph_id = axis_json_object_peek_string(json, axis_STR_GRAPH);
  const char *addon_name = axis_json_object_peek_string(json, axis_STR_ADDON);
  const char *instance_name = axis_json_object_peek_string(json, axis_STR_NAME);

  axis_shared_ptr_t *self = get_extension_group_info_in_extension_groups_info(
      extension_groups_info, app_uri, graph_id, addon_name, instance_name, NULL,
      err);
  if (!self) {
    return NULL;
  }

  axis_extension_group_info_t *extension_group_info =
      axis_shared_ptr_get_data(self);
  axis_ASSERT(axis_extension_group_info_check_integrity(extension_group_info),
             "Should not happen.");

  // Parse 'prop'
  axis_json_t *props_json = axis_json_object_peek(json, axis_STR_PROPERTY);
  if (props_json) {
    if (!axis_json_is_object(props_json)) {
      // Indicates an error.
      axis_ASSERT(0,
                 "Failed to parse 'prop' in 'start_graph' command, it's not an "
                 "object.");
      return NULL;
    }

    axis_value_object_merge_with_json(extension_group_info->property,
                                     props_json);
  }

  return self;
}
