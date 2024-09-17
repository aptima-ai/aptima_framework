//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_runtime/ten_config.h"

#include "include_internal/ten_runtime/common/constant_str.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/lib/file.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/lib/path.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/memory.h"

// Find the parent folder containing "manifest.json" with "type": "app"
void ten_app_find_base_dir(ten_string_t *start_path, ten_string_t **app_path) {
  TEN_ASSERT(start_path && app_path, "Invalid argument.");

  ten_string_t *parent_path = ten_string_clone(start_path);
  if (!parent_path) {
    TEN_LOGE("Failed to clone string: %s", start_path->buf);
    TEN_ASSERT(0, "Should not happen.");
    return;
  }

  while (ten_path_is_dir(parent_path)) {
    ten_string_t *manifest_path = ten_string_clone(parent_path);
    ten_string_append_formatted(manifest_path, "/manifest.json");

    if (ten_path_exists(ten_string_get_raw_str(manifest_path))) {
      // Read manifest.json, and check if there is a top-level "type" field with
      // "app" value.
      const char *manifest_content =
          ten_file_read(ten_string_get_raw_str(manifest_path));
      if (manifest_content) {
        ten_json_t *json = ten_json_from_string(manifest_content, NULL);
        if (json) {
          const char *type = ten_json_object_peek_string(json, TEN_STR_TYPE);
          if (type && strcmp(type, TEN_STR_APP) == 0) {
            *app_path = ten_path_realpath(parent_path);

            ten_json_destroy(json);
            TEN_FREE(manifest_content);
            ten_string_destroy(manifest_path);
            ten_string_destroy(parent_path);
            break;
          }

          ten_json_destroy(json);
        }

        TEN_FREE(manifest_content);
      }
    }

    ten_string_destroy(manifest_path);

    ten_string_t *next_parent = ten_path_get_dirname(parent_path);
    if (ten_string_is_equal(parent_path, next_parent)) {
      // No more parent folders.
      ten_string_destroy(parent_path);
      return;
    }

    ten_string_destroy(parent_path);
    parent_path = next_parent;

    if (!parent_path || ten_string_is_empty(parent_path)) {
      TEN_LOGE("Failed to find the app path");
      TEN_ASSERT(0, "Should not happen.");
      if (parent_path) {
        ten_string_destroy(parent_path);
      }
      return;
    }
  }
}
