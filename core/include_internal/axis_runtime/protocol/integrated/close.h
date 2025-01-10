//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

// The brief closing flow of the 'integrated' protocol is as follows:
//
// 1. If the protocol is being closed from the aptima world, ex: the remote is
//    closed.
//
// axis_remote_t   axis_connection_t    axis_protocol_t  axis_protocol_integrated_t
// -----------------------------------------------------------------------------
// close() {
//   is_closing = 1
// }
//
//                  close() {
//                    is_closing = 1
//                  }
//
//                                      close() {
//                                        is_closing = 1
//                                      }
//
//                                                         close() {
//                                                           close_stream()
//                                                         }
//
//                                                         on_stream_closed() {
//                                                           on_closed()
//                                                         }
//
//                                       // on_impl_closed
//                                       do_close() {
//                                         is_closed = true
//                                         on_closed()
//                                       }
//
//                  // on_protocol_closed
//                  do_close() {
//                    is_closed = true
//                    on_closed()
//                  }
//
// // on_connection_closed
// do_close() {
//    is_closed = true
//    on_closed()
// }
//
// 2. If the protocol is being closed from itself, ex: the physical connection
//    is broken.
//
// axis_remote_t   axis_connection_t    axis_protocol_t  axis_protocol_integrated_t
// -----------------------------------------------------------------------------
//                                                       on_stream_closed() {
//                                                         axis_protocol_close()
//                                                       }
//
//                                    // axis_protocol_close
//                                    close() {
//                                      is_closing = 1
//                                    }
//
//                                                       close() {
//                                                         // the stream is
//                                                         // already closed
//                                                         on_closed()
//                                                       }
//
//                                    do_close() {
//                                      is_closed = true
//                                      on_closed()
//                                    }
//
//                  // on_protocol_closed
//                  do_close() {
//                    close()
//                  }
//
//                  close() {
//                    is_closing = 1
//                    // the protocol is already closed
//                    on_closed()
//                  }
//
// // on_connection_closed
// do_close() {
//   close()
// }
//
// close() {
//   is_closing = 1
//   // the connection is already closed
//   on_closed()
// }
//

typedef struct axis_protocol_integrated_t axis_protocol_integrated_t;

axis_RUNTIME_PRIVATE_API void axis_protocol_integrated_on_close(
    axis_protocol_integrated_t *self);

axis_RUNTIME_PRIVATE_API void axis_protocol_integrated_on_stream_closed(
    axis_protocol_integrated_t *self);

axis_RUNTIME_PRIVATE_API void axis_protocol_integrated_on_transport_closed(
    axis_protocol_integrated_t *self);

axis_RUNTIME_PRIVATE_API void axis_protocol_integrated_close(
    axis_protocol_integrated_t *self);
