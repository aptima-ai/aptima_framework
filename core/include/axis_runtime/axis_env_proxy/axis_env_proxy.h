//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>
#include <stddef.h>

#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/lib/error.h"

typedef struct axis_env_proxy_t axis_env_proxy_t;
typedef struct axis_env_t axis_env_t;

typedef void (*axis_env_proxy_notify_func_t)(axis_env_t *axis_env,
                                            void *user_data);

axis_RUNTIME_API axis_env_proxy_t *axis_env_proxy_create(axis_env_t *axis_env,
                                                      size_t initial_thread_cnt,
                                                      axis_error_t *err);

axis_RUNTIME_API bool axis_env_proxy_release(axis_env_proxy_t *self,
                                           axis_error_t *err);

axis_RUNTIME_API bool axis_env_proxy_notify(
    axis_env_proxy_t *self, axis_env_proxy_notify_func_t notify_func,
    void *user_data, bool sync, axis_error_t *err);

axis_RUNTIME_API bool axis_env_proxy_acquire_lock_mode(axis_env_proxy_t *self,
                                                     axis_error_t *err);

axis_RUNTIME_API bool axis_env_proxy_release_lock_mode(axis_env_proxy_t *self,
                                                     axis_error_t *err);
