//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/addon/addon.h"

typedef struct axis_env_t axis_env_t;

typedef struct
    axis_extension_context_on_addon_create_extension_group_done_ctx_t {
  axis_extension_group_t *extension_group;
  axis_addon_context_t *addon_context;
} axis_extension_context_on_addon_create_extension_group_done_ctx_t;

axis_extension_context_on_addon_create_extension_group_done_ctx_t *
axis_extension_context_on_addon_create_extension_group_done_ctx_create(void);

void axis_extension_context_on_addon_create_extension_group_done_ctx_destroy(
    axis_extension_context_on_addon_create_extension_group_done_ctx_t *self);

axis_RUNTIME_PRIVATE_API void
axis_extension_context_on_addon_create_extension_group_done(
    axis_env_t *self, void *instance, axis_addon_context_t *addon_context);

axis_RUNTIME_PRIVATE_API void
axis_extension_context_on_addon_destroy_extension_group_done(
    axis_env_t *self, axis_addon_context_t *addon_context);
