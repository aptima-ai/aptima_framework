//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/axis_env/axis_env.h"

extern inline axis_extension_t *axis_env_get_attached_extension(axis_env_t *self);

extern inline axis_extension_group_t *axis_env_get_attached_extension_group(
    axis_env_t *self);

extern inline axis_app_t *axis_env_get_attached_app(axis_env_t *self);

extern inline axis_addon_host_t *axis_env_get_attached_addon(axis_env_t *self);

extern inline axis_engine_t *axis_env_get_attached_engine(axis_env_t *self);
