//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include <stdbool.h>
#include <stddef.h>

#include "aptima_runtime/aptima_env/aptima_env.h"
#include "aptima_utils/lib/error.h"

typedef struct aptima_env_proxy_t aptima_env_proxy_t;
typedef struct aptima_env_t aptima_env_t;

typedef void (*aptima_env_proxy_notify_func_t)(aptima_env_t *aptima_env,
                                            void *user_data);

aptima_RUNTIME_API aptima_env_proxy_t *aptima_env_proxy_create(aptima_env_t *aptima_env,
                                                      size_t initial_thread_cnt,
                                                      aptima_error_t *err);

aptima_RUNTIME_API bool aptima_env_proxy_release(aptima_env_proxy_t *self,
                                           aptima_error_t *err);

aptima_RUNTIME_API bool aptima_env_proxy_notify(
    aptima_env_proxy_t *self, aptima_env_proxy_notify_func_t notify_func,
    void *user_data, bool sync, aptima_error_t *err);

aptima_RUNTIME_API bool aptima_env_proxy_acquire_lock_mode(aptima_env_proxy_t *self,
                                                     aptima_error_t *err);

aptima_RUNTIME_API bool aptima_env_proxy_release_lock_mode(aptima_env_proxy_t *self,
                                                     aptima_error_t *err);
