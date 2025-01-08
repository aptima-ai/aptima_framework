//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/binding/common.h"
#include "include_internal/axis_runtime/metadata/metadata.h"
#include "axis_runtime/binding/common.h"
#include "axis_utils/container/list.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/sanitizer/thread_check.h"

#define axis_EXTENSION_GROUP_SIGNATURE 0x94F72EDA6137DF04U

typedef struct axis_extension_t axis_extension_t;
typedef struct axis_extension_context_t axis_extension_context_t;
typedef struct axis_extension_group_t axis_extension_group_t;
typedef struct axis_extension_thread_t axis_extension_thread_t;
typedef struct axis_engine_t axis_engine_t;
typedef struct axis_metadata_info_t axis_metadata_info_t;
typedef struct axis_app_t axis_app_t;
typedef struct axis_addon_host_t axis_addon_host_t;
typedef struct axis_env_t axis_env_t;
typedef struct axis_extension_group_info_t axis_extension_group_info_t;

typedef void (*axis_extension_group_on_configure_func_t)(
    axis_extension_group_t *self, axis_env_t *axis_env);

typedef void (*axis_extension_group_on_init_func_t)(axis_extension_group_t *self,
                                                   axis_env_t *axis_env);

typedef void (*axis_extension_group_on_deinit_func_t)(
    axis_extension_group_t *self, axis_env_t *axis_env);

typedef void (*axis_extension_group_on_create_extensions_func_t)(
    axis_extension_group_t *self, axis_env_t *axis_env);

typedef void (*axis_extension_group_on_destroy_extensions_func_t)(
    axis_extension_group_t *self, axis_env_t *axis_env, axis_list_t extensions);

typedef enum axis_EXTENSION_GROUP_STATE {
  axis_EXTENSION_GROUP_STATE_INIT,
  axis_EXTENSION_GROUP_STATE_DEINITING,  // on_deinit() is started.
  axis_EXTENSION_GROUP_STATE_DEINITTED,  // on_deinit_done() is called.
} axis_EXTENSION_GROUP_STATE;

typedef struct axis_extension_group_t {
  axis_binding_handle_t binding_handle;

  axis_signature_t signature;
  axis_sanitizer_thread_check_t thread_check;

  axis_EXTENSION_GROUP_STATE state;

  // @{
  // Public interface
  axis_extension_group_on_configure_func_t on_configure;
  axis_extension_group_on_init_func_t on_init;
  axis_extension_group_on_deinit_func_t on_deinit;
  axis_extension_group_on_create_extensions_func_t on_create_extensions;
  axis_extension_group_on_destroy_extensions_func_t on_destroy_extensions;
  // @}

  axis_app_t *app;
  axis_extension_context_t *extension_context;
  axis_extension_thread_t *extension_thread;

  axis_env_t *axis_env;

  axis_addon_host_t *addon_host;
  axis_string_t name;

  axis_extension_group_info_t *extension_group_info;

  // axis_extension_addon_and_instance_name_pair_t
  axis_list_t extension_addon_and_instance_name_pairs;

  // The error encountered in the extension thread before completing all
  // initializations (i.e., before starting the lifecycle stages of all included
  // extensions). For example, a failure to create extensions.
  axis_error_t err_before_ready;

  axis_value_t manifest;
  axis_value_t property;

  axis_metadata_info_t *manifest_info;
  axis_metadata_info_t *property_info;

  size_t extensions_cnt_of_being_destroyed;
} axis_extension_group_t;

axis_RUNTIME_API bool axis_extension_group_check_integrity(
    axis_extension_group_t *self, bool check_thread);

axis_RUNTIME_PRIVATE_API axis_extension_group_t *axis_extension_group_create(
    const char *name, axis_extension_group_on_configure_func_t on_configure,
    axis_extension_group_on_init_func_t on_init,
    axis_extension_group_on_deinit_func_t on_deinit,
    axis_extension_group_on_create_extensions_func_t on_create_extensions,
    axis_extension_group_on_destroy_extensions_func_t on_destroy_extensions);

axis_RUNTIME_PRIVATE_API void axis_extension_group_destroy(
    axis_extension_group_t *self);

axis_RUNTIME_PRIVATE_API axis_env_t *axis_extension_group_get_axis_env(
    axis_extension_group_t *self);

axis_RUNTIME_PRIVATE_API void axis_extension_group_create_extensions(
    axis_extension_group_t *self);

axis_RUNTIME_PRIVATE_API void axis_extension_group_destroy_extensions(
    axis_extension_group_t *self, axis_list_t extensions);

axis_RUNTIME_PRIVATE_API void axis_extension_group_set_addon(
    axis_extension_group_t *self, axis_addon_host_t *addon_host);

axis_RUNTIME_PRIVATE_API axis_shared_ptr_t *
axis_extension_group_create_invalid_dest_status(axis_shared_ptr_t *origin_cmd,
                                               axis_string_t *target_group_name);

axis_RUNTIME_PRIVATE_API axis_runloop_t *axis_extension_group_get_attached_runloop(
    axis_extension_group_t *self);

axis_RUNTIME_PRIVATE_API void axis_extension_group_on_init_done(axis_env_t *self);

axis_RUNTIME_PRIVATE_API void axis_extension_group_on_deinit_done(
    axis_env_t *self);

axis_RUNTIME_PRIVATE_API void axis_extension_group_on_destroy_extensions_done(
    axis_extension_group_t *self);

axis_RUNTIME_PRIVATE_API void axis_extension_group_on_create_extensions_done(
    axis_extension_group_t *self, axis_list_t *extensions);

axis_RUNTIME_PRIVATE_API axis_list_t *
axis_extension_group_get_extension_addon_and_instance_name_pairs(
    axis_extension_group_t *self);

axis_RUNTIME_PRIVATE_API void
axis_extension_group_set_extension_cnt_of_being_destroyed(
    axis_extension_group_t *self, size_t new_cnt);

axis_RUNTIME_PRIVATE_API size_t
axis_extension_group_decrement_extension_cnt_of_being_destroyed(
    axis_extension_group_t *self);

axis_RUNTIME_PRIVATE_API axis_extension_group_t *
axis_extension_group_create_internal(
    const char *name, axis_extension_group_on_configure_func_t on_configure,
    axis_extension_group_on_init_func_t on_init,
    axis_extension_group_on_deinit_func_t on_deinit,
    axis_extension_group_on_create_extensions_func_t on_create_extensions,
    axis_extension_group_on_destroy_extensions_func_t on_destroy_extensions);
