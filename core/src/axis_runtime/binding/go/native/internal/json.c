//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/go/internal/json.h"

#include "include_internal/axis_runtime/binding/go/internal/common.h"
#include "axis_runtime/common/errno.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"

axis_json_t *axis_go_json_loads(const void *json_bytes, int json_bytes_len,
                              axis_go_error_t *status) {
  axis_ASSERT(json_bytes && json_bytes_len > 0, "Should not happen.");
  axis_ASSERT(status, "Should not happen.");

  axis_string_t input;
  axis_string_init_formatted(&input, "%.*s", json_bytes_len, json_bytes);

  axis_error_t c_err;
  axis_error_init(&c_err);

  axis_json_t *json =
      axis_json_from_string(axis_string_get_raw_str(&input), &c_err);

  axis_string_deinit(&input);
  if (!json) {
    axis_go_error_set(status, axis_ERRNO_INVALID_JSON, axis_error_errmsg(&c_err));
  }

  axis_error_deinit(&c_err);
  return json;
}
