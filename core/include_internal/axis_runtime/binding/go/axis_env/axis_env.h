//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/binding/go/axis_env/axis_env_internal.h"

axis_RUNTIME_PRIVATE_API bool axis_go_axis_env_check_integrity(
    axis_go_axis_env_t *self);

axis_RUNTIME_PRIVATE_API axis_go_axis_env_t *axis_go_axis_env_reinterpret(
    uintptr_t bridge_addr);

axis_RUNTIME_PRIVATE_API axis_go_axis_env_t *axis_go_axis_env_wrap(
    axis_env_t *c_axis_env);

axis_RUNTIME_PRIVATE_API axis_go_handle_t
axis_go_axis_env_go_handle(axis_go_axis_env_t *self);

extern void tenGoCAsyncApiCallback(axis_go_handle_t callback,
                                   axis_go_error_t cgo_error);
