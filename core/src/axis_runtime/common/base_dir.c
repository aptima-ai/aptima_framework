//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/common/constant_str.h"
#include "axis_utils/lib/file.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/path.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/memory.h"

// Traverse up through the parent folders, searching for a folder containing a
// manifest.json with the specified type and name.
axis_string_t *axis_find_base_dir(const char *start_path, const char *addon_type,
                                const char *addon_name) {
  axis_ASSERT(start_path && addon_type && strlen(addon_type),
             "Invalid argument.");

  axis_string_t *parent_path = axis_string_create_formatted("%s", start_path);
  if (!parent_path) {
    axis_ASSERT(0, "Should not happen.");
    return NULL;
  }

  while (axis_path_is_dir(parent_path)) {
    axis_string_t *manifest_path = axis_string_clone(parent_path);
    axis_string_append_formatted(manifest_path, "/manifest.json");

    if (axis_path_exists(axis_string_get_raw_str(manifest_path))) {
      // Read manifest.json, and check if there is a top-level "type" field with
      // a value specified in `addon_type` parameter.
      const char *manifest_content =
          axis_file_read(axis_string_get_raw_str(manifest_path));
      if (manifest_content) {
        axis_json_t *json = axis_json_from_string(manifest_content, NULL);
        if (json) {
          do {
            const char *type_in_manifest =
                axis_json_object_peek_string(json, axis_STR_TYPE);
            if (!type_in_manifest) {
              break;
            }
            if (strcmp(type_in_manifest, addon_type) != 0) {
              break;
            }

            if (addon_name && strlen(addon_name)) {
              const char *name_in_manifest =
                  axis_json_object_peek_string(json, axis_STR_NAME);
              if (!name_in_manifest) {
                break;
              }
              if (strcmp(name_in_manifest, addon_name) != 0) {
                break;
              }
            }

            axis_string_t *base_dir = axis_path_realpath(parent_path);
            axis_path_to_system_flavor(base_dir);

            axis_json_destroy(json);
            axis_FREE(manifest_content);
            axis_string_destroy(manifest_path);
            axis_string_destroy(parent_path);

            return base_dir;
          } while (0);

          axis_json_destroy(json);
        }

        axis_FREE(manifest_content);
      }
    }

    axis_string_destroy(manifest_path);

    axis_string_t *next_parent = axis_path_get_dirname(parent_path);
    if (!next_parent || axis_string_is_empty(next_parent) ||
        axis_string_is_equal(parent_path, next_parent)) {
      // No more parent folders.
      axis_string_destroy(parent_path);
      if (next_parent) {
        axis_string_destroy(next_parent);
      }
      return NULL;
    }

    axis_string_destroy(parent_path);
    parent_path = next_parent;
  }

  return NULL;
}
