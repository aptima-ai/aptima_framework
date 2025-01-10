//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/protocol/close.h"

#include "include_internal/ten_runtime/protocol/integrated/close.h"
#include "include_internal/ten_runtime/protocol/integrated/protocol_integrated.h"
#include "include_internal/ten_runtime/protocol/protocol.h"
#include "include_internal/ten_runtime/timer/timer.h"
#include "ten_utils/io/stream.h"
#include "ten_utils/log/log.h"
#include "ten_utils/macro/check.h"

// The principle is very simple. As long as the integrated protocol still has
// resources in hands, it cannot be closed, otherwise it can.
static bool ten_protocol_integrated_could_be_close(
    ten_protocol_integrated_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  ten_protocol_t *protocol = &self->base;
  TEN_ASSERT(protocol && ten_protocol_check_integrity(protocol, true),
             "Should not happen.");
  TEN_ASSERT(protocol->role != TEN_PROTOCOL_ROLE_INVALID, "Should not happen.");

  // Check resources according to different roles.
  switch (protocol->role) {
    case TEN_PROTOCOL_ROLE_LISTEN:
      if (self->role_facility.listening_transport) {
        return false;
      }
      break;
    case TEN_PROTOCOL_ROLE_IN_INTERNAL:
    case TEN_PROTOCOL_ROLE_IN_EXTERNAL:
    case TEN_PROTOCOL_ROLE_OUT_INTERNAL:
    case TEN_PROTOCOL_ROLE_OUT_EXTERNAL:
      if (self->role_facility.communication_stream) {
        return false;
      }

      if (self->retry_timer) {
        return false;
      }
      break;
    default:
      TEN_ASSERT(0, "Should not happen.");
      break;
  }

  return true;
}

void ten_protocol_integrated_on_close(ten_protocol_integrated_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  ten_protocol_t *protocol = &self->base;
  TEN_ASSERT(protocol && ten_protocol_check_integrity(protocol, true),
             "Should not happen.");
  TEN_ASSERT(protocol->role != TEN_PROTOCOL_ROLE_INVALID, "Should not happen.");
  TEN_ASSERT(
      ten_protocol_is_closing(protocol),
      "As a principle, the protocol could only be closed from the aptima world.");

  if (!ten_protocol_integrated_could_be_close(self)) {
    TEN_LOGD("Could not close alive integrated protocol.");
    return;
  }
  TEN_LOGD("Close integrated protocol.");

  ten_protocol_on_close(&self->base);
}

void ten_protocol_integrated_on_stream_closed(ten_protocol_integrated_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  ten_protocol_t *protocol = &self->base;
  TEN_ASSERT(protocol && ten_protocol_check_integrity(protocol, true),
             "Invalid argument.");
  TEN_ASSERT(ten_protocol_role_is_communication(protocol),
             "Should not happen.");

  // Remember that this resource is closed.
  self->role_facility.communication_stream = NULL;

  if (ten_protocol_is_closing(protocol)) {
    ten_protocol_integrated_on_close(self);
  }
}

void ten_protocol_integrated_on_transport_closed(
    ten_protocol_integrated_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  ten_protocol_t *protocol = &self->base;
  TEN_ASSERT(protocol && ten_protocol_check_integrity(protocol, true),
             "Invalid argument.");
  TEN_ASSERT(protocol->role == TEN_PROTOCOL_ROLE_LISTEN, "Should not happen.");

  // Remember that this resource is closed.
  self->role_facility.listening_transport = NULL;

  if (ten_protocol_is_closing(protocol)) {
    ten_protocol_integrated_on_close(self);
  }
}

// Closing flow is as follows.
//
// - Stage 1 : 'Need to close' notification stage
//
//   APTIMA world -> (notify) -> base protocol -> (notify) -> integrated protocol
//
// - Stage 2 : 'I cam closed' notification stage
//
//                                 <- (I am closed) <- integrated protocol
//        <- (I am closed) <- base protocol
//   APTIMA world
//
// The following function is for the stage 1, and the above
// 'ten_protocol_integrated_on_xxx_closed' functions are for the stage 2.
void ten_protocol_integrated_close(ten_protocol_integrated_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  ten_protocol_t *protocol = &self->base;
  TEN_ASSERT(protocol && ten_protocol_check_integrity(protocol, true),
             "Should not happen.");

  bool perform_any_closing_operation = false;

  switch (protocol->role) {
    case TEN_PROTOCOL_ROLE_LISTEN:
      if (self->role_facility.listening_transport) {
        ten_transport_close(self->role_facility.listening_transport);
        perform_any_closing_operation = true;
      }
      break;

    case TEN_PROTOCOL_ROLE_IN_INTERNAL:
    case TEN_PROTOCOL_ROLE_IN_EXTERNAL:
    case TEN_PROTOCOL_ROLE_OUT_INTERNAL:
    case TEN_PROTOCOL_ROLE_OUT_EXTERNAL:
      if (self->role_facility.communication_stream) {
        ten_stream_close(self->role_facility.communication_stream);
        perform_any_closing_operation = true;
      }

      if (self->retry_timer) {
        ten_timer_stop_async(self->retry_timer);
        ten_timer_close_async(self->retry_timer);
        perform_any_closing_operation = true;
      }
      break;

    default:
      TEN_ASSERT(0, "Should not happen.");
      break;
  }

  if (!perform_any_closing_operation) {
    // If there is no any further closing operations submitted, it means the
    // integrated protocol could proceed its closing flow.
    if (ten_protocol_is_closing(protocol)) {
      ten_protocol_integrated_on_close(self);
    }
  }
}
