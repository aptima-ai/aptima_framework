//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include "ten_utils/sanitizer/thread_check.h"

TEN_UTILS_API void ten_sanitizer_thread_check_init(
    ten_sanitizer_thread_check_t *self);
