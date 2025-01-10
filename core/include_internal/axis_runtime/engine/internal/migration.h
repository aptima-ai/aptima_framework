//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/lib/smart_ptr.h"

typedef struct axis_engine_t axis_engine_t;
typedef struct axis_connection_t axis_connection_t;

typedef struct axis_engine_migration_user_data_t {
  axis_connection_t *connection;
  axis_shared_ptr_t *cmd;
} axis_engine_migration_user_data_t;

axis_RUNTIME_PRIVATE_API void axis_engine_on_connection_cleaned(
    axis_engine_t *self, axis_connection_t *connection, axis_shared_ptr_t *cmd);

axis_RUNTIME_PRIVATE_API void axis_engine_on_connection_cleaned_async(
    axis_engine_t *self, axis_connection_t *connection, axis_shared_ptr_t *cmd);

axis_RUNTIME_PRIVATE_API void axis_engine_on_connection_closed(
    axis_connection_t *connection, void *user_data);
