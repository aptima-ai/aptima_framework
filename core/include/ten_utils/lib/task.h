//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include <stdint.h>

typedef int64_t ten_pid_t;

/**
 * @brief Get process id.
 */
TEN_UTILS_API ten_pid_t ten_task_get_id();
