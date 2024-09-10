//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdbool.h>

#include "ten_runtime/ten_env/ten_env.h"

#define TEN_REGISTER_ADDON_AS_EXTENSION(NAME, ADDON) \
  TEN_ADDON_REGISTER(extension, NAME, ADDON)

typedef struct ten_env_t ten_env_t;
typedef struct ten_addon_t ten_addon_t;
typedef struct ten_extension_t ten_extension_t;
typedef struct ten_addon_host_t ten_addon_host_t;

TEN_RUNTIME_API ten_addon_host_t *ten_addon_register_extension(
    const char *name, ten_addon_t *addon);

TEN_RUNTIME_API void ten_addon_unregister_extension(const char *name,
                                                  ten_addon_t *addon);

TEN_RUNTIME_API ten_extension_t *ten_addon_create_extension(
    ten_env_t *ten_env, const char *addon_name, const char *instance_name,
    ten_error_t *err);

TEN_RUNTIME_API bool ten_addon_create_extension_async(
    ten_env_t *ten_env, const char *addon_name, const char *instance_name,
    ten_env_addon_on_create_instance_async_cb_t cb, void *user_data,
    ten_error_t *err);

TEN_RUNTIME_API bool ten_addon_create_extension_async_for_mock(
    ten_env_t *ten_env, const char *addon_name, const char *instance_name,
    ten_env_addon_on_create_instance_async_cb_t cb, void *cb_data,
    ten_error_t *err);

TEN_RUNTIME_API bool ten_addon_destroy_extension(ten_env_t *ten_env,
                                               ten_extension_t *extension,
                                               ten_error_t *err);

TEN_RUNTIME_API bool ten_addon_destroy_extension_async(
    ten_env_t *ten_env, ten_extension_t *extension,
    ten_env_addon_on_destroy_instance_async_cb_t cb, void *user_data,
    ten_error_t *err);

TEN_RUNTIME_API bool ten_addon_destroy_extension_async_for_mock(
    ten_env_t *ten_env, ten_extension_t *extension,
    ten_env_addon_on_destroy_instance_async_cb_t cb, void *cb_data,
    ten_error_t *err);
