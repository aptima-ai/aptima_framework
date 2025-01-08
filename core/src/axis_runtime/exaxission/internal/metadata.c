//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension/metadata.h"

#include <stddef.h>
#include <stdint.h>

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension/extension_info/extension_info.h"
#include "include_internal/axis_runtime/extension/path_timer.h"
#include "include_internal/axis_runtime/extension/axis_env/metadata.h"
#include "include_internal/axis_runtime/extension_thread/extension_thread.h"
#include "include_internal/axis_utils/lib/placeholder.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_get.h"
#include "axis_utils/value/value_is.h"
#include "axis_utils/value/value_merge.h"

static bool axis_extension_determine_axis_namespace_properties(
    axis_extension_t *self, axis_value_t *axis_namespace_properties) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Invalid argument.");
  axis_ASSERT(
      axis_namespace_properties && axis_value_is_object(axis_namespace_properties),
      "Invalid argument.");

  axis_error_t err;
  axis_error_init(&err);

  axis_value_object_foreach(axis_namespace_properties, iter) {
    axis_value_kv_t *kv = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(kv && axis_value_kv_check_integrity(kv), "Should not happen.");

    if (axis_string_is_equal_c_str(&kv->key, axis_STR_PATH_TIMEOUT)) {
      if (axis_value_is_object(kv->value)) {
        axis_value_t *in_path_timeout_value =
            axis_value_object_peek(kv->value, axis_STR_IN_PATH);
        if (in_path_timeout_value) {
          int64_t timeout = axis_value_get_int64(in_path_timeout_value, &err);
          if (timeout > 0) {
            self->path_timeout_info.in_path_timeout = timeout;
          }
        }

        axis_value_t *out_path_timeout_value =
            axis_value_object_peek(kv->value, axis_STR_OUT_PATH);
        if (out_path_timeout_value) {
          int64_t timeout = axis_value_get_int64(out_path_timeout_value, &err);
          if (timeout > 0) {
            self->path_timeout_info.out_path_timeout = timeout;
          }
        }
      } else {
        int64_t path_timeout = axis_value_get_int64(kv->value, &err);
        if (path_timeout > 0) {
          self->path_timeout_info.out_path_timeout = path_timeout;
        }
      }
    }

    if (axis_string_is_equal_c_str(&kv->key, axis_STR_PATH_CHECK_INTERVAL)) {
      int64_t check_interval = axis_value_get_int64(kv->value, &err);
      if (check_interval > 0) {
        self->path_timeout_info.check_interval = check_interval;
      }
    }
  }

  axis_error_deinit(&err);

  return true;
}

// It is unreasonable for 'in_path' to be removed due to timeout before
// 'out_path' is removed for the same reason. To eliminate the chance of
// 'in_path' being removed from the path table prior to the removal of
// 'out_path', we ensure that the timeout value for 'in_path' is greater than
// the sum of the timeout value of 'out_path' and the time-out-checking
// interval.
//
// Given the following scenario:
//
// Client ───► ExtensionA ──cmdA──► ExtensionB ──cmdB──► ExtensionC
//                ▲                    │  ▲                  │
//                │                    │  │                  │
//                └──────respA─────────┘  └───────respB──────┘
//
// ExtensionB responds to cmdA with respA only after it receives respB from
// the ExtensionC. So we have to ensure that, for ExtensionB, the in_path is
// removed after the out_path is removed.
//
// Suppose the timeout value for 'in_path' is 'x'.
// Suppose the timeout value for 'out_path' is 'y'.
// Suppose the time-out-checking interval is 'z'.
//
// Suppose the 'in_path' is added to the path table at time t0.
// Suppose the 'out_path' is added to the path table at time t0 + m (m > 0).
//
// The 'in_path' will be removed at time (t0 + x, t0 + x + z).
// The 'out_path' will be removed at time (t0 + m + y, t0 + m + y + z).
//
// To ensure that the 'in_path' is removed __after__ the 'out_path' is
// removed, we have to ensure that the earliest time for the 'in_path' to be
// removed is greater than the latest time for the 'out_path' to be removed.
// That is, we have to ensure that:
//
// t0 + x > t0 + m + y + z (m > 0)
// ===>  x > m + y + z (m > 0)
//
// 'm' can be any positive integer, ranging from potentially miniscule values
// like 10ns to substantial figures like 100s. Returning to the example above,
// if ExtensionB send cmdB to ExtensionC immediately after it receives cmdA
// from ExtensionA, then 'm' will be very small. We can almost guarantee that
// 'm' is less than 1s. So we can safely assume that 'm' is 1s. That is, we
// have to ensure that:
//
// x > 1s + y + z
//
// However, if ExtensionB send cmdB to ExtensionC after a certain period of
// time after it receives cmdA from ExtensionA, then 'm' will be very large.
// In this case, developers should set the timeout value for 'in_path' by
// themselves.
static void axis_extension_adjust_in_path_timeout(axis_extension_t *self) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Should not happen.");

  axis_path_timeout_info *path_timeout_info = &self->path_timeout_info;
  axis_ASSERT(path_timeout_info, "Should not happen.");

  uint64_t in_path_min_timeout = UINT64_MAX;
  uint64_t one_sec_in_us = (uint64_t)1 * 1000 * 1000;

  // Considering the case of integer overflow, calculate at least what the
  // value of 'in_path timeout' should be.
  if (path_timeout_info->out_path_timeout <
      UINT64_MAX - path_timeout_info->check_interval) {
    in_path_min_timeout =
        path_timeout_info->out_path_timeout + path_timeout_info->check_interval;
  }

  // An extension by default has one second to process its own operations.
  if (in_path_min_timeout < UINT64_MAX - one_sec_in_us) {
    in_path_min_timeout += one_sec_in_us;
  }

  // Update the in_path timeout to the calculated minimal value if the
  // original value is too small.
  if (path_timeout_info->in_path_timeout < in_path_min_timeout) {
    path_timeout_info->in_path_timeout = in_path_min_timeout;
  }
}

