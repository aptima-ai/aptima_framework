//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdbool.h>

#include "ten_utils/container/list.h"

typedef struct ten_extension_group_t ten_extension_group_t;
typedef struct ten_metadata_info_t ten_metadata_info_t;
typedef struct ten_env_t ten_env_t;

typedef void (*ten_extension_group_on_configure_func_t)(
    ten_extension_group_t *self, ten_env_t *ten_env);

typedef void (*ten_extension_group_on_init_func_t)(ten_extension_group_t *self,
                                                   ten_env_t *ten_env);

typedef void (*ten_extension_group_on_deinit_func_t)(
    ten_extension_group_t *self, ten_env_t *ten_env);

typedef void (*ten_extension_group_on_create_extensions_func_t)(
    ten_extension_group_t *self, ten_env_t *ten_env);

typedef void (*ten_extension_group_on_destroy_extensions_func_t)(
    ten_extension_group_t *self, ten_env_t *ten_env, ten_list_t extensions);

TEN_RUNTIME_API bool ten_extension_group_check_integrity(
    ten_extension_group_t *self, bool check_thread);

TEN_RUNTIME_API ten_extension_group_t *ten_extension_group_create(
    const char *name, ten_extension_group_on_configure_func_t on_configure,
    ten_extension_group_on_init_func_t on_init,
    ten_extension_group_on_deinit_func_t on_deinit,
    ten_extension_group_on_create_extensions_func_t on_create_extensions,
    ten_extension_group_on_destroy_extensions_func_t on_destroy_extensions);

TEN_RUNTIME_API void ten_extension_group_destroy(ten_extension_group_t *self);

TEN_RUNTIME_API ten_env_t *ten_extension_group_get_ten_env(
    ten_extension_group_t *self);
