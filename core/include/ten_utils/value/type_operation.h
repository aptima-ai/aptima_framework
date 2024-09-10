//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include "ten_utils/container/list.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/value/type.h"

TEN_UTILS_API TEN_TYPE ten_type_from_string(const char *type_str);

TEN_UTILS_API const char *ten_type_to_string(TEN_TYPE type);

TEN_UTILS_PRIVATE_API ten_list_t ten_type_from_json(ten_json_t *json);

TEN_UTILS_PRIVATE_API bool ten_type_is_compatible(TEN_TYPE actual,
                                                  TEN_TYPE expected);
