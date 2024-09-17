//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/binding/go/internal/json.h"

#include "include_internal/ten_runtime/binding/go/internal/common.h"
#include "ten_utils/macro/check.h"
#include "ten_runtime/common/errno.h"
#include "ten_utils/lib/string.h"

ten_json_t *ten_go_json_loads(const void *json_bytes, int json_bytes_len,
                              ten_go_status_t *status) {
  TEN_ASSERT(json_bytes && json_bytes_len > 0, "Should not happen.");
  TEN_ASSERT(status, "Should not happen.");

  ten_string_t input;
  ten_string_init_formatted(&input, "%.*s", json_bytes_len, json_bytes);

  ten_error_t c_err;
  ten_error_init(&c_err);

  ten_json_t *json =
      ten_json_from_string(ten_string_get_raw_str(&input), &c_err);

  ten_string_deinit(&input);
  if (!json) {
    ten_go_status_set(status, TEN_ERRNO_INVALID_JSON, ten_error_errmsg(&c_err));
  }

  ten_error_deinit(&c_err);
  return json;
}
