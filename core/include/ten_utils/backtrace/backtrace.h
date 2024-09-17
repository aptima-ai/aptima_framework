//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
// This file is modified from
// https://github.com/ianlancetaylor/libbacktrace [BSD license]
//
#pragma once

#include "ten_utils/ten_config.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

TEN_UTILS_API void ten_backtrace_dump_global(size_t skip);

#ifdef __cplusplus
} /* End extern "C".  */
#endif
