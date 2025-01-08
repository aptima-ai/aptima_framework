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
typedef struct axis_remote_t axis_remote_t;
typedef struct axis_connection_t axis_connection_t;

typedef void (*axis_engine_on_remote_created_cb_t)(axis_engine_t *engine,
                                                  axis_remote_t *remote,
                                                  void *user_data);
typedef void (*axis_engine_on_connected_to_graph_remote_cb_t)(
    axis_engine_t *engine, bool success, void *user_data);

typedef struct axis_engine_on_protocol_created_ctx_t {
  axis_engine_on_remote_created_cb_t cb;
  void *user_data;
} axis_engine_on_protocol_created_ctx_t;

axis_RUNTIME_PRIVATE_API void axis_engine_upgrade_weak_remote_to_normal_remote(
    axis_engine_t *self, axis_remote_t *remote);

axis_RUNTIME_PRIVATE_API bool axis_engine_connect_to_graph_remote(
    axis_engine_t *self, const char *uri, axis_shared_ptr_t *cmd);

axis_RUNTIME_PRIVATE_API void axis_engine_route_msg_to_remote(
    axis_engine_t *self, axis_shared_ptr_t *msg);

axis_RUNTIME_PRIVATE_API axis_remote_t *axis_engine_check_remote_is_existed(
    axis_engine_t *self, const char *uri);

axis_RUNTIME_PRIVATE_API bool axis_engine_check_remote_is_duplicated(
    axis_engine_t *self, const char *uri);

axis_RUNTIME_PRIVATE_API bool axis_engine_check_remote_is_weak(
    axis_engine_t *self, axis_remote_t *remote);

axis_RUNTIME_PRIVATE_API void axis_engine_on_remote_closed(axis_remote_t *remote,
                                                         void *on_closed_data);

axis_RUNTIME_PRIVATE_API bool axis_engine_receive_msg_from_remote(
    axis_remote_t *remote, axis_shared_ptr_t *msg, void *user_data);

axis_RUNTIME_PRIVATE_API void axis_engine_link_connection_to_remote(
    axis_engine_t *self, axis_connection_t *connection, const char *uri);
