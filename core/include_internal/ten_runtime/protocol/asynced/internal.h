//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

// The ten_protocol_asynced_t acts as a bridge between the implementation
// protocol (we consider it as the 'external') and the ten world (we consider it
// as the 'internal'). Some fields and functions of ten_protocol_asynced_t can
// only be accessed from the external, and others are internal. So we split the
// functions of ten_protocol_asynced_t into two parts. The functions in
// 'internal.h' can only be accessed from the ten world. In other words, all the
// functions _must_ be declared with 'TEN_RUNTIME_PRIVATE_API'.

#include "ten_runtime/ten_config.h"

#include <stdbool.h>

typedef struct ten_protocol_asynced_t ten_protocol_asynced_t;

typedef void (*ten_protocol_asynced_task_handler_func_t)(
    ten_protocol_asynced_t *self, void *arg);

typedef struct ten_protocol_asynced_task_t {
  ten_protocol_asynced_task_handler_func_t handler;
  void *arg;
} ten_protocol_asynced_task_t;

// The implementation protocol is always being closed from the ten world due to
// the closure of its owner (i.e., ten_connection_t or ten_app_t). And in the
// meantime, the resources in the implementation (ex: physical connection) maybe
// closed from the implementation thread.
//
// The brief closing flow of the 'asynced' protocol is as follows:
//
// 1. If the protocol is being closed from the ten world, ex: the ten_app_t is
//    closed.
//
//    Note that ten_libws_worker_t will implement the 'ten_closeable_t'
//    interface too.
//
//  ten_protocol_t         ten_protocol_asynced_t         ten_libws_worker_t
// (ten_closeable_t)          (ten_closeable_t)
//     ^                           |      ^                  close_async()
//     |                           |      |                       |
//     +--- underlying_resource ---+      +-- action_to_close() --+
//
//  // trigger by ten_connection_close() or ten_app_close()
//  ten_protocol_close() {
//    ten_closeable_close()
//  }
//
//  ten_closeable_close() {
//     close_owned()  ---------------+
//  }                                |
//                                   V
//                           ten_closeable_close() {
//                             action_to_close()      -------+
//                           }                               |
//                                                           V
//                                                        // in ten world
//                                                        action_to_close() {
//                                                          close_async()
//                                                        }
//
//                                                        // in the external
//                                                        // thread
//                                                        close() {
//                                                          close_conn()
//                                                        }
//
//                                                        on_conn_close() {
//                                                          on_closed()
//                                                        }
//
//                                                        on_closed() {
//                                    +------------------   closed_done_async()
//                                    |                   }
//                                    V
//                         // in the ten world
//                         action_to_close_done() {
//        +---------------   on_closed()
//        |                }
//        V
// ten_closeable_on_closed() {
//    on_closed()
//    // continue to call ten_connection_t::on_closed()...
// }
//
// 2. If the resources in the implementation are closed first, ex: the physical
//    connection is broken.
//
//  ten_protocol_t         ten_protocol_asynced_t         ten_libws_worker_t
// (ten_closeable_t)          (ten_closeable_t)
//     ^                            |     ^                  close_async()
//     |                            |     |                       |
//     +--- underlying_resource ----+     +-- intend_to_close() --+
//
//                                                      // in the external
//                                                      // thread
//                                                     on_conn_close() {
//                                 +------------------   intend_to_close_async()
//                                 |                   }
//                                 V
//                          // in the ten world
//                          intend_to_close() {
//                                     |
//                                     |
//                            is_closing_root() -------------------------+
//                          }                                            |
//                                                                       |
// is_closing_root() { <-------------------------------------------------+
//   if (is_root) {
//     ten_closeable_close()
//   } else {
//     announce_intend_to_close()
//   }
// }
//
// ten_closeable_close() {
//   // Start to close, same as the case 1.
// }

TEN_RUNTIME_PRIVATE_API void ten_protocol_asynced_init_closeable(
    ten_protocol_asynced_t *self);

TEN_RUNTIME_PRIVATE_API void ten_protocol_asynced_intends_to_close_task(
    void *self_, void *arg);

TEN_RUNTIME_PRIVATE_API void ten_protocol_asynced_on_impl_closed_task(void *self_,
                                                                    void *arg);

/**
 * @brief If the protocol attaches to a connection (i.e., the 'ten_connection_t'
 * object), it's not always safe to retrieve the runloop of the base protocol
 * (i.e., the 'ten_protocol_t' object) as the connection might be in migration.
 *
 * @return true if the 'migration_state' is INIT or DONE, otherwise false.
 */
TEN_RUNTIME_PRIVATE_API bool
ten_protocol_asynced_safe_to_retrieve_runtime_runloop(
    ten_protocol_asynced_t *self);

/**
 * @brief Submit a task to the runloop of the base protocol (i.e., the
 * 'ten_protocol_t') if the connection migration is not started or has been
 * completed. Otherwise, cache the task in the
 * 'ten_protocol_asynced_t::pending_task_queue'.
 *
 * @param handler_if_in_migration The handler to process the task once the
 * migration is completed. This function will be called from the external
 * protocol thread.
 *
 * @param runloop_task_handler The handler to process the task in the ten
 * app/engine thread.
 */
TEN_RUNTIME_PRIVATE_API void ten_protocol_asynced_post_task_to_ten(
    ten_protocol_asynced_t *self,
    ten_protocol_asynced_task_handler_func_t handler_if_in_migration,
    void (*runloop_task_handler)(void *, void *), void *arg);

TEN_RUNTIME_PRIVATE_API void ten_protocol_asynced_close_impl(
    ten_protocol_asynced_t *self, void *arg);
