//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

typedef struct axis_env_tester_t axis_env_tester_t;
typedef struct axis_env_tester_proxy_t axis_env_tester_proxy_t;
typedef struct axis_error_t axis_error_t;

typedef void (*axis_env_tester_proxy_notify_func_t)(
    axis_env_tester_t *axis_env_tester, void *user_data);

axis_RUNTIME_API axis_env_tester_proxy_t *axis_env_tester_proxy_create(
    axis_env_tester_t *axis_env_tester, axis_error_t *err);

axis_RUNTIME_API bool axis_env_tester_proxy_release(axis_env_tester_proxy_t *self,
                                                  axis_error_t *err);

axis_RUNTIME_API bool axis_env_tester_proxy_notify(
    axis_env_tester_proxy_t *self,
    axis_env_tester_proxy_notify_func_t notify_func, void *user_data,
    axis_error_t *err);
