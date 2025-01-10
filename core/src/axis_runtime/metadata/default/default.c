//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/metadata/metadata.h"
#include "include_internal/axis_runtime/metadata/metadata_info.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/path.h"
#include "axis_utils/macro/mark.h"

void axis_set_default_manifest_info(const char *base_dir,
                                   axis_metadata_info_t *manifest,
                                   axis_UNUSED axis_error_t *err) {
  axis_ASSERT(manifest, "Should not happen.");

  if (!base_dir || !strlen(base_dir)) {
    const char *instance_name =
        axis_env_get_attached_instance_name(manifest->belonging_to, true);
    axis_LOGI(
        "Skip the loading of manifest.json because the base_dir of %s is "
        "missing.",
        instance_name);
    return;
  }

  axis_string_t manifest_json_file_path;
  axis_string_init_formatted(&manifest_json_file_path, "%s/manifest.json",
                            base_dir);
  axis_path_to_system_flavor(&manifest_json_file_path);
  if (axis_path_exists(axis_string_get_raw_str(&manifest_json_file_path))) {
    axis_metadata_info_set(manifest, axis_METADATA_JSON_FILENAME,
                          axis_string_get_raw_str(&manifest_json_file_path));
  }
  axis_string_deinit(&manifest_json_file_path);
}

void axis_set_default_property_info(const char *base_dir,
                                   axis_metadata_info_t *property,
                                   axis_UNUSED axis_error_t *err) {
  axis_ASSERT(property, "Should not happen.");

  if (!base_dir || !strlen(base_dir)) {
    const char *instance_name =
        axis_env_get_attached_instance_name(property->belonging_to, true);
    axis_LOGI(
        "Skip the loading of property.json because the base_dir of %s is "
        "missing.",
        instance_name);
    return;
  }

  axis_string_t property_json_file_path;
  axis_string_init_formatted(&property_json_file_path, "%s/property.json",
                            base_dir);
  axis_path_to_system_flavor(&property_json_file_path);
  if (axis_path_exists(axis_string_get_raw_str(&property_json_file_path))) {
    axis_metadata_info_set(property, axis_METADATA_JSON_FILENAME,
                          axis_string_get_raw_str(&property_json_file_path));
  }
  axis_string_deinit(&property_json_file_path);
}
