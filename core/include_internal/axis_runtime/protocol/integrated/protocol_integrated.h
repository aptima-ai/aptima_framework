//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/protocol/integrated/retry.h"
#include "include_internal/axis_runtime/protocol/protocol.h"
#include "axis_utils/io/stream.h"
#include "axis_utils/io/transport.h"

typedef struct axis_protocol_integrated_t axis_protocol_integrated_t;
typedef struct axis_timer_t axis_timer_t;

typedef void (*axis_protocol_integrated_on_input_func_t)(
    axis_protocol_integrated_t *protocol, axis_buf_t buf, axis_list_t *input);

typedef axis_buf_t (*axis_protocol_integrated_on_output_func_t)(
    axis_protocol_integrated_t *protocol, axis_list_t *output);

typedef struct axis_protocol_integrated_connect_to_context_t {
  // The protocol which is trying to connect to the server.
  axis_protocol_integrated_t *protocol;

  // The server URI to connect to.
  axis_string_t server_uri;

  // The callback function to be called when the connection is established or
  // failed.
  //
  // @note Set to NULL if the callback has been called.
  axis_protocol_on_server_connected_func_t on_server_connected;

  void *user_data;
} axis_protocol_integrated_connect_to_context_t;

/**
 * @brief This is the base class of all the protocols which uses the event loop
 * inside the TEN world.
 */
struct axis_protocol_integrated_t {
  // All protocols should be inherited from the axis_protocol_t base structure.
  axis_protocol_t base;

  // The following fields are specific to this (integrated) protocol structure.

  union {
    // LISTENING-role protocol uses this field.
    axis_transport_t *listening_transport;

    // COMMUNICATION-role protocol uses this field.
    axis_stream_t *communication_stream;
  } role_facility;

  // Used to convert a buffer to TEN runtime messages.
  axis_protocol_integrated_on_input_func_t on_input;

  // Used to convert TEN runtime messages to a buffer.
  axis_protocol_integrated_on_output_func_t on_output;

  // Used to configure the retry mechanism.
  axis_protocol_integrated_retry_config_t retry_config;
  axis_timer_t *retry_timer;
};

axis_RUNTIME_API void axis_protocol_integrated_init(
    axis_protocol_integrated_t *self, const char *name,
    axis_protocol_integrated_on_input_func_t on_input,
    axis_protocol_integrated_on_output_func_t on_output);

axis_RUNTIME_PRIVATE_API axis_protocol_integrated_connect_to_context_t *
axis_protocol_integrated_connect_to_context_create(
    axis_protocol_integrated_t *self, const char *server_uri,
    axis_protocol_on_server_connected_func_t on_server_connected,
    void *user_data);

axis_RUNTIME_PRIVATE_API void axis_protocol_integrated_connect_to_context_destroy(
    axis_protocol_integrated_connect_to_context_t *context);
