//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_runtime/common/status_code.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/smart_ptr.h"

typedef struct axis_cmd_result_t axis_cmd_result_t;

axis_RUNTIME_API axis_shared_ptr_t *axis_cmd_result_create(
    axis_STATUS_CODE status_code);

axis_RUNTIME_API axis_shared_ptr_t *axis_cmd_result_create_from_cmd(
    axis_STATUS_CODE status_code, axis_shared_ptr_t *original_cmd);

axis_RUNTIME_API axis_STATUS_CODE
axis_cmd_result_get_status_code(axis_shared_ptr_t *self);

axis_RUNTIME_API bool axis_cmd_result_is_final(axis_shared_ptr_t *self,
                                             axis_error_t *err);

axis_RUNTIME_API bool axis_cmd_result_is_completed(axis_shared_ptr_t *self,
                                                 axis_error_t *err);

axis_RUNTIME_API bool axis_cmd_result_set_final(axis_shared_ptr_t *self,
                                              bool is_final, axis_error_t *err);
