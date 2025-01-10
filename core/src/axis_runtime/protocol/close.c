//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/protocol/close.h"

#include "include_internal/axis_runtime/protocol/protocol.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"

// The relationship of the protocol chain is as follows.
//
//  base protocol (axis_protocol_t) -->
//                implementation protocol (inherited from axis_protocol_t)
//
// The condition which a base protocol can be closed is when the implementation
// protocol is closed.
//
// Closing flow is as follows.
//
// - Stage 1 : 'Need to close' notification stage
//
//   APTIMA world -> (notify) ->
//                         base protocol -> (notify) ->
//                                                   implementation protocol
//
// - Stage 2 : 'I cam closed' notification stage
//
//                               <- (I am closed) <- implementation protocol
//        <- (I am closed) <- base protocol
//   APTIMA world
//
//   The time that a protocol (no matter it is a base protocol or an
//   implementation protocol) can be closed is that when all resources held in
//   that protocol are closed, then that protocol can be closed.
static bool axis_protocol_could_be_close(axis_protocol_t *self) {
  axis_ASSERT(self && axis_protocol_check_integrity(self, true),
             "Should not happen.");

  // As the only underlying resource of the base protocol is the implementation
  // protocol now. When closing a base protocol, the only thing it cares about
  // is the implementation protocol is closed or not, and this function will be
  // called only if the implementation protocol has been closed, so it always
  // returns true.
  //
  // If the base protocol has more underlying resources in the future, this
  // function will be called when each resource is closed. And there should be
  // one field in the base protocol to be paired with each underlying resource,
  // and check every field in this function to determine that whether all the
  // resources are closed. The brief calling flow is as follows.
  //
  // x_do_close() {
  //   axis_protocol_on_close();
  // }
  //
  // y_do_close() {
  //   axis_protocol_on_close();
  // }
  //
  // axis_protocol_on_close() {
  //   if (axis_protocol_could_be_close()) {
  //     axis_protocol_do_close();
  //   }
  // }
  //
  // axis_protocol_could_be_close() {
  //   return x_is_closed && y_is_closed;
  // }
  //
  return true;
}

void axis_protocol_on_close(axis_protocol_t *self) {
  axis_ASSERT(self && axis_protocol_check_integrity(self, true),
             "Should not happen.");

  if (!axis_protocol_could_be_close(self)) {
    axis_LOGD("Could not close alive base protocol.");
    return;
  }
  axis_LOGD("Close base protocol.");

  self->is_closed = true;

  if (self->on_closed) {
    // Call the registered on_closed callback when the base protocol is closed.
    self->on_closed(self, self->on_closed_data);
  }
}

bool axis_protocol_is_closing(axis_protocol_t *self) {
  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is designed to be used in any
                 // threads, that's why 'is_closing' is of atomic type.
                 axis_protocol_check_integrity(self, false),
             "Should not happen.");
  return axis_atomic_load(&self->is_closing) == 1;
}

bool axis_protocol_is_closed(axis_protocol_t *self) {
  axis_ASSERT(self && axis_protocol_check_integrity(self, true),
             "Access across threads.");

  return self->is_closed;
}

void axis_protocol_set_on_closed(axis_protocol_t *self,
                                axis_protocol_on_closed_func_t on_closed,
                                void *on_closed_data) {
  axis_ASSERT(self && axis_protocol_check_integrity(self, true),
             "Should not happen.");

  self->on_closed = on_closed;
  self->on_closed_data = on_closed_data;
}

void axis_protocol_close(axis_protocol_t *self) {
  axis_ASSERT(self, "Should not happen.");
  axis_ASSERT(axis_protocol_check_integrity(self, true), "Should not happen.");

  if (axis_atomic_bool_compare_swap(&self->is_closing, 0, 1)) {
    switch (self->role) {
      case axis_PROTOCOL_ROLE_LISTEN:
        axis_LOGD("Try to close listening protocol: %s",
                 axis_string_get_raw_str(&self->uri));
        break;
      case axis_PROTOCOL_ROLE_IN_INTERNAL:
      case axis_PROTOCOL_ROLE_IN_EXTERNAL:
      case axis_PROTOCOL_ROLE_OUT_INTERNAL:
      case axis_PROTOCOL_ROLE_OUT_EXTERNAL:
        axis_LOGD("Try to close communication protocol: %s",
                 axis_string_get_raw_str(&self->uri));
        break;
      default:
        axis_ASSERT(0, "Should not happen.");
        break;
    }

    self->close(self);
  }
}
