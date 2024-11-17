//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/app/metadata.h"

#include <stdbool.h>

#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/app/predefined_graph.h"
#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/common/log.h"
#include "include_internal/ten_runtime/extension/extension_info/extension_info.h"
#include "include_internal/ten_utils/log/log.h"
#include "include_internal/ten_utils/log/output.h"
#include "ten_runtime/app/app.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_node_ptr.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/value/value.h"
#include "ten_utils/value/value_get.h"
#include "ten_utils/value/value_is.h"
#include "ten_utils/value/value_kv.h"

// Retrieve those property fields that are reserved for the TEN runtime
// under the 'ten' namespace.
ten_value_t *ten_app_get_ten_namespace_properties(ten_app_t *self) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");

  return ten_value_object_peek(&self->property, TEN_STR_UNDERLINE_TEN);
}

bool ten_app_init_one_event_loop_per_engine(ten_app_t *self,
                                            ten_value_t *value) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");
  TEN_ASSERT(value && ten_value_check_integrity(value), "Should not happen.");

  if (!ten_value_is_bool(value)) {
    TEN_LOGE("Invalid value type for property: %s",
             TEN_STR_ONE_EVENT_LOOP_PER_ENGINE);
    return false;
  }

  ten_error_t err;
  ten_error_init(&err);

  self->one_event_loop_per_engine = ten_value_get_bool(value, &err);

  ten_error_deinit(&err);

  return true;
}

bool ten_app_init_long_running_mode(ten_app_t *self, ten_value_t *value) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");
  TEN_ASSERT(value && ten_value_check_integrity(value), "Should not happen.");

  if (!ten_value_is_bool(value)) {
    TEN_LOGE("Invalid value type for property: %s", TEN_STR_LONG_RUNNING_MODE);
    return false;
  }

  ten_error_t err;
  ten_error_init(&err);

  self->long_running_mode = ten_value_get_bool(value, &err);

  ten_error_deinit(&err);

  return true;
}

bool ten_app_init_uri(ten_app_t *self, ten_value_t *value) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");
  TEN_ASSERT(value && ten_value_check_integrity(value), "Should not happen.");

  if (!ten_value_is_string(value)) {
    TEN_LOGW("Invalid uri.");
    return false;
  }

  ten_string_t default_url;
  ten_string_init_formatted(&default_url, TEN_STR_LOCALHOST);

  const char *url_str = ten_value_peek_raw_str(value)
                            ? ten_value_peek_raw_str(value)
                            : ten_string_get_raw_str(&default_url);

  ten_string_set_from_c_str(&self->uri, url_str, strlen(url_str));

  ten_string_deinit(&default_url);

  return true;
}

bool ten_app_init_log_level(ten_app_t *self, ten_value_t *value) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");
  TEN_ASSERT(value && ten_value_check_integrity(value), "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  ten_log_global_set_output_level(ten_value_get_int64(value, &err));

  ten_error_deinit(&err);

  return true;
}

bool ten_app_init_log_file(ten_app_t *self, ten_value_t *value) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");
  TEN_ASSERT(value && ten_value_check_integrity(value), "Should not happen.");

  ten_string_t log_file;
  ten_string_init(&log_file);

  ten_string_init_from_c_str(&log_file, ten_value_peek_raw_str(value),
                             strlen(ten_value_peek_raw_str(value)));

  if (!ten_string_is_empty(&log_file)) {
    ten_log_global_set_output_to_file(ten_string_get_raw_str(&log_file));
  }

  ten_string_deinit(&log_file);

  return true;
}

static bool ten_app_determine_ten_namespace_properties(
    ten_app_t *self, ten_value_t *ten_namespace_properties) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");
  TEN_ASSERT(
      ten_namespace_properties && ten_value_is_object(ten_namespace_properties),
      "Should not happen.");

  ten_value_object_foreach(ten_namespace_properties, iter) {
    ten_value_kv_t *prop_kv = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(prop_kv && ten_value_kv_check_integrity(prop_kv),
               "Should not happen.");

    ten_string_t *item_key = &prop_kv->key;
    ten_value_t *item_value = prop_kv->value;

    for (int i = 0; i < ten_app_ten_namespace_prop_info_list_size; ++i) {
      const ten_app_ten_namespace_prop_info_t *prop_info =
          &ten_app_ten_namespace_prop_info_list[i];
      if (ten_string_is_equal_c_str(item_key, prop_info->name)) {
        bool rc = prop_info->init_from_value(self, item_value);
        if (rc) {
          break;
        } else {
          TEN_LOGW("Failed to init property: %s", prop_info->name);
          return false;
        }
      }
    }
  }

  return true;
}

bool ten_app_handle_ten_namespace_properties(ten_app_t *self) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");

  ten_value_t *ten_namespace_properties =
      ten_app_get_ten_namespace_properties(self);
  if (ten_namespace_properties == NULL) {
    return true;
  }

  TEN_ASSERT(
      ten_namespace_properties && ten_value_is_object(ten_namespace_properties),
      "Should not happen.");

  // Set default value for app properties and global log level.
  self->one_event_loop_per_engine = false;
  self->long_running_mode = false;

  // First, set the log-related configuration to default values. This way, if
  // there are no log-related properties under the `ten` namespace, the default
  // values will be used.
  ten_log_global_set_output_to_stderr();
  ten_log_global_set_output_level(DEFAULT_LOG_OUTPUT_LEVEL);

  if (!ten_app_determine_ten_namespace_properties(self,
                                                  ten_namespace_properties)) {
    return false;
  }

  return true;
}

void ten_app_handle_metadata(ten_app_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_app_check_integrity(self, true), "Invalid use of app %p.",
             self);

  // Load custom TEN app metadata.
  ten_metadata_load(ten_app_on_configure, self->ten_env);
}
