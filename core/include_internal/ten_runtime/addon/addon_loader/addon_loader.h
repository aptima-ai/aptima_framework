//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "include_internal/ten_runtime/addon/addon.h"
#include "ten_runtime/addon/addon.h"

typedef struct ten_addon_host_t ten_addon_host_t;
typedef struct ten_addon_store_t ten_addon_store_t;

#define TEN_REGISTER_ADDON_AS_ADDON_LOADER(ADDON_LOADER_NAME, ADDON) \
  TEN_ADDON_REGISTER(addon_loader, ADDON_LOADER_NAME, ADDON)

TEN_RUNTIME_PRIVATE_API ten_addon_store_t *ten_addon_loader_get_global_store(
    void);

TEN_RUNTIME_PRIVATE_API ten_addon_host_t *ten_addon_addon_loader_find(
    const char *addon_loader);

TEN_RUNTIME_API ten_addon_host_t *ten_addon_register_addon_loader(
    const char *name, const char *base_dir, ten_addon_t *addon,
    void *register_ctx);

TEN_RUNTIME_API ten_addon_t *ten_addon_unregister_addon_loader(
    const char *name);

TEN_RUNTIME_API void ten_addon_unregister_all_addon_loader(void);

TEN_RUNTIME_PRIVATE_API bool ten_addon_create_addon_loader(
    ten_env_t *ten_env, const char *addon_name, const char *instance_name,
    ten_env_addon_create_instance_done_cb_t cb, void *cb_data,
    ten_error_t *err);

TEN_RUNTIME_PRIVATE_API bool ten_addon_loader_create_singleton(
    ten_env_t *ten_env);

TEN_RUNTIME_PRIVATE_API bool ten_addon_loader_destroy_singleton(void);
