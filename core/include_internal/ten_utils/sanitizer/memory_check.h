//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include <stddef.h>

TEN_UTILS_API char *ten_sanitizer_memory_strndup(const char *str, size_t size,
                                                 const char *file_name,
                                                 uint32_t lineno,
                                                 const char *func_name);
