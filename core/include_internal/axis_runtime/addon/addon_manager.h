//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/addon/addon.h"
#include "axis_runtime/addon/addon_manager.h"
#include "axis_utils/container/list.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/lib/string.h"

typedef struct axis_app_t axis_app_t;

typedef struct axis_addon_registration_t {
  axis_ADDON_TYPE addon_type;
  axis_string_t addon_name;
  axis_addon_registration_func_t func;
} axis_addon_registration_t;

typedef struct axis_addon_register_ctx_t {
  axis_app_t *app;
} axis_addon_register_ctx_t;

typedef struct axis_addon_manager_t {
  // Define a registry map to store addon registration functions.
  // The key is the addon name (string), and the value is a function that takes
  // a register_ctx.
  axis_list_t registry;  // axis_addon_registration_t*
  axis_mutex_t *mutex;
} axis_addon_manager_t;

axis_RUNTIME_API void axis_addon_manager_register_all_addons(
    axis_addon_manager_t *self, void *register_ctx);

axis_RUNTIME_API bool axis_addon_manager_register_specific_addon(
    axis_addon_manager_t *self, axis_ADDON_TYPE addon_type,
    const char *addon_name, void *register_ctx);

axis_RUNTIME_PRIVATE_API axis_addon_register_ctx_t *axis_addon_register_ctx_create(
    void);

axis_RUNTIME_PRIVATE_API void axis_addon_register_ctx_destroy(
    axis_addon_register_ctx_t *self);
