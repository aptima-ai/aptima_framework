//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_utils/lib/error.h"
#include "ten_utils/lib/smart_ptr.h"

TEN_RUNTIME_API ten_shared_ptr_t *ten_cmd_create_from_json_string(
    const char *json_str, ten_error_t *err);

TEN_RUNTIME_API ten_shared_ptr_t *ten_cmd_create(const char *cmd_name,
                                                 ten_error_t *err);
