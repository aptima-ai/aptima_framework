//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/protocol/close.h"
#include "axis_utils/container/list.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/lib/ref.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"

#define axis_PROTOCOL_SIGNATURE 0x72CC0E4B2E807E08U

typedef struct axis_connection_t axis_connection_t;
typedef struct axis_app_t axis_app_t;
typedef struct axis_engine_t axis_engine_t;
typedef struct axis_runloop_t axis_runloop_t;
typedef struct axis_addon_host_t axis_addon_host_t;
typedef struct axis_protocol_t axis_protocol_t;

typedef enum axis_PROTOCOL_ATTACH_TO {
  axis_PROTOCOL_ATTACH_TO_INVALID,

  // The listening protocol will be attached to a APTIMA app.
  axis_PROTOCOL_ATTACH_TO_APP,

  // All protocols except the listening one will be attached to a APTIMA
  // connection.
  axis_PROTOCOL_ATTACH_TO_CONNECTION,
} axis_PROTOCOL_ATTACH_TO;

// The protocols will be created in the following scenarios:
// - A listening protocol when the app acts as a server.
// - A communication protocol when the server accepts a client from another app
//   through the graph flow.
// - A communication protocol when the server accepts a client from the external
//   world.
// - A client protocol when connecting to another app through the graph flow.
// - A client protocol when connecting to the external server.
typedef enum axis_PROTOCOL_ROLE {
  axis_PROTOCOL_ROLE_INVALID,

  // The listening endpoint.
  axis_PROTOCOL_ROLE_LISTEN,

  // The protocol whose role is 'axis_PROTOCOL_ROLE_IN_*' means that it is
  // created when the listening endpoint accepts a client. And the client might
  // be another aptima app or the external world such as the browser, so we use the
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
  // 'axis_protocol_t::on_input()' function will be called in the whole life
  // cycle of the protocol whose role is 'axis_PROTOCOL_ROLE_IN_*'. Ex: if a
  // client outside of the aptima world wants to send a message to the extension in
  // the aptima app, the 'axis_protocol_t::on_input()' function will be called to
  // receive the client message, but the 'axis_protocol_t::on_output()'
  // function will also be called when the extension wants to return a message
  // to the client side. A simple diagram is as follows:
  //
  //           [ external client ]
  //                 |     ^
  //     on_input()  |     | result
  //                 V     |
  //            [ axis_protocol_t ]
  //                 |     ^
  //        message  |     | on_output()
  //                 V     |
  //              [ extension ]
  axis_PROTOCOL_ROLE_IN_INTERNAL,
  axis_PROTOCOL_ROLE_IN_EXTERNAL,

  // The protocol whose role is 'axis_PROTOCOL_ROLE_OUT_*' means that it is
  // created when connecting to the remote server. And the remote server might
  // be another aptima app or the external server such as a nginx, so we use the
  // 'INTERNAL' and 'EXTERNAL' to distinguish them. The 'EXTERNAL' protocols are
  // always created when the engine handles the 'connect_to' cmds. So even if
  // the extension wants to connect to another aptima app through the 'connect_to'
  // cmd, the created protocol will be treated as 'EXTERNAL'.
  //
  // Please keep in mind that the _OUT_ in the name does _not_ mean that this
  // protocol will only process the output data. Not only the
  // 'axis_protocol_t::on_output()' function will be called in the whole life
  // cycle of the protocol whose role is 'axis_PROTOCOL_ROLE_OUT_*'. Ex: if an
  // extension wants to sent a message to the remote server, the
  // 'axis_protocol_t::on_output()' function will be called, but the
  // 'axis_protocol_t::on_input()' will also be called when the remote server
  // returns a result to the extension. A simple diagram is as follows:
  //
  //                [ extension ]
  //                   |     ^
  //       on_output() |     | result
  //                   V     |
  //             [ axis_protocol_t ]
  //                   |     ^
  //           message |     | on_input()
  //                   V     |
  //              [ remote server ]
  axis_PROTOCOL_ROLE_OUT_INTERNAL,
  axis_PROTOCOL_ROLE_OUT_EXTERNAL,

  axis_PROTOCOL_ROLE_IN_DEFAULT = axis_PROTOCOL_ROLE_IN_INTERNAL,
  axis_PROTOCOL_ROLE_OUT_DEFAULT = axis_PROTOCOL_ROLE_OUT_INTERNAL,
} axis_PROTOCOL_ROLE;

