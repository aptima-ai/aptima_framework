//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_runtime/binding/go/interface/ten/common.h"
#include "ten_utils/lib/json.h"

TEN_RUNTIME_PRIVATE_API ten_json_t *ten_go_json_loads(const void *json_bytes,
                                                    int json_bytes_len,
                                                    ten_go_status_t *status);
