//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_runtime/protocol/close.h"

#include "include_internal/ten_runtime/common/closeable.h"
#include "include_internal/ten_runtime/protocol/protocol.h"
#include "ten_utils/log/log.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/field.h"
#include "ten_utils/macro/mark.h"

// The relationship of the protocol chain is as follows.
//
//  base protocol (ten_protocol_t) -->
//                implementation protocol (inherited from ten_protocol_t)
//
// The condition which a base protocol can be closed is when the implementation
// protocol is closed.
//
// Closing flow is as follows.
//
// - Stage 1 : 'Need to close' notification stage
//
//   TEN world -> (notify) ->
//                         base protocol -> (notify) ->
//                                                   implementation protocol
//
// - Stage 2 : 'I cam closed' notification stage
//
//                               <- (I am closed) <- implementation protocol
//        <- (I am closed) <- base protocol
//   TEN world
//
//   The time that a protocol (no matter it is a base protocol or an
//   implementation protocol) can be closed is that when all resources held in
//   that protocol are closed, then that protocol can be closed.
static bool ten_protocol_could_be_close(ten_protocol_t *self) {
  TEN_ASSERT(self && ten_protocol_check_integrity(self, true),
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
  //   ten_protocol_on_close();
  // }
  //
  // y_do_close() {
  //   ten_protocol_on_close();
  // }
  //
  // ten_protocol_on_close() {
  //   if (ten_protocol_could_be_close()) {
  //     ten_protocol_do_close();
  //   }
  // }
  //
  // ten_protocol_could_be_close() {
  //   return x_is_closed && y_is_closed;
  // }
  //
  return true;
}

void ten_protocol_on_close(ten_protocol_t *self) {
  TEN_ASSERT(self && ten_protocol_check_integrity(self, true),
             "Should not happen.");

  if (!ten_protocol_could_be_close(self)) {
    TEN_LOGD("Could not close alive base protocol.");
    return;
  }
  TEN_LOGD("Close base protocol.");

  ten_closeable_action_to_close_myself_done(&self->closeable, NULL);
}

bool ten_protocol_is_closing(ten_protocol_t *self) {
  TEN_ASSERT(self &&
                 // TEN_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is designed to be used in any
                 // threads, that's why 'is_closing' is of atomic type.
                 ten_protocol_check_integrity(self, false),
             "Should not happen.");
  return ten_atomic_load(&self->is_closing) == 1;
}

bool ten_protocol_is_closed(ten_protocol_t *self) {
  TEN_ASSERT(self && ten_protocol_check_integrity(self, true),
             "Access across threads.");

  return self->is_closed;
}

void ten_protocol_set_on_closed(ten_protocol_t *self,
                                ten_protocol_on_closed_func_t on_closed,
                                void *on_closed_data) {
  TEN_ASSERT(self && ten_protocol_check_integrity(self, true),
             "Should not happen.");

  self->on_closed = on_closed;
  self->on_closed_data = on_closed_data;
}

/**
 * @brief The overall closing flow is as follows.
 *
 * ten_protocol_close
 *   => close implemented protocol
 *      (when the implementation protocol is closed)
 *         => ten_protocol_on_close
 */
void ten_protocol_close(ten_protocol_t *self) {
  TEN_ASSERT(self, "Should not happen.");
  TEN_ASSERT(ten_protocol_check_integrity(self, true), "Should not happen.");

  // As a design principle, the resources must be closed from top to bottom. So
  // the 'ten_protocol_close()' is expected to be called in the connection
  // (i.e., a 'ten_connection_t' object, and the protocol is used in
  // communication) or app (i.e., a 'ten_app_t' object, and the protocol is used
  // as endpoint).
  //
  // The brief closing flow is as follows:
  //
  // ten_connection_close()
  //   => ten_protocol_close()
  //     => ten_closeable_close()
  //       => ten_protocol_action_to_close_myself()
  //         => ten_protocol_t::close()  // closes the implementation
  if (ten_atomic_bool_compare_swap(&self->is_closing, 0, 1)) {
    switch (self->role) {
      case TEN_PROTOCOL_ROLE_LISTEN:
        TEN_LOGD("Try to close listening protocol: %s",
                 ten_string_get_raw_str(&self->uri));
        break;
      case TEN_PROTOCOL_ROLE_IN_INTERNAL:
      case TEN_PROTOCOL_ROLE_IN_EXTERNAL:
      case TEN_PROTOCOL_ROLE_OUT_INTERNAL:
      case TEN_PROTOCOL_ROLE_OUT_EXTERNAL:
        TEN_LOGD("Try to close communication protocol: %s",
                 ten_string_get_raw_str(&self->uri));
        break;
      default:
        TEN_ASSERT(0, "Should not happen.");
        break;
    }

    // TODO(Wei): Change to intend_to_close()?
    ten_closeable_close(&self->closeable);
  }
}

/**
 * TODO(Wei): After the implementation protocol implements the 'ten_closeable_t'
 * interface, refine this function accordingly.
 */
void ten_protocol_action_to_close_myself(
    ten_closeable_t *self_, TEN_UNUSED void *close_myself_data,
    TEN_UNUSED ten_closeable_action_to_close_myself_done_func_t
        close_myself_done) {
  ten_protocol_t *self = CONTAINER_OF_FROM_OFFSET(self_, self_->offset_in_impl);
  TEN_ASSERT(self && ten_protocol_check_integrity(self, true),
             "Should not happen.");

  self->close(self);
}

void ten_protocol_on_impl_intends_to_close(
    ten_closeable_t *impl, void *self_closeable,
    TEN_UNUSED void *on_intend_to_close_data) {
  TEN_ASSERT(impl && ten_closeable_check_integrity(impl, true),
             "Access across threads.");

  ten_closeable_t *closeable = (ten_closeable_t *)self_closeable;
  TEN_ASSERT(closeable && ten_closeable_check_integrity(closeable, true),
             "Access across threads.");

  ten_protocol_t *self =
      CONTAINER_OF_FROM_OFFSET(closeable, closeable->offset_in_impl);
  TEN_ASSERT(self && ten_protocol_check_integrity(self, true),
             "Should not happen.");

  bool to_close_protocol = false;

  if (self->role == TEN_PROTOCOL_ROLE_IN_EXTERNAL) {
    // If the protocol (i.e., self) is created when the server accepts a client
    // outside of the ten world (ex: browser), we can not do anything except
    // closing the protocol and its bound resources, and expect the client to
    // handle the situation such as reconnecting.
    to_close_protocol = true;
  }

  if (self->cascade_close_upward) {
    // If the 'cascade_close_upward' flag is true, which means the protocol (the
    // implementation protocol who owns the physical connection) will be closed
    // automatically when the underlying lower layers are closed. As a
    // principle, the protocol (the 'ten_protocol_t') could only be closed from
    // the runtime side. In other words, the implementation protocol owns its
    // underlying resources such as the physical connections, but the life cycle
    // (ex: closing and destroying) of the protocols (the 'ten_protocol_t') are
    // managed by the runtime.
    to_close_protocol = true;
  }

  if (to_close_protocol) {
    // TODO(Liu): Pass the 'intend_to_close' event to the owner (i.e.,
    // ten_connection_t or ten_app_t) of the protocol after they implement the
    // closeable interface.
    ten_protocol_close(self);
  }
}

void ten_protocol_on_impl_closed_all_done(
    TEN_UNUSED ten_closeable_t *impl, void *self_closeable,
    TEN_UNUSED void *on_closed_all_done_data) {
  ten_closeable_t *closeable = (ten_closeable_t *)self_closeable;
  TEN_ASSERT(closeable && ten_closeable_check_integrity(closeable, true),
             "Access across threads.");

  ten_protocol_t *self =
      CONTAINER_OF_FROM_OFFSET(closeable, closeable->offset_in_impl);
  TEN_ASSERT(self && ten_protocol_check_integrity(self, true),
             "Should not happen.");
  TEN_ASSERT(!self->is_closed, "Should not happen.");

  self->is_closed = true;

  if (self->on_closed) {
    // Call the registered on_closed callback when the base protocol is closed.
    self->on_closed(self, self->on_closed_data);
  }
}
