//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stddef.h>

#include "axis_utils/container/hash_handle.h"
#include "axis_utils/lib/string.h"

typedef struct axis_extension_t axis_extension_t;

typedef struct axis_extension_output_msg_not_connected_count_t {
  axis_hashhandle_t hh_in_map;

  axis_string_t msg_name;
  size_t count;
} axis_extension_output_msg_not_connected_count_t;

axis_RUNTIME_PRIVATE_API bool axis_extension_increment_msg_not_connected_count(
    axis_extension_t *extension, const char *msg_name);
