//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdbool.h>

#include "include_internal/ten_runtime/addon/addon.h"
#include "ten_utils/lib/error.h"

typedef struct ten_addon_store_t ten_addon_store_t;
typedef struct ten_addon_t ten_addon_t;
typedef struct ten_env_t ten_env_t;
typedef struct ten_extension_t ten_extension_t;

TEN_RUNTIME_PRIVATE_API ten_addon_store_t *ten_extension_get_global_store(void);

TEN_RUNTIME_PRIVATE_API void ten_addon_unregister_all_extension(void);

TEN_RUNTIME_PRIVATE_API bool ten_addon_create_extension(
    ten_env_t *ten_env, const char *addon_name, const char *instance_name,
    ten_env_addon_create_instance_done_cb_t cb, void *user_data,
    ten_error_t *err);

TEN_RUNTIME_PRIVATE_API bool ten_addon_destroy_extension(
    ten_env_t *ten_env, ten_extension_t *extension,
    ten_env_addon_destroy_instance_done_cb_t cb, void *user_data,
    ten_error_t *err);

TEN_RUNTIME_API ten_addon_t *ten_addon_unregister_extension(const char *name);

TEN_RUNTIME_PRIVATE_API void ten_addon_on_create_extension_instance_ctx_destroy(
    ten_addon_on_create_extension_instance_ctx_t *self);
