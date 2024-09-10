//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include <stdbool.h>

#include "ten_utils/lib/json.h"

typedef struct ten_value_t ten_value_t;

TEN_UTILS_API ten_value_t *ten_value_from_json(ten_json_t *json);

TEN_UTILS_API ten_json_t *ten_value_to_json(ten_value_t *self);
