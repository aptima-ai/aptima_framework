//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/binding/common.h"
#include "axis_runtime/addon/addon.h"
#include "axis_utils/lib/signature.h"

#define axis_ADDON_LOADER_SIGNATURE 0xAE4FCDE7983727E4U

typedef struct axis_addon_loader_t axis_addon_loader_t;
typedef struct axis_addon_host_t axis_addon_host_t;

typedef void (*axis_addon_loader_on_init_func_t)(
    axis_addon_loader_t *addon_loader);

typedef void (*axis_addon_loader_on_deinit_func_t)(
    axis_addon_loader_t *addon_loader);

typedef void (*axis_addon_loader_on_load_addon_func_t)(
    axis_addon_loader_t *addon_loader, axis_ADDON_TYPE addon_type,
    const char *addon_name);

typedef struct axis_addon_loader_t {
  axis_binding_handle_t binding_handle;
  axis_signature_t signature;

  axis_addon_host_t *addon_host;

  axis_addon_loader_on_init_func_t on_init;
  axis_addon_loader_on_deinit_func_t on_deinit;
  axis_addon_loader_on_load_addon_func_t on_load_addon;
} axis_addon_loader_t;

axis_RUNTIME_PRIVATE_API axis_list_t *axis_addon_loader_get_all(void);

axis_RUNTIME_PRIVATE_API bool axis_addon_loader_check_integrity(
    axis_addon_loader_t *self);

axis_RUNTIME_API axis_addon_loader_t *axis_addon_loader_create(
    axis_addon_loader_on_init_func_t on_init,
    axis_addon_loader_on_deinit_func_t on_deinit,
    axis_addon_loader_on_load_addon_func_t on_load_addon);

axis_RUNTIME_API void axis_addon_loader_destroy(axis_addon_loader_t *self);

axis_RUNTIME_PRIVATE_API void axis_addon_loader_load_addon(
    axis_addon_loader_t *self, axis_ADDON_TYPE addon_type,
    const char *addon_name);
