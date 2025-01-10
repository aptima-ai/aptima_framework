//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/msg_conversion/msg_conversion/per_property/fixed_value.h"
#include "include_internal/axis_runtime/msg_conversion/msg_conversion/per_property/from_original.h"
#include "axis_utils/lib/error.h"

/**
 * APTIMA runtime only provides basic message conversion methods, with plans
 * for moderate expansion in the future. If you need more flexible message
 * conversion, you can develop your own extension for message conversion. In
 * this extension's on_cmd/data/... APIs, you can perform any kind of message
 * conversion.
 */
typedef enum axis_MSG_CONVERSION_PER_PROPERTY_RULE_CONVERSION_MODE {
  axis_MSG_CONVERSION_PER_PROPERTY_RULE_CONVERSION_MODE_INVALID,

  axis_MSG_CONVERSION_PER_PROPERTY_RULE_CONVERSION_MODE_FROM_ORIGINAL,
  axis_MSG_CONVERSION_PER_PROPERTY_RULE_CONVERSION_MODE_FIXED_VALUE,
} axis_MSG_CONVERSION_PER_PROPERTY_RULE_CONVERSION_MODE;

// {
//   "rules": [{
//     "path": "...",
//     "conversion_mode": "from_original",
//     "original_path": "..."
//   },{
//     "path": "...",
//     "conversion_mode": "fixed_value",
//     "value": "..."
//   }]
// }
typedef struct axis_msg_conversion_per_property_rule_t {
  axis_string_t property_path;

  axis_MSG_CONVERSION_PER_PROPERTY_RULE_CONVERSION_MODE conversion_mode;

  union {
    axis_msg_conversion_per_property_rule_from_original_t from_original;
    axis_msg_conversion_per_property_rule_fixed_value_t fixed_value;
  } u;
} axis_msg_conversion_per_property_rule_t;

axis_RUNTIME_PRIVATE_API void axis_msg_conversion_per_property_rule_destroy(
    axis_msg_conversion_per_property_rule_t *self);

axis_RUNTIME_PRIVATE_API bool axis_msg_conversion_per_property_rule_convert(
    axis_msg_conversion_per_property_rule_t *self, axis_shared_ptr_t *msg,
    axis_shared_ptr_t *new_msg, axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_msg_conversion_per_property_rule_t *
axis_msg_conversion_per_property_rule_from_json(axis_json_t *json,
                                               axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_json_t *
axis_msg_conversion_per_property_rule_to_json(
    axis_msg_conversion_per_property_rule_t *self, axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_msg_conversion_per_property_rule_t *
axis_msg_conversion_per_property_rule_from_value(axis_value_t *value,
                                                axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_value_t *
axis_msg_conversion_per_property_rule_to_value(
    axis_msg_conversion_per_property_rule_t *self, axis_error_t *err);
