//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdbool.h>

#include "aptima_utils/lib/json.h"

typedef struct aptima_value_t aptima_value_t;

aptima_UTILS_API aptima_value_t *aptima_value_from_json(aptima_json_t *json);

aptima_UTILS_API aptima_json_t *aptima_value_to_json(aptima_value_t *self);

aptima_UTILS_API bool aptima_value_set_from_json(aptima_value_t *self, aptima_json_t *json);
