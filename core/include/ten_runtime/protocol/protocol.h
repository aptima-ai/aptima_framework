//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_runtime/protocol/close.h"
#include "ten_utils/container/list.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/lib/string.h"

typedef struct ten_connection_t ten_connection_t;
typedef struct ten_protocol_t ten_protocol_t;
typedef struct ten_app_t ten_app_t;
typedef struct ten_engine_t ten_engine_t;
typedef struct ten_addon_host_t ten_addon_host_t;
typedef struct ten_runloop_t ten_runloop_t;
typedef struct ten_protocol_context_store_t ten_protocol_context_store_t;

// The protocols will be created in the following scenarios:
// - A listening protocol when the app acts as a server.
// - A communication protocol when the server accepts a client from another app
//   through the graph flow.
// - A communication protocol when the server accepts a client from the external
//   world.
// - A client protocol when connecting to another app through the graph flow.
// - A client protocol when connecting to the external server.
typedef enum TEN_PROTOCOL_ROLE {
  TEN_PROTOCOL_ROLE_INVALID,

  // The listening endpoint.
  TEN_PROTOCOL_ROLE_LISTEN,

  // The protocol whose role is 'TEN_PROTOCOL_ROLE_IN_*' means that it is
  // created when the listening endpoint accepts a client. And the client might
  // be another ten app or the external world such as the browser, so we use the
  // 'INTERNAL' and 'EXTERNAL' to distinguish them.
  //
  // The reason why we need to distinguish the 'INTERNAL' and 'EXTERNAL' is that
  // the 'INTERNAL' is always created by the graph (i.e., the 'start_graph'
  // cmd). As the graph is the message flow between extensions in apps, so the
  // protocols created by the graph should be treated as persistent. In other
  // words, the life cycle of the 'INTERNAL' protocols should be equal to the
  // graph. The 'INTERNAL' protocols could _not_ be closed or destroyed until
  // the graph is closed, even if the physical connection is broken. On the
  // contrary, the 'EXTERNAL' protocols are created as needed, they should be
  // treated as temporary.
  //
  // Please keep in mind that the _IN_ in the name does _not_ mean that this
  // protocol will only process the input data. Not only the
  // 'ten_protocol_t::on_input()' function will be called in the whole life
  // cycle of the protocol whose role is 'TEN_PROTOCOL_ROLE_IN_*'. Ex: if a
  // client outside of the ten world wants to send a message to the extension in
  // the ten app, the 'ten_protocol_t::on_input()' function will be called to
  // receive the client message, but the 'ten_protocol_t::on_output()'
  // function will also be called when the extension wants to return a message
  // to the client side. A simple diagram is as follows:
  //
  //           [ external client ]
  //                 |     ^
  //     on_input()  |     | result
  //                 V     |
  //            [ ten_protocol_t ]
  //                 |     ^
  //        message  |     | on_output()
  //                 V     |
  //              [ extension ]
  TEN_PROTOCOL_ROLE_IN_INTERNAL,
  TEN_PROTOCOL_ROLE_IN_EXTERNAL,

  // The protocol whose role is 'TEN_PROTOCOL_ROLE_OUT_*' means that it is
  // created when connecting to the remote server. And the remote server might
  // be another ten app or the external server such as a nginx, so we use the
  // 'INTERNAL' and 'EXTERNAL' to distinguish them. The 'EXTERNAL' protocols are
  // always created when the engine handles the 'connect_to' cmds. So even if
  // the extension wants to connect to another ten app through the 'connect_to'
  // cmd, the created protocol will be treated as 'EXTERNAL'.
  //
  // Please keep in mind that the _OUT_ in the name does _not_ mean that this
  // protocol will only process the output data. Not only the
  // 'ten_protocol_t::on_output()' function will be called in the whole life
  // cycle of the protocol whose role is 'TEN_PROTOCOL_ROLE_OUT_*'. Ex: if an
  // extension wants to sent a message to the remote server, the
  // 'ten_protocol_t::on_output()' function will be called, but the
  // 'ten_protocol_t::on_input()' will also be called when the remote server
  // returns a result to the extension. A simple diagram is as follows:
  //
  //                [ extension ]
  //                   |     ^
  //       on_output() |     | result
  //                   V     |
  //             [ ten_protocol_t ]
  //                   |     ^
  //           message |     | on_input()
  //                   V     |
  //              [ remote server ]
  TEN_PROTOCOL_ROLE_OUT_INTERNAL,
  TEN_PROTOCOL_ROLE_OUT_EXTERNAL,

  TEN_PROTOCOL_ROLE_IN_DEFAULT = TEN_PROTOCOL_ROLE_IN_INTERNAL,
  TEN_PROTOCOL_ROLE_OUT_DEFAULT = TEN_PROTOCOL_ROLE_OUT_INTERNAL,
} TEN_PROTOCOL_ROLE;

