//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include "aptima_utils/lib/error.h"
#include "aptima_utils/lib/smart_ptr.h"

typedef struct aptima_msg_t aptima_msg_t;
typedef struct aptima_cmd_start_graph_t aptima_cmd_start_graph_t;

aptima_RUNTIME_API aptima_shared_ptr_t *aptima_cmd_start_graph_create(void);

aptima_RUNTIME_API bool aptima_cmd_start_graph_set_predefined_graph_name(
    aptima_shared_ptr_t *self, const char *predefined_graph_name,
    aptima_error_t *err);

aptima_RUNTIME_API bool aptima_cmd_start_graph_set_long_running_mode(
    aptima_shared_ptr_t *self, bool long_running_mode, aptima_error_t *err);

aptima_RUNTIME_API bool aptima_cmd_start_graph_set_graph_from_json_str(
    aptima_shared_ptr_t *self, const char *json_str, aptima_error_t *err);
