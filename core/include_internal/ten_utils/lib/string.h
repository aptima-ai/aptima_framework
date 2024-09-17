//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include "ten_utils/lib/string.h"

TEN_UTILS_PRIVATE_API void ten_string_init_from_va_list(ten_string_t *self,
                                                        const char *fmt,
                                                        va_list ap);