// @{
// The interface API of 'ten_protocol_t'
typedef void (*ten_protocol_close_func_t)(ten_protocol_t *self);

typedef void (*ten_protocol_on_output_func_t)(ten_protocol_t *self,
                                              ten_list_t *output);

typedef void (*ten_protocol_listen_func_t)(ten_protocol_t *self,
                                           const char *uri);

typedef ten_connection_t *(*ten_protocol_on_client_accepted_func_t)(
    ten_protocol_t *self, ten_protocol_t *new_protocol);

typedef bool (*ten_protocol_connect_to_func_t)(ten_protocol_t *self,
                                               const char *uri);

typedef void (*ten_protocol_on_server_connected_func_t)(ten_protocol_t *self,
                                                        bool success);

typedef void (*ten_protocol_migrate_func_t)(ten_protocol_t *self,
                                            ten_engine_t *engine,
                                            ten_connection_t *connection,
                                            ten_shared_ptr_t *cmd);

typedef void (*ten_protocol_on_migrated_func_t)(ten_protocol_t *self);

typedef void (*ten_protocol_clean_func_t)(ten_protocol_t *self);

typedef void (*ten_protocol_on_cleaned_for_internal_func_t)(
    ten_protocol_t *self);

/**
 * @brief This function will be called to notify the implementation protocol in
 * the following two scenarios:
 *
 * - The migration in the TEN world has been completed, all the resources bound
 *   to the base protocol has been cleaned during the migration.
 *
 * - The migration has not been started as the expected engine was not found.
 *   The migration state should be reset, then the connection could be checked
 *   if the migration is needed when handling the subsequent messages.
 *
 * @param is_migration_state_reset Whether the migration state has been reset.
 *
 * @note This function is always called in the ENGINE thread. So if the
 * implementation protocol runs in its own thread, this function should care
 * about the thread context switch. Refer to
 * 'ten_protocol_asynced_on_base_protocol_cleaned()'.
 */
typedef void (*ten_protocol_on_cleaned_for_external_func_t)(
    ten_protocol_t *self, bool is_migration_state_reset);
// @}

typedef struct ten_protocol_t ten_protocol_t;

TEN_RUNTIME_API bool ten_protocol_check_integrity(ten_protocol_t *self,
                                                  bool check_thread);

TEN_RUNTIME_API void ten_protocol_init(
    ten_protocol_t *self, const char *name, ten_protocol_close_func_t close,
    ten_protocol_on_output_func_t on_output, ten_protocol_listen_func_t listen,
    ten_protocol_connect_to_func_t connect_to,
    ten_protocol_migrate_func_t migrate, ten_protocol_clean_func_t clean);

TEN_RUNTIME_API void ten_protocol_deinit(ten_protocol_t *self);

TEN_RUNTIME_API void ten_protocol_attach_to_app(ten_protocol_t *self,
                                                ten_app_t *app);

TEN_RUNTIME_API void ten_protocol_attach_to_app_and_thread(ten_protocol_t *self,
                                                           ten_app_t *app);

TEN_RUNTIME_API void ten_protocol_send_msg(ten_protocol_t *self,
                                           ten_shared_ptr_t *msg);

/**
 * @return NULL if the protocol attaches to a connection who is in migration.
 */
TEN_RUNTIME_API ten_runloop_t *ten_protocol_get_attached_runloop(
    ten_protocol_t *self);

TEN_RUNTIME_API ten_protocol_context_store_t *ten_protocol_get_context_store(
    ten_protocol_t *self);

TEN_RUNTIME_API bool ten_protocol_role_is_communication(ten_protocol_t *self);

TEN_RUNTIME_API bool ten_protocol_role_is_listening(ten_protocol_t *self);
