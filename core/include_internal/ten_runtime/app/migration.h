//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

typedef struct ten_app_t ten_app_t;
typedef struct ten_connection_t ten_connection_t;
typedef struct ten_engine_t ten_engine_t;

TEN_RUNTIME_PRIVATE_API void ten_app_clean_connection(
    ten_app_t *self, ten_connection_t *connection);

TEN_RUNTIME_PRIVATE_API void ten_app_clean_connection_async(
    ten_app_t *self, ten_connection_t *connection);
