//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include <stddef.h>

TEN_UTILS_API size_t ten_terminal_get_width_in_char(void);

TEN_UTILS_API int ten_terminal_is_terminal(int fd);
