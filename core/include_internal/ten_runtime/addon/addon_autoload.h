//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdbool.h>

#include "ten_utils/lib/error.h"

TEN_RUNTIME_PRIVATE_API void ten_addon_load_from_path(const char *path);

TEN_RUNTIME_PRIVATE_API bool ten_addon_load_all(ten_error_t *err);
