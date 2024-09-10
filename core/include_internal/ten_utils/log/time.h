//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include <time.h>

typedef void (*ten_log_get_time_func_t)(struct tm *tm, size_t *usec);

TEN_UTILS_PRIVATE_API ten_log_get_time_func_t g_ten_log_get_time;

TEN_UTILS_PRIVATE_API void ten_log_get_time(struct tm *tm, size_t *msec);
