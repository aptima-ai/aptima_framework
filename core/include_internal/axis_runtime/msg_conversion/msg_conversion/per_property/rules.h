//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/container/list.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/value/value.h"

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
typedef struct axis_msg_conversion_per_property_rules_t {
  axis_list_t rules;    // axis_msg_conversion_per_property_rule_t
  bool keep_original;  // Determine whether original properties will be cloned.
} axis_msg_conversion_per_property_rules_t;

axis_RUNTIME_PRIVATE_API void axis_msg_conversion_per_property_rules_destroy(
    axis_msg_conversion_per_property_rules_t *self);

axis_RUNTIME_PRIVATE_API axis_shared_ptr_t *
axis_msg_conversion_per_property_rules_convert(
    axis_msg_conversion_per_property_rules_t *self, axis_shared_ptr_t *msg,
    axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_shared_ptr_t *
axis_result_conversion_per_property_rules_convert(
    axis_msg_conversion_per_property_rules_t *self, axis_shared_ptr_t *msg,
    axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_msg_conversion_per_property_rules_t *
axis_msg_conversion_per_property_rules_from_json(axis_json_t *json,
                                                axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_json_t *
axis_msg_conversion_per_property_rules_to_json(
    axis_msg_conversion_per_property_rules_t *self, axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_msg_conversion_per_property_rules_t *
axis_msg_conversion_per_property_rules_from_value(axis_value_t *value,
                                                 axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_value_t *
axis_msg_conversion_per_property_rules_to_value(
    axis_msg_conversion_per_property_rules_t *self, axis_error_t *err);
