//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_utils/schema/bindings/rust/schema.h"

#include "include_internal/axis_utils/schema/schema.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/macro/check.h"

axis_schema_t *axis_schema_create_from_json_str_proxy(const char *json_string,
                                                    const char **err_msg) {
  axis_ASSERT(json_string, "Invalid argument.");
  return axis_schema_create_from_json_str(json_string, err_msg);
}

void axis_schema_destroy_proxy(const axis_schema_t *self) {
  axis_schema_t *self_ = (axis_schema_t *)self;
  axis_ASSERT(self && axis_schema_check_integrity(self_), "Invalid argument.");
  axis_schema_destroy(self_);
}

bool axis_schema_adjust_and_validate_json_str_proxy(const axis_schema_t *self,
                                                   const char *json_string,
                                                   const char **err_msg) {
  axis_schema_t *self_ = (axis_schema_t *)self;
  axis_ASSERT(self && axis_schema_check_integrity(self_), "Invalid argument.");
  axis_ASSERT(json_string, "Invalid argument.");

  return axis_schema_adjust_and_validate_json_str(self_, json_string, err_msg);
}

bool axis_schema_is_compatible_proxy(const axis_schema_t *self,
                                    const axis_schema_t *target,
                                    const char **err_msg) {
  axis_schema_t *self_ = (axis_schema_t *)self;
  axis_ASSERT(self && axis_schema_check_integrity(self_), "Invalid argument.");

  axis_schema_t *target_ = (axis_schema_t *)target;
  axis_ASSERT(target && axis_schema_check_integrity(target_),
             "Invalid argument.");

  axis_error_t err;
  axis_error_init(&err);

  bool result = axis_schema_is_compatible(self_, target_, &err);
  if (!result) {
    *err_msg = axis_strdup(axis_error_errmsg(&err));
  }

  axis_error_deinit(&err);
  return result;
}
