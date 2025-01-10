//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>

// This header file will be used by the Rust `bindgen` tool to generate the
// FFIs.

// In Rust, `axis_schema_t` will be represented as a raw pointer to an opaque
// struct. Rust cannot directly access the fields of a raw pointer, even if it
// knows the structure's layout.
typedef struct axis_schema_t axis_schema_t;

axis_UTILS_API axis_schema_t *axis_schema_create_from_json_str_proxy(
    const char *json_str, const char **err_msg);

axis_UTILS_API void axis_schema_destroy_proxy(const axis_schema_t *self);

axis_UTILS_API bool axis_schema_adjust_and_validate_json_str_proxy(
    const axis_schema_t *self, const char *json_str, const char **err_msg);

axis_UTILS_API bool axis_schema_is_compatible_proxy(const axis_schema_t *self,
                                                  const axis_schema_t *target,
                                                  const char **err_msg);
