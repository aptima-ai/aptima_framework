//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/addon/addon.h"
#include "axis_runtime/addon/addon.h"

typedef struct axis_addon_host_t axis_addon_host_t;
typedef struct axis_addon_store_t axis_addon_store_t;

#define axis_REGISTER_ADDON_AS_ADDON_LOADER(ADDON_LOADER_NAME, ADDON) \
  axis_ADDON_REGISTER(addon_loader, ADDON_LOADER_NAME, ADDON)

axis_RUNTIME_PRIVATE_API axis_addon_store_t *axis_addon_loader_get_global_store(
    void);

axis_RUNTIME_PRIVATE_API axis_addon_host_t *axis_addon_addon_loader_find(
    const char *addon_loader);

axis_RUNTIME_API axis_addon_host_t *axis_addon_register_addon_loader(
    const char *name, const char *base_dir, axis_addon_t *addon,
    void *register_ctx);

axis_RUNTIME_API axis_addon_t *axis_addon_unregister_addon_loader(
    const char *name);

axis_RUNTIME_PRIVATE_API void axis_addon_unregister_all_addon_loader(void);

axis_RUNTIME_PRIVATE_API bool axis_addon_create_addon_loader(
    axis_env_t *axis_env, const char *addon_name, const char *instance_name,
    axis_env_addon_create_instance_done_cb_t cb, void *cb_data,
    axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_addon_loader_addons_create_singleton_instance(
    axis_env_t *axis_env);

axis_RUNTIME_PRIVATE_API void axis_addon_loader_addons_destroy_singleton_instance(
    void);
