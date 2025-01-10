//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/addon/addon.h"
#include "axis_utils/lib/error.h"

typedef struct axis_app_t axis_app_t;

typedef void (*axis_addon_register_func_t)(void *register_ctx);

axis_RUNTIME_PRIVATE_API bool axis_addon_load_all_from_app_base_dir(
    const char *app_base_dir, axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_addon_load_all_from_axis_package_base_dirs(
    axis_list_t *axis_package_base_dirs, axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool
axis_addon_try_load_specific_addon_using_native_addon_loader(
    const char *app_base_dir, axis_ADDON_TYPE addon_type,
    const char *addon_name);

axis_RUNTIME_PRIVATE_API bool
axis_addon_try_load_specific_addon_using_all_addon_loaders(
    axis_ADDON_TYPE addon_type, const char *addon_name);
