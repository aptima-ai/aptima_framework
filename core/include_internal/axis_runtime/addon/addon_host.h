//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/metadata/metadata_info.h"
#include "axis_runtime/addon/addon.h"
#include "axis_utils/lib/ref.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/string.h"

#define axis_ADDON_HOST_SIGNATURE 0x44FAE6B3F920A44EU

typedef struct axis_addon_t axis_addon_t;
typedef struct axis_addon_store_t axis_addon_store_t;

typedef struct axis_addon_host_t {
  axis_signature_t signature;

  axis_string_t name;  // The name of the addon.
  axis_string_t base_dir;

  axis_value_t manifest;
  axis_value_t property;

  axis_metadata_info_t *manifest_info;
  axis_metadata_info_t *property_info;

  axis_addon_t *addon;
  axis_addon_store_t *store;

  axis_ref_t ref;  // Used to control the timing of addon destruction.
  axis_env_t *axis_env;

  axis_ADDON_TYPE type;

  void *user_data;
} axis_addon_host_t;

typedef struct axis_addon_host_on_destroy_instance_ctx_t {
  axis_addon_host_t *addon_host;
  void *instance;
  axis_env_addon_destroy_instance_done_cb_t cb;
  void *cb_data;
} axis_addon_host_on_destroy_instance_ctx_t;

axis_RUNTIME_PRIVATE_API void axis_addon_host_init(axis_addon_host_t *self);

axis_RUNTIME_API void axis_addon_host_destroy(axis_addon_host_t *self);

axis_RUNTIME_PRIVATE_API void axis_addon_host_find_and_set_base_dir(
    axis_addon_host_t *self, const char *path);

axis_RUNTIME_PRIVATE_API bool axis_addon_host_destroy_instance_async(
    axis_addon_host_t *self, axis_env_t *axis_env, void *instance,
    axis_env_addon_destroy_instance_done_cb_t cb, void *cb_data);

axis_RUNTIME_PRIVATE_API void axis_addon_host_create_instance_async(
    axis_addon_host_t *self, axis_env_t *axis_env, const char *name,
    axis_env_addon_create_instance_done_cb_t cb, void *cb_data);

axis_RUNTIME_PRIVATE_API bool axis_addon_host_destroy_instance(
    axis_addon_host_t *self, axis_env_t *axis_env, void *instance);

/**
 * @brief The base directory of the loaded addon. This function can be
 * called before any TEN app starts. Note that the returned string must be
 * destroyed by users.
 */
axis_RUNTIME_PRIVATE_API const char *axis_addon_host_get_base_dir(
    axis_addon_host_t *self);

axis_RUNTIME_PRIVATE_API bool axis_addon_host_check_integrity(
    axis_addon_host_t *self);

axis_RUNTIME_PRIVATE_API const char *axis_addon_host_get_name(
    axis_addon_host_t *self);

axis_RUNTIME_PRIVATE_API axis_addon_host_on_destroy_instance_ctx_t *
axis_addon_host_on_destroy_instance_ctx_create(
    axis_addon_host_t *self, void *instance,
    axis_env_addon_destroy_instance_done_cb_t cb, void *cb_data);

axis_RUNTIME_PRIVATE_API void axis_addon_host_on_destroy_instance_ctx_destroy(
    axis_addon_host_on_destroy_instance_ctx_t *self);

axis_RUNTIME_PRIVATE_API axis_addon_host_t *axis_addon_host_create(
    axis_ADDON_TYPE type);

axis_RUNTIME_PRIVATE_API axis_addon_host_t *axis_addon_host_find(
    axis_ADDON_TYPE addon_type, const char *addon_name);

axis_RUNTIME_PRIVATE_API axis_addon_host_t *
axis_addon_host_find_or_create_one_if_not_found(axis_ADDON_TYPE addon_type,
                                               const char *addon_name,
                                               bool *newly_created);

axis_RUNTIME_PRIVATE_API void axis_addon_host_load_metadata(
    axis_addon_host_t *self, axis_env_t *axis_env,
    axis_addon_on_init_func_t on_init);