// @{
// The interface API of 'axis_protocol_t'
typedef void (*axis_protocol_close_func_t)(axis_protocol_t *self);

typedef void (*axis_protocol_on_output_func_t)(axis_protocol_t *self,
                                              axis_list_t *output);

typedef axis_connection_t *(*axis_protocol_on_client_accepted_func_t)(
    axis_protocol_t *self, axis_protocol_t *new_protocol);

typedef void (*axis_protocol_lisaxis_func_t)(
    axis_protocol_t *self, const char *uri,
    axis_protocol_on_client_accepted_func_t on_client_accepted);

typedef void (*axis_protocol_on_server_connected_func_t)(axis_protocol_t *self,
                                                        bool success);

typedef void (*axis_protocol_connect_to_func_t)(
    axis_protocol_t *self, const char *uri,
    axis_protocol_on_server_connected_func_t on_server_connected);

typedef void (*axis_protocol_migrate_func_t)(axis_protocol_t *self,
                                            axis_engine_t *engine,
                                            axis_connection_t *connection,
                                            axis_shared_ptr_t *cmd);

typedef void (*axis_protocol_on_migrated_func_t)(axis_protocol_t *self);

typedef void (*axis_protocol_clean_func_t)(axis_protocol_t *self);

typedef void (*axis_protocol_on_cleaned_for_internal_func_t)(
    axis_protocol_t *self);

/**
 * @brief This function will be called to notify the implementation protocol in
 * the following two scenarios:
 *
 * - The migration in the APTIMA world has been completed, all the resources bound
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
 * 'axis_protocol_asynced_on_base_protocol_cleaned()'.
 */
typedef void (*axis_protocol_on_cleaned_for_external_func_t)(
    axis_protocol_t *self, bool is_migration_state_reset);
// @}

/**
 * @brief This is the base class of all the protocols. All the protocols must
 * inherit 'axis_protocol_t' and implement the necessary apis such as
 * 'on_accepted', 'on_input' and 'on_output'. As the implementation might or
 * might not have its own runloop, we provide the following two standard layers:
 *
 * - axis_protocol_integrated_t
 *   It uses the runloop of the aptima app or engine.
 *
 * - axis_protocol_asynced_t
 *   It supposes that the implementation protocol has its own runloop and runs
 *   in another thread.
 *
 * The relationship between those classes is as follows:
 *
 *                           axis_protocol_t
 *                                  ^
 *                                  |  <== inherits
 *                                  |
 *                          +-----------------+
 *                          |                 |
 *           axis_protocol_integrated_t     axis_protocol_asynced_t
 *                      ^                             ^
 *                      |  <== inherits               |  <== inherits
 *              +---------------+             +---------------+
 *              |               |             |               |
 *            impl             impl          impl            impl
 *        (ex: msgpack)                                 (ex: libws_http)
 */
