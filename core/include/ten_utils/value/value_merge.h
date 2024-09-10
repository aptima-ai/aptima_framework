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

TEN_UTILS_API bool ten_value_object_merge_with_move(ten_value_t *dest,
                                                    ten_value_t *src);

TEN_UTILS_API bool ten_value_object_merge_with_clone(ten_value_t *dest,
                                                     ten_value_t *src);

TEN_UTILS_API bool ten_value_object_merge_with_json(ten_value_t *dest,
                                                    ten_json_t *src);
