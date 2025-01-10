//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/app/metadata.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/app/predefined_graph.h"
#include "include_internal/axis_runtime/app/axis_property.h"
#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/common/log.h"
#include "include_internal/axis_runtime/extension/extension_info/extension_info.h"
#include "include_internal/axis_runtime/metadata/manifest.h"
#include "include_internal/axis_utils/log/log.h"
#include "include_internal/axis_utils/log/output.h"
#include "axis_runtime/app/app.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node_ptr.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_get.h"
#include "axis_utils/value/value_is.h"
#include "axis_utils/value/value_kv.h"

// Retrieve those property fields that are reserved for the APTIMA runtime
// under the 'aptima' namespace.
axis_value_t *axis_app_get_axis_namespace_properties(axis_app_t *self) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");

  return axis_value_object_peek(&self->property, axis_STR_UNDERLINE_TEN);
}

bool axis_app_init_one_event_loop_per_engine(axis_app_t *self,
                                            axis_value_t *value) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");
  axis_ASSERT(value && axis_value_check_integrity(value), "Should not happen.");

  if (!axis_value_is_bool(value)) {
    axis_LOGE("Invalid value type for property: %s",
             axis_STR_ONE_EVENT_LOOP_PER_ENGINE);
    return false;
  }

  axis_error_t err;
  axis_error_init(&err);

  self->one_event_loop_per_engine = axis_value_get_bool(value, &err);

  axis_error_deinit(&err);

  return true;
}

bool axis_app_init_long_running_mode(axis_app_t *self, axis_value_t *value) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");
  axis_ASSERT(value && axis_value_check_integrity(value), "Should not happen.");

  if (!axis_value_is_bool(value)) {
    axis_LOGE("Invalid value type for property: %s", axis_STR_LONG_RUNNING_MODE);
    return false;
  }

  axis_error_t err;
  axis_error_init(&err);

  self->long_running_mode = axis_value_get_bool(value, &err);

  axis_error_deinit(&err);

  return true;
}

bool axis_app_init_uri(axis_app_t *self, axis_value_t *value) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");
  axis_ASSERT(value && axis_value_check_integrity(value), "Should not happen.");

  if (!axis_value_is_string(value)) {
    axis_LOGW("Invalid uri.");
    return false;
  }

  axis_string_t default_url;
  axis_string_init_formatted(&default_url, axis_STR_LOCALHOST);

  const char *url_str = axis_value_peek_raw_str(value, NULL)
                            ? axis_value_peek_raw_str(value, NULL)
                            : axis_string_get_raw_str(&default_url);

  axis_string_set_from_c_str(&self->uri, url_str, strlen(url_str));

  axis_string_deinit(&default_url);

  return true;
}

bool axis_app_init_log_level(axis_app_t *self, axis_value_t *value) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");
  axis_ASSERT(value && axis_value_check_integrity(value), "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  axis_log_global_set_output_level(axis_value_get_int64(value, &err));

  axis_error_deinit(&err);

  return true;
}

bool axis_app_init_log_file(axis_app_t *self, axis_value_t *value) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");
  axis_ASSERT(value && axis_value_check_integrity(value), "Should not happen.");

  axis_string_t log_file;
  axis_string_init(&log_file);

  axis_string_init_from_c_str(&log_file, axis_value_peek_raw_str(value, NULL),
                             strlen(axis_value_peek_raw_str(value, NULL)));

  if (!axis_string_is_empty(&log_file)) {
    axis_log_global_set_output_to_file(axis_string_get_raw_str(&log_file));
  }

  axis_string_deinit(&log_file);

  return true;
}

static bool axis_app_determine_axis_namespace_properties(
    axis_app_t *self, axis_value_t *axis_namespace_properties) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");
  axis_ASSERT(
      axis_namespace_properties && axis_value_is_object(axis_namespace_properties),
      "Should not happen.");

  axis_value_object_foreach(axis_namespace_properties, iter) {
    axis_value_kv_t *prop_kv = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(prop_kv && axis_value_kv_check_integrity(prop_kv),
               "Should not happen.");

    axis_string_t *item_key = &prop_kv->key;
    axis_value_t *item_value = prop_kv->value;

    for (int i = 0; i < axis_app_axis_namespace_prop_info_list_size; ++i) {
      const axis_app_axis_namespace_prop_info_t *prop_info =
          &axis_app_axis_namespace_prop_info_list[i];
      if (axis_string_is_equal_c_str(item_key, prop_info->name)) {
        bool rc = prop_info->init_from_value(self, item_value);
        if (rc) {
          break;
        } else {
          axis_LOGW("Failed to init property: %s", prop_info->name);
          return false;
        }
      }
    }
  }

  return true;
}

bool axis_app_handle_axis_namespace_properties(axis_app_t *self) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");

  axis_value_t *axis_namespace_properties =
      axis_app_get_axis_namespace_properties(self);
  if (axis_namespace_properties == NULL) {
    return true;
  }

  axis_ASSERT(
      axis_namespace_properties && axis_value_is_object(axis_namespace_properties),
      "Should not happen.");

  // Set default value for app properties and global log level.
  self->one_event_loop_per_engine = false;
  self->long_running_mode = false;

  // First, set the log-related configuration to default values. This way, if
  // there are no log-related properties under the `aptima` namespace, the default
  // values will be used.
  axis_log_global_set_output_to_stderr();
  axis_log_global_set_output_level(DEFAULT_LOG_OUTPUT_LEVEL);

  if (!axis_app_determine_axis_namespace_properties(self,
                                                  axis_namespace_properties)) {
    return false;
  }

  return true;
}

void axis_app_handle_metadata(axis_app_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_app_check_integrity(self, true), "Invalid use of app %p.",
             self);

  // Load custom APTIMA app metadata.
  axis_metadata_load(axis_app_on_configure, self->axis_env);
}
