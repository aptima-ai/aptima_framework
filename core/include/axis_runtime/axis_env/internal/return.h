//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include <stdbool.h>

#include "aptima_runtime/aptima_env/aptima_env.h"
#include "aptima_utils/lib/error.h"
#include "aptima_utils/lib/smart_ptr.h"

typedef void (*aptima_env_return_result_error_handler_func_t)(aptima_env_t *self,
                                                           void *user_data,
                                                           aptima_error_t *err);

aptima_RUNTIME_API bool aptima_env_return_result(
    aptima_env_t *self, aptima_shared_ptr_t *result, aptima_shared_ptr_t *target_cmd,
    aptima_env_return_result_error_handler_func_t error_handler,
    void *error_handler_user_data, aptima_error_t *err);

aptima_RUNTIME_API bool aptima_env_return_result_directly(
    aptima_env_t *self, aptima_shared_ptr_t *cmd,
    aptima_env_return_result_error_handler_func_t error_handler,
    void *error_handler_user_data, aptima_error_t *err);
