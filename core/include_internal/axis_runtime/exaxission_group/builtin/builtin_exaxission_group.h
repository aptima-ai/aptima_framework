//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/axis_config.h"

#include "axis_utils/container/list.h"
#include "axis_utils/macro/mark.h"

typedef struct axis_addon_t axis_addon_t;
typedef struct axis_env_t axis_env_t;

typedef struct axis_extension_group_create_extensions_done_ctx_t {
  axis_list_t results;
} axis_extension_group_create_extensions_done_ctx_t;

axis_RUNTIME_PRIVATE_API void axis_builtin_extension_group_addon_on_init(
    axis_UNUSED axis_addon_t *addon, axis_env_t *axis_env);

axis_RUNTIME_PRIVATE_API void axis_builtin_extension_group_addon_create_instance(
    axis_addon_t *addon, axis_env_t *axis_env, const char *name, void *context);

axis_RUNTIME_PRIVATE_API void axis_builtin_extension_group_addon_destroy_instance(
    axis_UNUSED axis_addon_t *addon, axis_env_t *axis_env, void *_extension_group,
    void *context);

axis_RUNTIME_PRIVATE_API void axis_builtin_extension_group_addon_register(void);

axis_RUNTIME_PRIVATE_API void axis_builtin_extension_group_addon_unregister(void);
