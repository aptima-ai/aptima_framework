//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_runtime/protocol/context.h"
#include "ten_utils/lib/ref.h"

#define TEN_PROTOCOL_CONTEXT_SIGNATURE 0x5A47EA3A49BD3EE2U

// The protocol context is closed completely, send a notification to its owner.
typedef void (*ten_protocol_context_on_closed_func_t)(
    ten_protocol_context_t *self, void *on_closed_data);

// If a protocol creates a thread outside of the TEN world to serve the protocol
// operations, from the perspective of the TEN world, a 'protocol context' needs
// to be created to represent this external thread.
//
// Therefore, a 'protocol context' corresponds to a 'external protocol thread',
// and vice versa.
//
// Ex:
// - In the case of 'libwebsockets' protocol, two 'protocol context'(s) will be
//   created to represent the libws-server-thread and libws-client-thread.
// - In the case of 'integrated' protocol, because there is no external threads
//   outside of the TEN world, so it's unnecessary to create protocol_context in
//   this case.

// The 'ten_protocol_context_t' (named 'TEN protocol context') is in the TEN
// world, and, normally, in the external protocol world, there will be their own
// protocol context (ex: libws protocol context in the libws world, and UAP
// protocol context in the UAP world).
//
// The owner of the TEN protocol context should be the
// 'ten_protocol_context_store_t'.

// The lifecycle of a 'ten_protocol_context_t' would be:
//
// - ten_app_t starts                                       |
//   - ten_protocol_context_store_create()                  | In TEN app thread
//     > The 'ten_protocol_context_store_t' is the owner of |
//       the 'ten_protocol_context_t'.                      |
//
// - ten_protocol_t::listen()                               |
//   - impl_protocol_t::listen()                            |
//     - implementation protocol context created.           |
//     - ten_protocol_context_create()                      | In TEN app thread
//       > Add 'ten_protocol_context_t' into                |
//         'ten_protocol_context_store_t', the belonging    |
//         thread and the attached runloop of the TEN       |
//         protocol context inherits from                   |
//         'ten_protocol_context_store_t'.                  |
//       > Then start the 'external protocol thread', it's
//         the belonging thread of the implementation
//         protocol context.
//
// - ten_protocol_t::connect_to()                        |
//   > The execution process is roughly the same as      | In TEN engine thread
//     ten_protocol_t::listen(), the only difference is  |
//     'connect_to()' executes in the TEN engine thread. |
//
// - ten_app_close()                                     |
//   - ten_protocol_context_store_close()                | In TEN app thread
//     - ten_protocol_context_close()                    |
//       - ten_protocol_context_t::close_impl            |
//         > Switch to the 'external protocol thread' to |
//           close the implementation protocol context.  |
//
//       - impl_protocol_context_close()                      | In the external
//                                                            | protocol thread
//       - ten_protocol_context_on_implemented_closed_async() |
//         > Switch to the belonging thread of the TEN
//           protocol context (the TEN app thread).
//
//       - ten_protocol_context_t::on_closed                |
//         > Remove the 'ten_protocol_context_t' from       |
//           'ten_protocol_context_store_t'.                | In TEN app thread
//       - ten_protocol_context_destroy()                   |
//         - ten_protocol_context_t::destroy_impl           |
//           > Destroy the implementation protocol context  |
//   - ten_protocol_context_store_t::on_closed              |

// According to the comments above, the implementation protocol context maybe
// accessed from three threads:
// 1. the TEN app thread
// 2. the TEN engine thread
// 3. the external protocol thread
//
// The R/W operations to the implementation protocol context from those
// threads maybe performed concurrently. Ex: when the TEN engine processes a
// 'connect_to' cmd, and at the same time, the TEN app is being closed. The
// external thread maybe stopped, as it does not receive the 'connect_to'
// request yet, and then notify the TEN app thread that the implementation
// protocol context could be closed. The engine thread may access the memory
// space already destroyed.
typedef struct ten_protocol_context_t {
  ten_signature_t signature;
  ten_sanitizer_thread_check_t thread_check;

  // The owner of 'ten_protocol_context_t'.
  ten_protocol_context_store_t *context_store;

  ten_string_t key_in_store;

  ten_ref_t ref;

  ten_protocol_context_on_closed_func_t on_closed;
  void *on_closed_data;

  ten_protocol_context_close_impl_func_t close_impl;
  ten_protocol_context_destroy_impl_func_t destroy_impl;

  // The pointer of the implementation protocol context.
  //
  // The belonging threads between the TEN protocol context and the
  // implementation protocol context should be different in most cases, so the
  // implementation protocol context will use composition instead of inheritance
  // to integrate the TEN protocol context. In other words, the implementation
  // protocol context will keep a pointer of 'ten_protocol_context_t'.
  //
  // The protocol contexts will only be closed from the TEN world due to the
  // closure of the TEN app. The TEN protocol context will be closing before
  // the implementation protocol context, and the TEN protocol context needs to
  // close the implementation when it is closing, refer to the comments above.
  //
  // So the pointer to the implementation protocol context, rather
  // than the TEN protocol context, should be passed as the parameter of
  // 'close_impl()' and 'destroy_impl()'.
  void *impl_protocol_context;

  // When the TEN app or TEN client is closing:
  // 1) trigger all protocol context in the TEN app or TEN client to close.
  // 2) trigger the implemented protocol context to close.
  // 3) when the implemented protocol context is closed, notify the base
  //    protocol context to continue to close.
  // 4) when all the base protocol context is closed, then the TEN app or TEN
  //    client could be closed.
  ten_atomic_t is_closing;

  // This is a simple check which is used to ensure that the implementation
  // protocol context should be closed just once.
  bool impl_is_closed;
} ten_protocol_context_t;

TEN_RUNTIME_PRIVATE_API bool ten_protocol_context_check_integrity(
    ten_protocol_context_t *self, bool thread_check);

TEN_RUNTIME_PRIVATE_API void ten_protocol_context_set_on_closed(
    ten_protocol_context_t *self,
    ten_protocol_context_on_closed_func_t on_closed, void *on_closed_data);

TEN_RUNTIME_PRIVATE_API void ten_protocol_context_close(
    ten_protocol_context_t *self);
