//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "include_internal/ten_runtime/protocol/integrated/retry.h"
#include "include_internal/ten_runtime/protocol/protocol.h"
#include "ten_utils/io/stream.h"
#include "ten_utils/io/transport.h"

typedef struct ten_protocol_integrated_t ten_protocol_integrated_t;
typedef struct ten_timer_t ten_timer_t;

typedef void (*ten_protocol_integrated_on_input_func_t)(
    ten_protocol_integrated_t *protocol, ten_buf_t buf, ten_list_t *input);

typedef ten_buf_t (*ten_protocol_integrated_on_output_func_t)(
    ten_protocol_integrated_t *protocol, ten_list_t *output);

typedef struct ten_protocol_integrated_connect_to_context_t {
  // The protocol which is trying to connect to the server.
  ten_protocol_integrated_t *protocol;

  // The server URI to connect to.
  ten_string_t server_uri;

  // The callback function to be called when the connection is established or
  // failed.
  //
  // @note Set to NULL if the callback has been called.
  ten_protocol_on_server_connected_func_t on_server_connected;

  void *user_data;
} ten_protocol_integrated_connect_to_context_t;

/**
 * @brief This is the base class of all the protocols which uses the event loop
 * inside the APTIMA world.
 */
struct ten_protocol_integrated_t {
  // All protocols should be inherited from the ten_protocol_t base structure.
  ten_protocol_t base;

  // The following fields are specific to this (integrated) protocol structure.

  union {
    // LISTENING-role protocol uses this field.
    ten_transport_t *listening_transport;

    // COMMUNICATION-role protocol uses this field.
    ten_stream_t *communication_stream;
  } role_facility;

  // Used to convert a buffer to APTIMA runtime messages.
  ten_protocol_integrated_on_input_func_t on_input;

  // Used to convert APTIMA runtime messages to a buffer.
  ten_protocol_integrated_on_output_func_t on_output;

  // Used to configure the retry mechanism.
  ten_protocol_integrated_retry_config_t retry_config;
  ten_timer_t *retry_timer;
};

TEN_RUNTIME_API void ten_protocol_integrated_init(
    ten_protocol_integrated_t *self, const char *name,
    ten_protocol_integrated_on_input_func_t on_input,
    ten_protocol_integrated_on_output_func_t on_output);

TEN_RUNTIME_PRIVATE_API ten_protocol_integrated_connect_to_context_t *
ten_protocol_integrated_connect_to_context_create(
    ten_protocol_integrated_t *self, const char *server_uri,
    ten_protocol_on_server_connected_func_t on_server_connected,
    void *user_data);

TEN_RUNTIME_PRIVATE_API void ten_protocol_integrated_connect_to_context_destroy(
    ten_protocol_integrated_connect_to_context_t *context);
