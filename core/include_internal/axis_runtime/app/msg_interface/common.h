//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/lib/error.h"
#include "axis_utils/lib/smart_ptr.h"

typedef struct axis_app_t axis_app_t;
typedef struct axis_connection_t axis_connection_t;
typedef struct axis_engine_t axis_engine_t;

axis_RUNTIME_PRIVATE_API void axis_app_push_to_in_msgs_queue(
    axis_app_t *self, axis_shared_ptr_t *msg);

axis_RUNTIME_PRIVATE_API bool axis_app_dispatch_msg(axis_app_t *self,
                                                  axis_shared_ptr_t *msg,
                                                  axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_app_handle_in_msg(axis_app_t *self,
                                                   axis_connection_t *connection,
                                                   axis_shared_ptr_t *msg,
                                                   axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_connection_t *axis_app_find_src_connection_for_msg(
    axis_app_t *self, axis_shared_ptr_t *msg);

axis_RUNTIME_PRIVATE_API void
axis_app_do_connection_migration_or_push_to_engine_queue(
    axis_connection_t *connection, axis_engine_t *engine, axis_shared_ptr_t *msg);
