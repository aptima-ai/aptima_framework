//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdbool.h>

#include "ten_runtime/ten_env/ten_env.h"

TEN_RUNTIME_PRIVATE_API bool ten_is_cmd_connected_async(
    ten_env_t *self, const char *cmd_name,
    ten_env_is_cmd_connected_async_cb_t cb, void *cb_data, ten_error_t *err);
