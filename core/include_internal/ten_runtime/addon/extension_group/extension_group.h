//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdbool.h>

#include "ten_runtime/ten_env/ten_env.h"

typedef struct ten_env_t ten_env_t;
typedef struct ten_addon_store_t ten_addon_store_t;

TEN_RUNTIME_PRIVATE_API ten_addon_store_t *ten_extension_group_get_store(void);

TEN_RUNTIME_API bool ten_addon_extension_group_create(
    ten_env_t *ten_env, const char *addon_name, const char *instance_name,
    ten_env_addon_on_create_instance_async_cb_t cb, void *user_data);

TEN_RUNTIME_API bool ten_addon_extension_group_destroy(
    ten_env_t *ten_env, ten_extension_group_t *extension_group,
    ten_env_addon_on_destroy_instance_async_cb_t cb, void *user_data);
