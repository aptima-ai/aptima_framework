//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "include_internal/ten_runtime/msg_conversion/msg_conversion_operation/base.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/value/value.h"

typedef struct ten_msg_conversion_operation_per_property_rules_t
    ten_msg_conversion_operation_per_property_rules_t;

// {
//   "type": "per_property",
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
typedef struct ten_msg_conversion_operation_per_property_t {
  ten_msg_conversion_operation_t base;
  ten_msg_conversion_operation_per_property_rules_t *rules;
} ten_msg_conversion_operation_per_property_t;

TEN_RUNTIME_PRIVATE_API ten_msg_conversion_operation_per_property_t *
ten_msg_conversion_operation_per_property_create(
    ten_msg_conversion_operation_per_property_rules_t *rules);

TEN_RUNTIME_PRIVATE_API void ten_msg_conversion_operation_per_property_destroy(
    ten_msg_conversion_operation_per_property_t *self);

TEN_RUNTIME_PRIVATE_API ten_msg_conversion_operation_per_property_t *
ten_msg_conversion_operation_per_property_from_json(ten_json_t *json,
                                                    ten_error_t *err);

TEN_RUNTIME_PRIVATE_API ten_json_t *
ten_msg_conversion_operation_per_property_to_json(
    ten_msg_conversion_operation_per_property_t *self, ten_error_t *err);

TEN_RUNTIME_PRIVATE_API ten_msg_conversion_operation_per_property_t *
ten_msg_conversion_operation_per_property_from_value(ten_value_t *value,
                                                     ten_error_t *err);

TEN_RUNTIME_PRIVATE_API ten_value_t *
ten_msg_conversion_operation_per_property_to_value(
    ten_msg_conversion_operation_per_property_t *self, ten_error_t *err);
