//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdbool.h>

#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/smart_ptr.h"

typedef void (*ten_env_return_result_error_handler_func_t)(ten_env_t *self,
                                                           void *user_data,
                                                           ten_error_t *err);

TEN_RUNTIME_API bool ten_env_return_result(
    ten_env_t *self, ten_shared_ptr_t *result, ten_shared_ptr_t *target_cmd,
    ten_env_return_result_error_handler_func_t error_handler,
    void *error_handler_user_data, ten_error_t *err);

TEN_RUNTIME_API bool ten_env_return_result_directly(
    ten_env_t *self, ten_shared_ptr_t *cmd,
    ten_env_return_result_error_handler_func_t error_handler,
    void *error_handler_user_data, ten_error_t *err);
