//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

typedef struct ten_env_t ten_env_t;

TEN_RUNTIME_PRIVATE_API void ten_builtin_test_extension_addon_register(void);

TEN_RUNTIME_PRIVATE_API void ten_builtin_test_extension_addon_unregister(void);

TEN_RUNTIME_PRIVATE_API void
ten_builtin_test_extension_ten_env_notify_on_start_done(ten_env_t *ten_env,
                                                        void *user_data);
