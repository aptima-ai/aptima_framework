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
#include "axis_utils/lib/error.h"

typedef struct axis_addon_store_t axis_addon_store_t;
typedef struct axis_addon_t axis_addon_t;
typedef struct axis_env_t axis_env_t;
typedef struct axis_extension_t axis_extension_t;
typedef struct axis_extension_group_create_extensions_done_ctx_t
    axis_extension_group_create_extensions_done_ctx_t;

typedef struct axis_addon_create_extension_done_ctx_t {
  axis_string_t extension_name;
  axis_extension_group_create_extensions_done_ctx_t *create_extensions_done_ctx;
} axis_addon_create_extension_done_ctx_t;

axis_RUNTIME_PRIVATE_API axis_addon_store_t *axis_extension_get_global_store(void);

axis_RUNTIME_PRIVATE_API void axis_addon_unregister_all_extension(void);

axis_RUNTIME_PRIVATE_API axis_addon_create_extension_done_ctx_t *
axis_addon_create_extension_done_ctx_create(
    const char *extension_name,
    axis_extension_group_create_extensions_done_ctx_t *ctx);

axis_RUNTIME_PRIVATE_API void axis_addon_create_extension_done_ctx_destroy(
    axis_addon_create_extension_done_ctx_t *self);

axis_RUNTIME_PRIVATE_API bool axis_addon_create_extension(
    axis_env_t *axis_env, const char *addon_name, const char *instance_name,
    axis_env_addon_create_instance_done_cb_t cb, void *user_data,
    axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_addon_destroy_extension(
    axis_env_t *axis_env, axis_extension_t *extension,
    axis_env_addon_destroy_instance_done_cb_t cb, void *user_data,
    axis_error_t *err);

axis_RUNTIME_API axis_addon_t *axis_addon_unregister_extension(const char *name);

axis_RUNTIME_PRIVATE_API void axis_addon_on_create_extension_instance_ctx_destroy(
    axis_addon_on_create_extension_instance_ctx_t *self);
