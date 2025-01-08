//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_runtime/common/errno.h"                  // IWYU pragma: keep
#include "axis_runtime/axis_env/internal/log.h"          // IWYU pragma: keep
#include "axis_runtime/axis_env/internal/metadata.h"     // IWYU pragma: keep
#include "axis_runtime/axis_env/internal/on_xxx_done.h"  // IWYU pragma: keep
#include "axis_runtime/axis_env/internal/return.h"       // IWYU pragma: keep
#include "axis_runtime/axis_env/internal/send.h"         // IWYU pragma: keep

typedef struct axis_env_t axis_env_t;
typedef struct axis_extension_group_t axis_extension_group_t;

axis_RUNTIME_API bool axis_env_check_integrity(axis_env_t *self,
                                             bool check_thread);

axis_RUNTIME_API void *axis_env_get_attached_target(axis_env_t *self);

axis_RUNTIME_API void axis_env_destroy(axis_env_t *self);
