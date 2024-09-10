//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_utils/lib/smart_ptr.h"

typedef struct ten_cmd_close_app_t ten_cmd_close_app_t;

TEN_RUNTIME_API ten_shared_ptr_t *ten_cmd_close_app_create(void);
