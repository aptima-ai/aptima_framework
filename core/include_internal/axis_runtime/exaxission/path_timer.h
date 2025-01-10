//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdint.h>

#define axis_DEFAULT_PATH_CHECK_INTERVAL \
  ((uint64_t)(10 * 1000 * 1000))  // 10s by default.
#define axis_DEFAULT_PATH_TIMEOUT \
  ((uint64_t)(3 * 60 * 1000 * 1000))  // 3min by default.

typedef struct axis_extension_t axis_extension_t;
typedef struct axis_timer_t axis_timer_t;
typedef struct axis_path_t axis_path_t;

typedef struct axis_path_timeout_info {
  uint64_t in_path_timeout;   // microseconds.
  uint64_t out_path_timeout;  // microseconds.
  uint64_t check_interval;    // microseconds.
} axis_path_timeout_info;

axis_RUNTIME_PRIVATE_API axis_timer_t *axis_extension_create_timer_for_in_path(
    axis_extension_t *self);

axis_RUNTIME_PRIVATE_API axis_timer_t *axis_extension_create_timer_for_out_path(
    axis_extension_t *self);
