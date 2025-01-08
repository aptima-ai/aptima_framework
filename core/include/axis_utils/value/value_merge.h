//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>

#include "axis_utils/lib/json.h"

typedef struct axis_value_t axis_value_t;

axis_UTILS_API bool axis_value_object_merge_with_move(axis_value_t *dest,
                                                    axis_value_t *src);

axis_UTILS_API bool axis_value_object_merge_with_clone(axis_value_t *dest,
                                                     axis_value_t *src);

axis_UTILS_API bool axis_value_object_merge_with_json(axis_value_t *dest,
                                                    axis_json_t *src);
