//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "include_internal/ten_runtime/protocol/protocol.h"
#include "ten_utils/io/stream.h"
#include "ten_utils/io/transport.h"

typedef struct ten_protocol_integrated_t ten_protocol_integrated_t;

typedef void (*ten_protocol_integrated_on_input_func_t)(
    ten_protocol_integrated_t *protocol, ten_buf_t buf, ten_list_t *input);

typedef ten_buf_t (*ten_protocol_integrated_on_output_func_t)(
    ten_protocol_integrated_t *protocol, ten_list_t *output);

/**
 * @brief This is the base class of all the protocols which uses the event loop
 * inside the TEN world.
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

  // Used to convert a buffer to TEN runtime messages.
  ten_protocol_integrated_on_input_func_t on_input;

  // Used to convert TEN runtime messages to a buffer.
  ten_protocol_integrated_on_output_func_t on_output;
};

TEN_RUNTIME_API void ten_protocol_integrated_init(
    ten_protocol_integrated_t *self, const char *name,
    ten_protocol_integrated_on_input_func_t on_input,
    ten_protocol_integrated_on_output_func_t on_output);
