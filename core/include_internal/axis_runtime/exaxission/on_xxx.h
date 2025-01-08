//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_runtime/axis_env/axis_env.h"

axis_RUNTIME_PRIVATE_API bool axis_extension_on_configure_done(axis_env_t *self);

axis_RUNTIME_PRIVATE_API bool axis_extension_on_init_done(axis_env_t *self);

axis_RUNTIME_PRIVATE_API bool axis_extension_on_start_done(axis_env_t *self);

axis_RUNTIME_PRIVATE_API bool axis_extension_on_stop_done(axis_env_t *self);

axis_RUNTIME_PRIVATE_API bool axis_extension_on_deinit_done(axis_env_t *self);
