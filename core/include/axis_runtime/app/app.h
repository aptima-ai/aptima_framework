//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include <stdbool.h>

#include "aptima_utils/lib/error.h"

typedef struct aptima_app_t aptima_app_t;
typedef struct aptima_metadata_info_t aptima_metadata_info_t;
typedef struct aptima_env_t aptima_env_t;

typedef void (*aptima_app_on_configure_func_t)(aptima_app_t *app, aptima_env_t *aptima_env);

typedef void (*aptima_app_on_init_func_t)(aptima_app_t *app, aptima_env_t *aptima_env);

typedef void (*aptima_app_on_deinit_func_t)(aptima_app_t *app, aptima_env_t *aptima_env);

aptima_RUNTIME_API aptima_app_t *aptima_app_create(
    aptima_app_on_configure_func_t on_configure, aptima_app_on_init_func_t on_init,
    aptima_app_on_deinit_func_t on_deinit, aptima_error_t *err);

aptima_RUNTIME_API void aptima_app_destroy(aptima_app_t *self);

aptima_RUNTIME_API bool aptima_app_close(aptima_app_t *self, aptima_error_t *err);

aptima_RUNTIME_API bool aptima_app_run(aptima_app_t *self, bool run_in_background,
                                 aptima_error_t *err);

aptima_RUNTIME_API bool aptima_app_wait(aptima_app_t *self, aptima_error_t *err);

aptima_RUNTIME_API bool aptima_app_check_integrity(aptima_app_t *self,
                                             bool check_thread);

aptima_RUNTIME_API aptima_env_t *aptima_app_get_aptima_env(aptima_app_t *self);
