//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "axis_utils/lib/error.h"

typedef struct axis_env_t axis_env_t;
typedef struct axis_extension_group_create_extensions_done_ctx_t
    axis_extension_group_create_extensions_done_ctx_t;

axis_RUNTIME_PRIVATE_API bool axis_env_on_create_extensions_done(
    axis_env_t *self, axis_extension_group_create_extensions_done_ctx_t *ctx,
    axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_env_on_destroy_extensions_done(
    axis_env_t *self, axis_error_t *err);
