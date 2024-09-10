//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include <stdbool.h>

#include "ten_utils/lib/string.h"
#include "ten_utils/value/value_kv.h"

TEN_UTILS_API bool ten_value_to_string(ten_value_t *self, ten_string_t *str,
                                       ten_error_t *err);

TEN_UTILS_API ten_value_t *ten_value_from_type_and_string(TEN_TYPE type,
                                                          const char *str,
                                                          ten_error_t *err);
