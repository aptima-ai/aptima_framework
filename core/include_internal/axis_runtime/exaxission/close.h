//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

typedef struct axis_extension_t axis_extension_t;
typedef struct axis_timer_t axis_timer_t;

axis_RUNTIME_PRIVATE_API void axis_extension_do_pre_close_action(
    axis_extension_t *self);

axis_RUNTIME_PRIVATE_API void axis_extension_on_timer_closed(
    axis_timer_t *timer, void *on_closed_data);

axis_RUNTIME_PRIVATE_API void axis_extension_on_path_timer_closed(
    axis_timer_t *timer, void *on_closed_data);
