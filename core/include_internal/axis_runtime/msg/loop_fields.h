//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/lib/error.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/value/value_kv.h"

typedef struct axis_msg_t axis_msg_t;

typedef struct axis_msg_field_process_data_t {
  // The name of the field.
  const char *field_name;

  // The value of the field.
  axis_value_t *field_value;

  // Whether this field is a user-defined field. If it is not user-defined, then
  // it is a `_ten` field.
  bool is_user_defined_properties;

  // Whether the value has been modified. Some logic checks if the value has
  // changed, requiring it to be written back to the original memory space for
  // that field. For example, in the message, `src/dest loc` exists as a
  // `axis_loc_t` type. If, within certain process logic, the value is modified,
  // the updated value should reflect in the corresponding `axis_loc_t`. The
  // `value_is_changed_after_process` field indicates whether the value has been
  // modified, allowing users to determine if it should be written back to the
  // original field.
  bool value_is_changed_after_process;
} axis_msg_field_process_data_t;

typedef bool (*axis_raw_msg_process_one_field_func_t)(
    axis_msg_t *msg, axis_msg_field_process_data_t *field, void *user_data,
    axis_error_t *err);

axis_RUNTIME_API void axis_msg_field_process_data_init(
    axis_msg_field_process_data_t *self, const char *field_name,
    axis_value_t *field_value, bool is_user_defined_properties);

axis_RUNTIME_API bool axis_raw_msg_loop_all_fields(
    axis_msg_t *self, axis_raw_msg_process_one_field_func_t cb, void *user_data,
    axis_error_t *err);

axis_RUNTIME_API bool axis_msg_loop_all_fields(
    axis_shared_ptr_t *self, axis_raw_msg_process_one_field_func_t cb,
    void *user_data, axis_error_t *err);
