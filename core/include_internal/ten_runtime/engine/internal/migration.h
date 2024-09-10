//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_utils/lib/smart_ptr.h"

typedef struct ten_engine_t ten_engine_t;
typedef struct ten_connection_t ten_connection_t;

typedef struct ten_engine_migration_user_data_t {
  ten_connection_t *connection;
  ten_shared_ptr_t *cmd;
} ten_engine_migration_user_data_t;

TEN_RUNTIME_PRIVATE_API void ten_engine_on_connection_cleaned(
    ten_engine_t *self, ten_connection_t *connection, ten_shared_ptr_t *cmd);

TEN_RUNTIME_PRIVATE_API void ten_engine_on_connection_cleaned_async(
    ten_engine_t *self, ten_connection_t *connection, ten_shared_ptr_t *cmd);

TEN_RUNTIME_PRIVATE_API void ten_engine_on_connection_closed(
    ten_connection_t *connection, void *user_data);
