//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/lib/json.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/value/value.h"

typedef struct axis_msg_conversion_t axis_msg_conversion_t;

typedef axis_shared_ptr_t *(*axis_msg_conversion_func_t)(
    axis_msg_conversion_t *operation, axis_shared_ptr_t *msg, axis_error_t *err);

/**
 * If the desired message conversion is beyond the capabilities of the APTIMA
 * runtime, an extension can be used to handle the conversion. To avoid having
 * message conversion logic intrude into other extensions, a dedicated extension
 * specifically for message conversion can be implemented. This standalone
 * extension should be designed with enough flexibility to accommodate various
 * conversion needs.
 */
typedef enum axis_MSG_CONVERSION_TYPE {
  axis_MSG_CONVERSION_TYPE_INVALID,

  axis_MSG_CONVERSION_TYPE_PER_PROPERTY,
} axis_MSG_CONVERSION_TYPE;

typedef struct axis_msg_conversion_t {
  axis_MSG_CONVERSION_TYPE type;
  axis_msg_conversion_func_t operation;
} axis_msg_conversion_t;

axis_RUNTIME_PRIVATE_API void axis_msg_conversion_destroy(
    axis_msg_conversion_t *self);

axis_RUNTIME_PRIVATE_API axis_msg_conversion_t *axis_msg_conversion_from_json(
    axis_json_t *json, axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_json_t *axis_msg_conversion_to_json(
    axis_msg_conversion_t *self, axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_msg_conversion_t *axis_msg_conversion_from_value(
    axis_value_t *value, axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_value_t *axis_msg_conversion_to_value(
    axis_msg_conversion_t *self, axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_shared_ptr_t *axis_msg_conversion_convert(
    axis_msg_conversion_t *self, axis_shared_ptr_t *msg, axis_error_t *err);
