//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/addon/common/store.h"
#include "include_internal/axis_runtime/binding/common.h"
#include "axis_runtime/addon/addon.h"
#include "axis_runtime/binding/common.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/lib/signature.h"

#define axis_ADDON_SIGNATURE 0xDB9CA797E07377D4U

typedef struct axis_app_t axis_app_t;

typedef void (*axis_env_addon_create_instance_done_cb_t)(axis_env_t *axis_env,
                                                        void *instance,
                                                        void *cb_data);

typedef void (*axis_env_addon_destroy_instance_done_cb_t)(axis_env_t *axis_env,
                                                         void *cb_data);

typedef struct axis_addon_context_t {
  axis_env_t *caller_axis_env;

  axis_env_addon_create_instance_done_cb_t create_instance_done_cb;
  void *create_instance_done_cb_data;

  axis_env_addon_destroy_instance_done_cb_t destroy_instance_done_cb;
  void *destroy_instance_done_cb_data;
} axis_addon_context_t;

typedef struct axis_addon_t {
  axis_binding_handle_t binding_handle;

  axis_signature_t signature;

  axis_addon_on_init_func_t on_init;
  axis_addon_on_deinit_func_t on_deinit;

  axis_addon_on_create_instance_func_t on_create_instance;
  axis_addon_on_destroy_instance_func_t on_destroy_instance;

  axis_addon_on_destroy_func_t on_destroy;

  void *user_data;
} axis_addon_t;

typedef struct axis_addon_on_create_extension_instance_ctx_t {
  axis_string_t addon_name;
  axis_string_t instance_name;
  axis_ADDON_TYPE addon_type;  // Used to retrieve addon from the correct store.
  axis_env_addon_create_instance_done_cb_t cb;
  void *cb_data;
} axis_addon_on_create_extension_instance_ctx_t;

axis_RUNTIME_API bool axis_addon_check_integrity(axis_addon_t *self);

axis_RUNTIME_PRIVATE_API axis_ADDON_TYPE
axis_addon_type_from_string(const char *addon_type_str);

axis_RUNTIME_PRIVATE_API axis_addon_t *axis_addon_unregister(
    axis_addon_store_t *store, const char *addon_name);

axis_RUNTIME_API void axis_unregister_all_addons_and_cleanup(void);

axis_RUNTIME_PRIVATE_API axis_addon_store_t *axis_addon_get_store(void);

axis_RUNTIME_PRIVATE_API bool axis_addon_create_instance_async(
    axis_env_t *axis_env, axis_ADDON_TYPE addon_type, const char *addon_name,
    const char *instance_name, axis_env_addon_create_instance_done_cb_t cb,
    void *cb_data);

axis_RUNTIME_API const char *axis_addon_type_to_string(axis_ADDON_TYPE type);

axis_RUNTIME_PRIVATE_API void axis_addon_context_destroy(
    axis_addon_context_t *self);

axis_RUNTIME_PRIVATE_API axis_addon_host_t *axis_addon_register(
    axis_ADDON_TYPE addon_type, const char *name, const char *base_dir,
    axis_addon_t *addon, void *register_ctx);

axis_RUNTIME_API void axis_addon_init(
    axis_addon_t *self, axis_addon_on_init_func_t on_init,
    axis_addon_on_deinit_func_t on_deinit,
    axis_addon_on_create_instance_func_t on_create_instance,
    axis_addon_on_destroy_instance_func_t on_destroy_instance,
    axis_addon_on_destroy_func_t on_destroy);
