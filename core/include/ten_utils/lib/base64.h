//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include "ten_utils/lib/buf.h"
#include "ten_utils/lib/string.h"

TEN_UTILS_API bool ten_base64_to_string(ten_string_t *result, ten_buf_t *buf);

TEN_UTILS_API bool ten_base64_from_string(ten_string_t *str, ten_buf_t *result);
