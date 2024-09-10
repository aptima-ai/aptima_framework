//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_utils/lib/smart_ptr.h"

typedef struct ten_msg_conversion_operation_t ten_msg_conversion_operation_t;

typedef struct ten_msg_and_result_conversion_t {
  ten_shared_ptr_t *msg;
  ten_msg_conversion_operation_t *operation;
} ten_msg_and_result_conversion_t;

TEN_RUNTIME_PRIVATE_API ten_msg_and_result_conversion_t *
ten_msg_and_result_conversion_create(
    ten_shared_ptr_t *msg, ten_msg_conversion_operation_t *result_conversion);

TEN_RUNTIME_PRIVATE_API void ten_msg_and_result_conversion_destroy(
    ten_msg_and_result_conversion_t *self);
