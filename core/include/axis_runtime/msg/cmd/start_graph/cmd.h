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

typedef struct axis_msg_t axis_msg_t;
typedef struct axis_cmd_start_graph_t axis_cmd_start_graph_t;

axis_RUNTIME_API axis_shared_ptr_t *axis_cmd_start_graph_create(void);

axis_RUNTIME_API bool axis_cmd_start_graph_set_predefined_graph_name(
    axis_shared_ptr_t *self, const char *predefined_graph_name,
    axis_error_t *err);

axis_RUNTIME_API bool axis_cmd_start_graph_set_long_running_mode(
    axis_shared_ptr_t *self, bool long_running_mode, axis_error_t *err);

axis_RUNTIME_API bool axis_cmd_start_graph_set_graph_from_json_str(
    axis_shared_ptr_t *self, const char *json_str, axis_error_t *err);
