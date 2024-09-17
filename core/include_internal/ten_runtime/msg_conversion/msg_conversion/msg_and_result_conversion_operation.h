//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_utils/lib/json.h"
#include "ten_utils/value/value.h"

typedef struct ten_msg_conversion_operation_t ten_msg_conversion_operation_t;

typedef struct ten_msg_and_result_conversion_operation_t {
  ten_msg_conversion_operation_t *msg_operation;
  ten_msg_conversion_operation_t *result_operation;
} ten_msg_and_result_conversion_operation_t;

TEN_RUNTIME_PRIVATE_API void ten_msg_and_result_conversion_operation_destroy(
    ten_msg_and_result_conversion_operation_t *self);

TEN_RUNTIME_PRIVATE_API ten_msg_and_result_conversion_operation_t *
ten_msg_and_result_conversion_operation_from_json(ten_json_t *json,
                                                  ten_error_t *err);

TEN_RUNTIME_PRIVATE_API ten_json_t *
ten_msg_and_result_conversion_operation_to_json(
    ten_msg_and_result_conversion_operation_t *self, ten_error_t *err);

TEN_RUNTIME_PRIVATE_API ten_msg_and_result_conversion_operation_t *
ten_msg_and_result_conversion_operation_from_value(ten_value_t *value,
                                                   ten_error_t *err);

TEN_RUNTIME_PRIVATE_API ten_value_t *
ten_msg_and_result_conversion_operation_to_value(
    ten_msg_and_result_conversion_operation_t *self, ten_error_t *err);
