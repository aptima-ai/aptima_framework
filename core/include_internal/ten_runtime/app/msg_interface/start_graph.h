//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_utils/lib/error.h"
#include "ten_utils/lib/smart_ptr.h"

typedef struct ten_app_t ten_app_t;
typedef struct ten_connection_t ten_connection_t;

TEN_RUNTIME_PRIVATE_API bool ten_app_on_cmd_start_graph(
    ten_app_t *self, ten_connection_t *connection, ten_shared_ptr_t *cmd,
    ten_error_t *err);
