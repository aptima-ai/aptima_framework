//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/smart_ptr.h"

typedef void (*axis_env_return_result_error_handler_func_t)(axis_env_t *self,
                                                           void *user_data,
                                                           axis_error_t *err);

axis_RUNTIME_API bool axis_env_return_result(
    axis_env_t *self, axis_shared_ptr_t *result, axis_shared_ptr_t *target_cmd,
    axis_env_return_result_error_handler_func_t error_handler,
    void *error_handler_user_data, axis_error_t *err);

axis_RUNTIME_API bool axis_env_return_result_directly(
    axis_env_t *self, axis_shared_ptr_t *cmd,
    axis_env_return_result_error_handler_func_t error_handler,
    void *error_handler_user_data, axis_error_t *err);