typedef struct axis_protocol_t {
  axis_signature_t signature;

  /**
   * @note The base protocol and the implementation protocol may belongs
   * different threads. The base protocol's belonging thread should be same with
   * the related connection's. The implementation protocol may has its own
   * thread.
   */
  axis_sanitizer_thread_check_t thread_check;

  axis_ref_t ref;

  axis_addon_host_t *addon_host;

  // Start to trigger the closing of the base protocol.
  axis_atomic_t is_closing;

  // This field is used to mark that the base protocol is totally closed, it
  // means that all the resources bound to the base protocol has been closed.
  //
  // Now the only underlying resource of the base protocol is the implementation
  // protocol, so we do not use a field such as 'impl_is_closed' to store the
  // closed state of the implementation.
  bool is_closed;

  // Trigger binding resource to close, like connection/stream.
  axis_protocol_on_closed_func_t on_closed;
  void *on_closed_data;

  // This is the uri of this protocol represents.
  //   - For listening protocol, this is the local uri.
  //   - For communication protocol, this is the remote uri.
  axis_string_t uri;

  axis_PROTOCOL_ROLE role;

  // Even though this field will be access from multi threads (e.g., the
  // implementation protocol thread), but it is immutable after the assignment
  // in the app (e.g., the listening protocol, and the communication protocol
  // when client accepted) or engine thread (e.g., the communication protocol
  // when connecting to the remote server).
  //
  // Note that if this field might be modified in multi threads, the
  // modifications to 'attach_to' and 'attached_target' must be done in one
  // atomic operation.
  axis_PROTOCOL_ATTACH_TO attach_to;
  union {
    axis_app_t *app;  // The app where this protocol resides.
    axis_connection_t
        *connection;  // The connection where this protocol attached.
  } attached_target;

  // Used to react the closing request.
  axis_protocol_close_func_t close;

  // Used to react the listening request.
  axis_protocol_lisaxis_func_t listen;

  // Used to react the connect_to request.
  axis_protocol_connect_to_func_t connect_to;

  // Used to react the migration to new runloop request.
  axis_protocol_migrate_func_t migrate;

  // Used to clean the resources bound to the old runloop.
  axis_protocol_clean_func_t clean;

  // TODO(Wei): Have an 'on_input' field here.

  // Used to handle the output APTIMA messages to the remote.
  axis_protocol_on_output_func_t on_output;

  // This is the callback function when this protocol is migrated to the new
  // runloop.
  axis_protocol_on_migrated_func_t on_migrated;

  // This is the callback function when all the resources bound to the old
  // runloop is cleaned.
  axis_protocol_on_cleaned_for_internal_func_t on_cleaned_for_internal;
  axis_protocol_on_cleaned_for_external_func_t on_cleaned_for_external;

  /**
   * @brief This is the control flag which is used to determine whether to close
   * the protocol when the underlying low layers are closed.
   *
   * Please keep in mind that this flag is used to close 'ourselves' when the
   * resources owned by us are closed, it is not used to close our 'owner' when
   * we are closed. We do _not_ have the permission to control the behavior of
   * our owners.
   *
   * @note This flag can only be set in the implementation protocol.
   *
   * @note As the protocol is paired with a connection (i.e.,
   * 'axis_connection_t'), and the connection is paired with a remote (i.e.,
   * 'axis_remote_t') if a remote has been created by the engine. The life cycle
   * of the protocol, connection and remote must be same. In other words, the
   * connection should be closed when the protocol is closed, and the remote
   * should be closed when the connection is closed. So the
   * 'cascade_close_upward' flag in the connection and remote are always true.
   */
  bool cascade_close_upward;

  // @{
  // These fields is for storing the input data.
  //
  // TODO(Liu): The 'in_lock' field is useless for now:
  // - For implementation implements the integrated protocol, all the reads and
  //   writes of 'in_msgs' are in the same thread. The accesses of 'in_msgs' are
  //   in sequence even in the migration stage.
  //
  // - For implementation implements the asynced protocol, all the 'in_msgs' are
  //   transferred through the runloop task.
  axis_mutex_t *in_lock;
  axis_list_t in_msgs;
  // @}

  // @{
  // These fields is for storing the output data.
  axis_mutex_t *out_lock;
  axis_list_t out_msgs;
  // @}
} axis_protocol_t;

axis_RUNTIME_PRIVATE_API bool axis_protocol_cascade_close_upward(
    axis_protocol_t *self);

