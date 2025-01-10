//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/extension/extension_addon_and_instance_name_pair.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/macro/check.h"

axis_extension_addon_and_instance_name_pair_t *
axis_extension_addon_and_instance_name_pair_create(
    const char *extension_addon_name, const char *extension_instance_name) {
  axis_extension_addon_and_instance_name_pair_t *self =
      (axis_extension_addon_and_instance_name_pair_t *)axis_MALLOC(
          sizeof(axis_extension_addon_and_instance_name_pair_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_string_init_formatted(&self->addon_name, "%s",
                            extension_addon_name ? extension_addon_name : "");
  axis_string_init_formatted(
      &self->instance_name, "%s",
      extension_instance_name ? extension_instance_name : "");

  return self;
}

void axis_extension_addon_and_instance_name_pair_destroy(
    axis_extension_addon_and_instance_name_pair_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_string_deinit(&self->addon_name);
  axis_string_deinit(&self->instance_name);

  axis_FREE(self);
}

void axis_extension_addon_and_instance_name_pair_to_json(
    axis_json_t *json, const char *key, axis_string_t *addon_name,
    axis_string_t *instance_name) {
  axis_ASSERT(json && key && addon_name && instance_name, "Should not happen.");

  if (axis_string_is_empty(addon_name)) {
    axis_json_t *extension_group_json =
        axis_json_create_string(axis_string_get_raw_str(instance_name));
    axis_ASSERT(extension_group_json, "Should not happen.");
    axis_json_object_set_new(json, key, extension_group_json);
  } else {
    axis_json_t *extension_group_json = axis_json_create_object();
    axis_json_object_set_new(
        extension_group_json, axis_STR_ADDON,
        axis_json_create_string(axis_string_get_raw_str(addon_name)));
    axis_json_object_set_new(
        extension_group_json, axis_STR_NAME,
        axis_json_create_string(axis_string_get_raw_str(instance_name)));
    axis_json_object_set_new(json, key, extension_group_json);
  }
}
