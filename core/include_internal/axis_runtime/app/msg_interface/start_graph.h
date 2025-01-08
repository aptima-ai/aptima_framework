//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/lib/error.h"
#include "axis_utils/lib/smart_ptr.h"

typedef struct axis_app_t axis_app_t;
typedef struct axis_connection_t axis_connection_t;

axis_RUNTIME_PRIVATE_API bool axis_app_handle_start_graph_cmd(
    axis_app_t *self, axis_connection_t *connection, axis_shared_ptr_t *cmd,
    axis_error_t *err);