axis_RUNTIME_PRIVATE_API void axis_protocol_listen(
    axis_protocol_t *self, const char *uri,
    axis_protocol_on_client_accepted_func_t on_client_accepted);

axis_RUNTIME_PRIVATE_API void axis_protocol_connect_to(
    axis_protocol_t *self, const char *uri,
    axis_protocol_on_server_connected_func_t on_server_connected);

axis_RUNTIME_PRIVATE_API void axis_protocol_migrate(
    axis_protocol_t *self, axis_engine_t *engine, axis_connection_t *connection,
    axis_shared_ptr_t *cmd, axis_protocol_on_migrated_func_t on_migrated);

axis_RUNTIME_PRIVATE_API void axis_protocol_clean(
    axis_protocol_t *self,
    axis_protocol_on_cleaned_for_internal_func_t on_cleaned_for_internal);

axis_RUNTIME_PRIVATE_API void axis_protocol_update_belonging_thread_on_cleaned(
    axis_protocol_t *self);

axis_RUNTIME_PRIVATE_API void axis_protocol_attach_to_connection(
    axis_protocol_t *self, axis_connection_t *connection);

/**
 * @brief Try to send one message to check if the connection needs to be
 * migrated when handling the first message, or just send one message after the
 * migration is completed.
 */
axis_RUNTIME_PRIVATE_API void axis_protocol_on_input(axis_protocol_t *self,
                                                   axis_shared_ptr_t *msg);

/**
 * @brief Send messages in batch after the migration is completed.
 *
 * @note The caller side must be responsible to ensure that the migration has
 * been completed.
 */
axis_RUNTIME_PRIVATE_API void axis_protocol_on_inputs(axis_protocol_t *self,
                                                    axis_list_t *msgs);

axis_RUNTIME_PRIVATE_API axis_string_t *axis_protocol_uri_to_transport_uri(
    const char *uri);

axis_RUNTIME_PRIVATE_API void axis_protocol_set_uri(axis_protocol_t *self,
                                                  axis_string_t *uri);

axis_RUNTIME_PRIVATE_API void axis_protocol_set_addon(
    axis_protocol_t *self, axis_addon_host_t *addon_host);

axis_RUNTIME_PRIVATE_API void axis_protocol_determine_default_property_value(
    axis_protocol_t *self);

axis_RUNTIME_API axis_PROTOCOL_ATTACH_TO
axis_protocol_attach_to(axis_protocol_t *self);

axis_RUNTIME_API bool axis_protocol_check_integrity(axis_protocol_t *self,
                                                  bool check_thread);

axis_RUNTIME_API void axis_protocol_init(
    axis_protocol_t *self, const char *name, axis_protocol_close_func_t close,
    axis_protocol_on_output_func_t on_output, axis_protocol_lisaxis_func_t listen,
    axis_protocol_connect_to_func_t connect_to,
    axis_protocol_migrate_func_t migrate, axis_protocol_clean_func_t clean);

axis_RUNTIME_API void axis_protocol_deinit(axis_protocol_t *self);

axis_RUNTIME_PRIVATE_API void axis_protocol_attach_to_app(axis_protocol_t *self,
                                                        axis_app_t *app);

axis_RUNTIME_PRIVATE_API void axis_protocol_attach_to_app_and_thread(
    axis_protocol_t *self, axis_app_t *app);

axis_RUNTIME_PRIVATE_API void axis_protocol_send_msg(axis_protocol_t *self,
                                                   axis_shared_ptr_t *msg);

/**
 * @return NULL if the protocol attaches to a connection who is in migration.
 */
axis_RUNTIME_PRIVATE_API axis_runloop_t *axis_protocol_get_attached_runloop(
    axis_protocol_t *self);

axis_RUNTIME_PRIVATE_API bool axis_protocol_role_is_communication(
    axis_protocol_t *self);

axis_RUNTIME_PRIVATE_API bool axis_protocol_role_is_listening(
    axis_protocol_t *self);
