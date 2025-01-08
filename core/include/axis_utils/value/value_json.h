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

axis_UTILS_API axis_value_t *axis_value_from_json(axis_json_t *json);

axis_UTILS_API axis_json_t *axis_value_to_json(axis_value_t *self);

axis_UTILS_API bool axis_value_set_from_json(axis_value_t *self, axis_json_t *json);
