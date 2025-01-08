//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/addon/addon.h"
#include "axis_runtime/axis_env/axis_env.h"

axis_RUNTIME_PRIVATE_API void axis_extension_group_on_init(axis_env_t *axis_env);

axis_RUNTIME_PRIVATE_API void axis_extension_group_on_deinit(
    axis_extension_group_t *self);

axis_RUNTIME_PRIVATE_API void axis_extension_group_on_addon_create_extension_done(
    axis_env_t *self, void *instance, axis_addon_context_t *addon_context);

axis_RUNTIME_PRIVATE_API void
axis_extension_group_on_addon_destroy_extension_done(
    axis_env_t *self, axis_addon_context_t *addon_context);

axis_RUNTIME_PRIVATE_API const char *axis_extension_group_get_name(
    axis_extension_group_t *self, bool check_thread);