// Retrieve those property fields that are reserved for the TEN runtime under
// the 'ten' namespace.
static axis_value_t *axis_extension_get_axis_namespace_properties(
    axis_extension_t *self) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Should not happen.");

  return axis_value_object_peek(&self->property, axis_STR_UNDERLINE_TEN);
}

static bool axis_extension_graph_property_resolve_placeholders(
    axis_extension_t *self, axis_value_t *curr_value, axis_error_t *err) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Should not happen.");

  if (!curr_value || !axis_value_is_valid(curr_value)) {
    return false;
  }

  switch (curr_value->type) {
    case axis_TYPE_INT8:
    case axis_TYPE_INT16:
    case axis_TYPE_INT32:
    case axis_TYPE_INT64:
    case axis_TYPE_UINT8:
    case axis_TYPE_UINT16:
    case axis_TYPE_UINT32:
    case axis_TYPE_UINT64:
    case axis_TYPE_FLOAT32:
    case axis_TYPE_FLOAT64:
      return true;

    case axis_TYPE_STRING: {
      const char *str_value = axis_value_peek_raw_str(curr_value, err);
      if (axis_c_str_is_placeholder(str_value)) {
        axis_placeholder_t placeholder;
        axis_placeholder_init(&placeholder);

        if (!axis_placeholder_parse(&placeholder, str_value, err)) {
          return false;
        }

        if (!axis_placeholder_resolve(&placeholder, curr_value, err)) {
          return false;
        }

        axis_placeholder_deinit(&placeholder);
      }
      return true;
    }

    case axis_TYPE_OBJECT: {
      axis_value_object_foreach(curr_value, iter) {
        axis_value_kv_t *kv = axis_ptr_listnode_get(iter.node);
        axis_ASSERT(kv && axis_value_kv_check_integrity(kv),
                   "Should not happen.");

        axis_value_t *kv_value = axis_value_kv_get_value(kv);
        if (!axis_extension_graph_property_resolve_placeholders(self, kv_value,
                                                               err)) {
          return false;
        }
      }
      return true;
    }

    case axis_TYPE_ARRAY: {
      axis_value_array_foreach(curr_value, iter) {
        axis_value_t *array_value = axis_ptr_listnode_get(iter.node);
        axis_ASSERT(array_value && axis_value_check_integrity(array_value),
                   "Should not happen.");

        if (!axis_extension_graph_property_resolve_placeholders(
                self, array_value, err)) {
          return false;
        }
      }
      return true;
    }

    default:
      return true;
  }

  axis_ASSERT(0, "Should not happen.");
}

bool axis_extension_resolve_properties_in_graph(axis_extension_t *self,
                                               axis_error_t *err) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Should not happen.");

  if (!self->extension_info) {
    return true;
  }

  axis_value_t *graph_property = self->extension_info->property;
  if (!graph_property) {
    return true;
  }

  if (!axis_value_is_valid(graph_property)) {
    return false;
  }

  axis_ASSERT(axis_value_is_object(graph_property), "Should not happen.");

  return axis_extension_graph_property_resolve_placeholders(self, graph_property,
                                                           err);
}

void axis_extension_merge_properties_from_graph(axis_extension_t *self) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Should not happen.");

  // Merge properties in graph into the extension's property store.
  if (self->extension_info && self->extension_info->property) {
    axis_value_object_merge_with_clone(&self->property,
                                      self->extension_info->property);
  }
}

// Determine the internal properties of the extension according to the
// 'ten' object in the extension's property store. like:
// {
//   "_ten": {
//     "path_timeout": {
//       "in_path": 5000000,
//       "out_path": 1000000,
//     },
//     "path_check_interval": 1000000
//   }
// }
bool axis_extension_handle_axis_namespace_properties(
    axis_extension_t *self, axis_extension_context_t *extension_context) {
  axis_ASSERT(
      self && axis_extension_check_integrity(self, true) && extension_context,
      "Should not happen.");

  // This function is safe to be called from the extension main threads,
  // because all the resources it accesses are the
  // 'extension_info_from_graph', and all the
  // 'extension_info_from_graph' are setup completely before the
  // extension main threads are started, that means all the
  // 'extension_info_from_graph' will not be modified when this
  // function is being called.
  axis_ASSERT(axis_extension_thread_call_by_me(self->extension_thread),
             "Should not happen.");

  axis_value_t *axis_namespace_properties =
      axis_extension_get_axis_namespace_properties(self);
  if (axis_namespace_properties == NULL) {
    axis_LOGI("[%s] `%s` section is not found in the property, skip.",
             axis_extension_get_name(self, true), axis_STR_UNDERLINE_TEN);
    return true;
  }

  axis_extension_determine_axis_namespace_properties(self,
                                                   axis_namespace_properties);

  axis_extension_adjust_in_path_timeout(self);

  return true;
}
