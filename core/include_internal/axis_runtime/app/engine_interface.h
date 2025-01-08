//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/lib/smart_ptr.h"

typedef struct axis_engine_t axis_engine_t;
typedef struct axis_app_t axis_app_t;
typedef struct axis_predefined_graph_info_t axis_predefined_graph_info_t;

axis_RUNTIME_PRIVATE_API axis_engine_t *axis_app_create_engine(
    axis_app_t *self, axis_shared_ptr_t *cmd);

axis_RUNTIME_PRIVATE_API void axis_app_del_engine(axis_app_t *self,
                                                axis_engine_t *engine);

axis_RUNTIME_PRIVATE_API axis_predefined_graph_info_t *
axis_app_get_singleton_predefined_graph_info_based_on_dest_graph_id_from_msg(
    axis_app_t *self, axis_shared_ptr_t *msg);

axis_RUNTIME_PRIVATE_API axis_engine_t *
axis_app_get_engine_based_on_dest_graph_id_from_msg(axis_app_t *self,
                                                   axis_shared_ptr_t *msg);
