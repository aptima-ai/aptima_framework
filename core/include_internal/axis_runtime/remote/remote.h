//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/common/loc.h"
#include "axis_utils/container/hash_handle.h"
#include "axis_utils/container/hash_table.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/smart_ptr.h"

#define axis_REMOTE_SIGNATURE 0xB4540BD80996AA45U

typedef struct axis_connection_t axis_connection_t;
typedef struct axis_remote_t axis_remote_t;
typedef struct axis_engine_t axis_engine_t;

typedef void (*axis_remote_on_closed_func_t)(axis_remote_t *self,
                                            void *on_closed_data);

typedef bool (*axis_remote_on_msg_func_t)(axis_remote_t *self,
                                         axis_shared_ptr_t *msg,
                                         void *on_msg_data);

typedef void (*axis_remote_on_server_connected_func_t)(axis_remote_t *self,
                                                      axis_shared_ptr_t *cmd);

typedef void (*axis_remote_on_error_func_t)(axis_remote_t *self,
                                           axis_shared_ptr_t *cmd);

struct axis_remote_t {
  axis_signature_t signature;
  axis_sanitizer_thread_check_t thread_check;

  axis_hashhandle_t hh_in_remote_table;

  bool is_closing;
  bool is_closed;

  axis_string_t uri;

  // The role of 'remote' is a bridge between a 'connection' and a 'engine'.
  //
  //     ... <==> engine <==> remote <==> connection <==> ...
  //                           ^^^^
  axis_connection_t *connection;
  axis_engine_t *engine;

  // This field is used in the scenario of 'connect_to', all the messages coming
  // from this remote will go to the extension where 'connect_to' command is
  // executed.
  axis_loc_t explicit_dest_loc;

  // @{
  // Member functions.
  axis_remote_on_closed_func_t on_closed;
  void *on_closed_data;

  axis_remote_on_msg_func_t on_msg;
  void *on_msg_data;

  axis_remote_on_server_connected_func_t on_server_connected;
  axis_shared_ptr_t *on_server_connected_cmd;

  axis_remote_on_error_func_t on_error;
  // @}
};

axis_RUNTIME_PRIVATE_API bool axis_remote_check_integrity(axis_remote_t *self,
                                                        bool check_thread);

axis_RUNTIME_PRIVATE_API void axis_remote_destroy(axis_remote_t *self);

axis_RUNTIME_PRIVATE_API bool axis_remote_on_input(axis_remote_t *self,
                                                 axis_shared_ptr_t *msg,
                                                 axis_error_t *err);

axis_RUNTIME_PRIVATE_API void axis_remote_send_msg(axis_remote_t *self,
                                                 axis_shared_ptr_t *msg);

axis_RUNTIME_PRIVATE_API bool axis_remote_is_uri_equal_to(axis_remote_t *self,
                                                        const char *uri);

axis_RUNTIME_PRIVATE_API axis_runloop_t *axis_remote_get_attached_runloop(
    axis_remote_t *self);

axis_RUNTIME_PRIVATE_API void axis_remote_on_connection_closed(
    axis_connection_t *connection, void *on_closed_data);

axis_RUNTIME_PRIVATE_API axis_remote_t *axis_remote_create_for_engine(
    const char *uri, axis_engine_t *engine, axis_connection_t *connection);

axis_RUNTIME_PRIVATE_API void axis_remote_close(axis_remote_t *self);

axis_RUNTIME_PRIVATE_API void axis_remote_connect_to(
    axis_remote_t *self, axis_remote_on_server_connected_func_t connected,
    axis_shared_ptr_t *on_server_connected_cmd,
    axis_remote_on_error_func_t on_error);
