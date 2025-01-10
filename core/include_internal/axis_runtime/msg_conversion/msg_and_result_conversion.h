//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/value/value.h"

typedef struct axis_msg_conversion_t axis_msg_conversion_t;

typedef struct axis_msg_and_result_conversion_t {
  axis_msg_conversion_t *msg;
  axis_msg_conversion_t *result;
} axis_msg_and_result_conversion_t;

axis_RUNTIME_PRIVATE_API void axis_msg_and_result_conversion_destroy(
    axis_msg_and_result_conversion_t *self);

axis_RUNTIME_PRIVATE_API axis_msg_and_result_conversion_t *
axis_msg_and_result_conversion_from_json(axis_json_t *json, axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_json_t *axis_msg_and_result_conversion_to_json(
    axis_msg_and_result_conversion_t *self, axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_msg_and_result_conversion_t *
axis_msg_and_result_conversion_from_value(axis_value_t *value, axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_value_t *axis_msg_and_result_conversion_to_value(
    axis_msg_and_result_conversion_t *self, axis_error_t *err);
