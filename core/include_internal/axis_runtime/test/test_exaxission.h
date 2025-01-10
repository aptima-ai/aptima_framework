//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

typedef struct axis_env_t axis_env_t;

axis_RUNTIME_PRIVATE_API void axis_builtin_test_extension_addon_register(void);

axis_RUNTIME_PRIVATE_API void axis_builtin_test_extension_addon_unregister(void);

axis_RUNTIME_PRIVATE_API void
axis_builtin_test_extension_axis_env_notify_on_start_done(axis_env_t *axis_env,
                                                        void *user_data);
