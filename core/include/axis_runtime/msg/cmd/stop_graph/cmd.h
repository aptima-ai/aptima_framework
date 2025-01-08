//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/lib/smart_ptr.h"

typedef struct axis_msg_t axis_msg_t;

axis_RUNTIME_API axis_shared_ptr_t *axis_cmd_stop_graph_create(void);

axis_RUNTIME_API const char *axis_cmd_stop_graph_get_graph_id(
    axis_shared_ptr_t *self);

axis_RUNTIME_API bool axis_cmd_stop_graph_set_graph_id(axis_shared_ptr_t *self,
                                                     const char *graph_id);
