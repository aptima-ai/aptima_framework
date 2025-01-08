//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "axis_utils/lib/error.h"

typedef struct axis_app_t axis_app_t;
typedef struct axis_metadata_info_t axis_metadata_info_t;
typedef struct axis_env_t axis_env_t;

typedef void (*axis_app_on_configure_func_t)(axis_app_t *app, axis_env_t *axis_env);

typedef void (*axis_app_on_init_func_t)(axis_app_t *app, axis_env_t *axis_env);

typedef void (*axis_app_on_deinit_func_t)(axis_app_t *app, axis_env_t *axis_env);

axis_RUNTIME_API axis_app_t *axis_app_create(
    axis_app_on_configure_func_t on_configure, axis_app_on_init_func_t on_init,
    axis_app_on_deinit_func_t on_deinit, axis_error_t *err);

axis_RUNTIME_API void axis_app_destroy(axis_app_t *self);

axis_RUNTIME_API bool axis_app_close(axis_app_t *self, axis_error_t *err);

axis_RUNTIME_API bool axis_app_run(axis_app_t *self, bool run_in_background,
                                 axis_error_t *err);

axis_RUNTIME_API bool axis_app_wait(axis_app_t *self, axis_error_t *err);

axis_RUNTIME_API bool axis_app_check_integrity(axis_app_t *self,
                                             bool check_thread);

axis_RUNTIME_API axis_env_t *axis_app_get_axis_env(axis_app_t *self);
