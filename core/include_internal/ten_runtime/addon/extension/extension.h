//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

typedef struct ten_addon_store_t ten_addon_store_t;

TEN_RUNTIME_PRIVATE_API ten_addon_store_t *ten_extension_get_store(void);
