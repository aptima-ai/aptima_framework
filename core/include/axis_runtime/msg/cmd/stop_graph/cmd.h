//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include "aptima_utils/lib/smart_ptr.h"

typedef struct aptima_msg_t aptima_msg_t;

aptima_RUNTIME_API aptima_shared_ptr_t *aptima_cmd_stop_graph_create(void);

aptima_RUNTIME_API const char *aptima_cmd_stop_graph_get_graph_id(
    aptima_shared_ptr_t *self);

aptima_RUNTIME_API bool aptima_cmd_stop_graph_set_graph_id(aptima_shared_ptr_t *self,
                                                     const char *graph_id);
