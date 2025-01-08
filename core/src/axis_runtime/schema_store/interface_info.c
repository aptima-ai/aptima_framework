//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/schema_store/interface.h"
#include "axis_runtime/common/errno.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_is.h"
#include "axis_utils/value/value_json.h"

#if defined(axis_ENABLE_axis_RUST_APIS)
#include "include_internal/axis_rust/axis_rust.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/macro/memory.h"
#endif

axis_value_t *axis_interface_schema_info_resolve(
    axis_value_t *unresolved_interface_schema_def,
    axis_UNUSED const char *base_dir, axis_error_t *err) {
  axis_ASSERT(unresolved_interface_schema_def &&
                 axis_value_check_integrity(unresolved_interface_schema_def),
             "Invalid argument.");
  axis_ASSERT(err && axis_error_check_integrity(err), "Invalid argument.");

  if (!axis_value_is_array(unresolved_interface_schema_def)) {
    axis_error_set(err, axis_ERRNO_GENERIC,
                  "The interface schema should be an array.");
    return NULL;
  }

#if defined(axis_ENABLE_axis_RUST_APIS)
  axis_json_t *unresolved_interface_schema_json =
      axis_value_to_json(unresolved_interface_schema_def);

  bool must_free = false;
  const char *unresolved_interface_schema_json_str =
      axis_json_to_string(unresolved_interface_schema_json, NULL, &must_free);

  axis_value_t *resolved_interface_schema_def = NULL;
  const char *resolved_interface_schema_str = NULL;
  const char *err_msg = NULL;

  bool rc = axis_interface_schema_resolve_definition(
      unresolved_interface_schema_json_str, base_dir,
      &resolved_interface_schema_str, &err_msg);

  if (rc) {
    axis_json_t *resolved_interface_schema_json =
        axis_json_from_string(resolved_interface_schema_str, err);
    if (!resolved_interface_schema_json) {
      axis_LOGW("Invalid interface schema string after resolved, %s.",
               axis_error_errmsg(err));
    } else {
      resolved_interface_schema_def =
          axis_value_from_json(resolved_interface_schema_json);

      axis_json_destroy(resolved_interface_schema_json);
    }

    axis_rust_free_cstring(resolved_interface_schema_str);
  } else {
    axis_error_set(err, axis_ERRNO_GENERIC, err_msg);

    axis_rust_free_cstring(err_msg);
  }

  if (must_free) {
    axis_FREE(unresolved_interface_schema_json_str);
  }

  axis_json_destroy(unresolved_interface_schema_json);

  bool success = axis_error_is_success(err);
  if (success) {
    axis_ASSERT(resolved_interface_schema_def, "Should not happen.");
  }

  return resolved_interface_schema_def;
#else
  return NULL;
#endif
}
