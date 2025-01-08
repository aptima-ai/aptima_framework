//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

typedef struct axis_env_t axis_env_t;
typedef struct axis_env_proxy_t axis_env_proxy_t;

axis_RUNTIME_PRIVATE_API void axis_env_add_axis_proxy(
    axis_env_t *self, axis_env_proxy_t *axis_env_proxy);

axis_RUNTIME_PRIVATE_API void axis_env_delete_axis_proxy(
    axis_env_t *self, axis_env_proxy_t *axis_env_proxy);
