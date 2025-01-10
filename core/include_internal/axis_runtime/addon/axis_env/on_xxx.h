//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_runtime/axis_env/axis_env.h"

axis_RUNTIME_PRIVATE_API void axis_addon_on_init_done(axis_env_t *self);

axis_RUNTIME_PRIVATE_API void axis_addon_on_deinit_done(axis_env_t *self);

axis_RUNTIME_PRIVATE_API void axis_addon_on_create_instance_done(axis_env_t *self,
                                                               void *instance,
                                                               void *context);

axis_RUNTIME_PRIVATE_API void axis_addon_on_destroy_instance_done(axis_env_t *self,
                                                                void *context);
