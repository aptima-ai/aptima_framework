//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "include_internal/ten_runtime/binding/common.h"
#include "ten_runtime/addon/addon.h"

typedef struct ten_addon_loader_t ten_addon_loader_t;
typedef struct ten_addon_host_t ten_addon_host_t;

typedef void (*ten_addon_loader_on_init_func_t)(
    ten_addon_loader_t *addon_loader);

typedef void (*ten_addon_loader_on_deinit_func_t)(
    ten_addon_loader_t *addon_loader);

typedef void (*ten_addon_loader_on_load_addon_func_t)(
    ten_addon_loader_t *addon_loader, TEN_ADDON_TYPE addon_type,
    const char *addon_name);

typedef struct ten_addon_loader_t {
  ten_binding_handle_t binding_handle;

  ten_addon_host_t *addon_host;

  ten_addon_loader_on_init_func_t on_init;
  ten_addon_loader_on_deinit_func_t on_deinit;
  ten_addon_loader_on_load_addon_func_t on_load_addon;
} ten_addon_loader_t;

TEN_RUNTIME_PRIVATE_API ten_list_t *ten_addon_loader_get_global(void);

TEN_RUNTIME_API ten_addon_loader_t *ten_addon_loader_create(
    ten_addon_loader_on_init_func_t on_init,
    ten_addon_loader_on_deinit_func_t on_deinit,
    ten_addon_loader_on_load_addon_func_t on_load_addon);

TEN_RUNTIME_API void ten_addon_loader_destroy(ten_addon_loader_t *addon_loader);

TEN_RUNTIME_PRIVATE_API void ten_addon_loader_init(
    ten_addon_loader_t *addon_loader);

TEN_RUNTIME_PRIVATE_API void ten_addon_loader_deinit(
    ten_addon_loader_t *addon_loaderv);

TEN_RUNTIME_PRIVATE_API void ten_addon_loader_load_addon(
    ten_addon_loader_t *addon_loader, TEN_ADDON_TYPE addon_type,
    const char *addon_name);
