//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include <stdbool.h>

typedef struct aptima_env_tester_t aptima_env_tester_t;
typedef struct aptima_env_tester_proxy_t aptima_env_tester_proxy_t;
typedef struct aptima_error_t aptima_error_t;

typedef void (*aptima_env_tester_proxy_notify_func_t)(
    aptima_env_tester_t *aptima_env_tester, void *user_data);

aptima_RUNTIME_API aptima_env_tester_proxy_t *aptima_env_tester_proxy_create(
    aptima_env_tester_t *aptima_env_tester, aptima_error_t *err);

aptima_RUNTIME_API bool aptima_env_tester_proxy_release(aptima_env_tester_proxy_t *self,
                                                  aptima_error_t *err);

aptima_RUNTIME_API bool aptima_env_tester_proxy_notify(
    aptima_env_tester_proxy_t *self,
    aptima_env_tester_proxy_notify_func_t notify_func, void *user_data,
    aptima_error_t *err);
