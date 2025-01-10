//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/value/value.h"

// {
//   "path": "...",
//   "conversion_mode": "from_original",
//   "original_path": "..."
// }
typedef struct axis_msg_conversion_per_property_rule_from_original_t {
  axis_string_t original_path;
} axis_msg_conversion_per_property_rule_from_original_t;

axis_RUNTIME_PRIVATE_API void
axis_msg_conversion_per_property_rule_from_original_deinit(
    axis_msg_conversion_per_property_rule_from_original_t *self);

axis_RUNTIME_PRIVATE_API bool
axis_msg_conversion_per_property_rule_from_original_convert(
    axis_msg_conversion_per_property_rule_from_original_t *self,
    axis_shared_ptr_t *msg, axis_shared_ptr_t *new_msg,
    const char *new_msg_property_path, axis_error_t *err);

axis_RUNTIME_PRIVATE_API void
axis_msg_conversion_per_property_rule_from_original_from_json(
    axis_msg_conversion_per_property_rule_from_original_t *self,
    axis_json_t *json);

axis_RUNTIME_PRIVATE_API bool
axis_msg_conversion_per_property_rule_from_original_to_json(
    axis_msg_conversion_per_property_rule_from_original_t *self,
    axis_json_t *json, axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool
axis_msg_conversion_per_property_rule_from_original_from_value(
    axis_msg_conversion_per_property_rule_from_original_t *self,
    axis_value_t *value, axis_error_t *err);

axis_RUNTIME_PRIVATE_API void
axis_msg_conversion_per_property_rule_from_original_to_value(
    axis_msg_conversion_per_property_rule_from_original_t *self,
    axis_value_t *value);
