//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/schema_store/interface.h"
#include "include_internal/ten_rust/ten_rust.h"
#include "ten_runtime/common/errno.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/log/log.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/memory.h"
#include "ten_utils/value/value.h"
#include "ten_utils/value/value_is.h"
#include "ten_utils/value/value_json.h"

ten_value_t *ten_interface_schema_info_resolve(
    ten_value_t *unresolved_interface_schema_def, const char *base_dir,
    ten_error_t *err) {
  TEN_ASSERT(unresolved_interface_schema_def &&
                 ten_value_check_integrity(unresolved_interface_schema_def),
             "Invalid argument.");
  TEN_ASSERT(err && ten_error_check_integrity(err), "Invalid argument.");
  TEN_ASSERT(base_dir, "Invalid argument.");

  if (!ten_value_is_array(unresolved_interface_schema_def)) {
    ten_error_set(err, TEN_ERRNO_GENERIC,
                  "The interface schema should be an array.");
    return NULL;
  }

  ten_json_t *unresolved_interface_schema_json =
      ten_value_to_json(unresolved_interface_schema_def);

  bool must_free = false;
  const char *unresolved_interface_schema_json_str =
      ten_json_to_string(unresolved_interface_schema_json, NULL, &must_free);

  ten_value_t *resolved_interface_schema_def = NULL;
  const char *resolved_interface_schema_str = NULL;
  const char *err_msg = NULL;

  bool rc = ten_interface_schema_resolve_definition(
      unresolved_interface_schema_json_str, base_dir,
      &resolved_interface_schema_str, &err_msg);

  if (rc) {
    ten_json_t *resolved_interface_schema_json =
        ten_json_from_string(resolved_interface_schema_str, err);
    if (!resolved_interface_schema_json) {
      TEN_LOGW("Invalid interface schema string after resolved, %s.",
               ten_error_errmsg(err));
    } else {
      resolved_interface_schema_def =
          ten_value_from_json(resolved_interface_schema_json);

      ten_json_destroy(resolved_interface_schema_json);
    }

    ten_rust_free_cstring(resolved_interface_schema_str);
  } else {
    ten_error_set(err, TEN_ERRNO_GENERIC, err_msg);

    ten_rust_free_cstring(err_msg);
  }

  if (must_free) {
    TEN_FREE(unresolved_interface_schema_json_str);
  }

  ten_json_destroy(unresolved_interface_schema_json);

  bool success = ten_error_is_success(err);
  if (success) {
    TEN_ASSERT(resolved_interface_schema_def, "Should not happen.");
  }

  return resolved_interface_schema_def;
}
