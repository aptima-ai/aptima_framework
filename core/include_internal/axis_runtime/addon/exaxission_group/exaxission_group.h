//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/addon/addon.h"
#include "axis_runtime/axis_env/axis_env.h"

typedef struct axis_env_t axis_env_t;
typedef struct axis_addon_store_t axis_addon_store_t;
typedef struct axis_addon_t axis_addon_t;
typedef struct axis_addon_host_t axis_addon_host_t;

#define axis_REGISTER_ADDON_AS_EXTENSION_GROUP(NAME, ADDON) \
  axis_ADDON_REGISTER(extension_group, NAME, ADDON)

axis_RUNTIME_PRIVATE_API axis_addon_store_t *axis_extension_group_get_global_store(
    void);

axis_RUNTIME_PRIVATE_API bool axis_addon_create_extension_group(
    axis_env_t *axis_env, const char *addon_name, const char *instance_name,
    axis_env_addon_create_instance_done_cb_t cb, void *user_data);

axis_RUNTIME_PRIVATE_API bool axis_addon_destroy_extension_group(
    axis_env_t *axis_env, axis_extension_group_t *extension_group,
    axis_env_addon_destroy_instance_done_cb_t cb, void *user_data);

axis_RUNTIME_PRIVATE_API axis_addon_host_t *axis_addon_register_extension_group(
    const char *name, const char *base_dir, axis_addon_t *addon,
    void *register_ctx);

axis_RUNTIME_PRIVATE_API axis_addon_t *axis_addon_unregister_extension_group(
    const char *name);

axis_RUNTIME_PRIVATE_API void axis_addon_unregister_all_extension_group(void);
