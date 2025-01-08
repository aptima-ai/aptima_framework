//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>
#include <stdint.h>

#include "axis_runtime/binding/go/interface/ten/common.h"
#include "axis_utils/lib/signature.h"

#define axis_GO_VALUE_SIGNATURE 0x34898DCE3ED53FB8U

typedef struct axis_value_t axis_value_t;

typedef struct axis_go_value_t {
  axis_signature_t signature;

  axis_go_bridge_t bridge;

  bool own;
  axis_value_t *c_value;
} axis_go_value_t;

axis_RUNTIME_PRIVATE_API void axis_go_axis_value_get_type_and_size(
    axis_value_t *self, uint8_t *type, uintptr_t *size);

axis_RUNTIME_PRIVATE_API void axis_go_axis_value_get_string(
    axis_value_t *self, void *value, axis_go_error_t *status);

axis_RUNTIME_PRIVATE_API void axis_go_axis_value_get_buf(axis_value_t *self,
                                                      void *value,
                                                      axis_go_error_t *status);

axis_RUNTIME_PRIVATE_API void axis_go_axis_value_get_ptr(axis_value_t *self,
                                                      axis_go_handle_t *value,
                                                      axis_go_error_t *status);

axis_RUNTIME_PRIVATE_API axis_value_t *axis_go_axis_value_create_buf(void *value,
                                                                 int value_len);

axis_RUNTIME_PRIVATE_API axis_value_t *axis_go_axis_value_create_ptr(
    axis_go_handle_t value);

axis_RUNTIME_PRIVATE_API bool axis_go_axis_value_to_json(axis_value_t *self,
                                                      uintptr_t *json_str_len,
                                                      const char **json_str,
                                                      axis_go_error_t *status);

axis_RUNTIME_PRIVATE_API bool axis_go_value_check_integrity(axis_go_value_t *self);

axis_RUNTIME_PRIVATE_API axis_go_handle_t
axis_go_value_go_handle(axis_go_value_t *self);

axis_RUNTIME_PRIVATE_API axis_value_t *axis_go_value_c_value(axis_go_value_t *self);

axis_RUNTIME_PRIVATE_API axis_go_handle_t axis_go_wrap_value(axis_value_t *c_value,
                                                          bool own);
