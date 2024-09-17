//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include <time.h>

#include "ten_utils/lib/string.h"

TEN_UTILS_PRIVATE_API void ten_log_get_time(struct tm *time_info, size_t *msec);

TEN_UTILS_PRIVATE_API void ten_log_add_time_string(ten_string_t *buf,
                                                   struct tm *time_info,
                                                   size_t msec);
