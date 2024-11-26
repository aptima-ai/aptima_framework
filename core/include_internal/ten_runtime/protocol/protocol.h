//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "include_internal/ten_runtime/protocol/close.h"
#include "ten_runtime/protocol/protocol.h"
#include "ten_utils/container/list.h"
#include "ten_utils/lib/mutex.h"
#include "ten_utils/lib/ref.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/lib/string.h"

#define TEN_PROTOCOL_SIGNATURE 0x72CC0E4B2E807E08U

typedef struct ten_connection_t ten_connection_t;
typedef struct ten_app_t ten_app_t;
typedef struct ten_engine_t ten_engine_t;
typedef struct ten_runloop_t ten_runloop_t;

typedef enum TEN_PROTOCOL_ATTACH_TO {
  TEN_PROTOCOL_ATTACH_TO_INVALID,

  // The listening protocol will be attached to a TEN app.
  TEN_PROTOCOL_ATTACH_TO_APP,

  // All protocols except the listening one will be attached to a TEN
  // connection.
  TEN_PROTOCOL_ATTACH_TO_CONNECTION,
} TEN_PROTOCOL_ATTACH_TO;

/**
 * @brief This is the base class of all the protocols. All the protocols must
 * inherit 'ten_protocol_t' and implement the necessary apis such as
 * 'on_accepted', 'on_input' and 'on_output'. As the implementation might or
 * might not have its own runloop, we provide the following two standard layers:
 *
 * - ten_protocol_integrated_t
 *   It uses the runloop of the ten app or engine.
 *
 * - ten_protocol_asynced_t
 *   It supposes that the implementation protocol has its own runloop and runs
 *   in another thread.
 *
 * The relationship between those classes is as follows:
 *
 *                           ten_protocol_t
 *                                  ^
 *                                  |  <== inherits
 *                                  |
 *                          +-----------------+
 *                          |                 |
 *           ten_protocol_integrated_t     ten_protocol_asynced_t
 *                      ^                             ^
 *                      |  <== inherits               |  <== inherits
 *              +---------------+             +---------------+
 *              |               |             |               |
 *            impl             impl          impl            impl
 *        (ex: msgpack)                                 (ex: libws_http)
 */
typedef struct ten_protocol_t {
  ten_signature_t signature;

  /**
   * @note The base protocol and the implementation protocol may belongs
   * different threads. The base protocol's belonging thread should be same with
   * the related connection's. The implementation protocol may has its own
   * thread.
   */
  ten_sanitizer_thread_check_t thread_check;

  ten_ref_t ref;

  ten_addon_host_t *addon_host;

  // Start to trigger the closing of the base protocol.
  ten_atomic_t is_closing;

  // This field is used to mark that the base protocol is totally closed, it
  // means that all the resources bound to the base protocol has been closed.
  //
  // Now the only underlying resource of the base protocol is the implementation
  // protocol, so we do not use a field such as 'impl_is_closed' to store the
  // closed state of the implementation.
  bool is_closed;

  // Trigger binding resource to close, like connection/stream.
  ten_protocol_on_closed_func_t on_closed;
  void *on_closed_data;

  // This is the uri of this protocol represents.
  //   - For listening protocol, this is the local uri.
  //   - For communication protocol, this is the remote uri.
  ten_string_t uri;

  TEN_PROTOCOL_ROLE role;

  // Even though this field will be access from multi threads (e.g., the
  // implementation protocol thread), but it is immutable after the assignment
  // in the app (e.g., the listening protocol, and the communication protocol
  // when client accepted) or engine thread (e.g., the communication protocol
  // when connecting to the remote server).
  //
  // Note that if this field might be modified in multi threads, the
  // modifications to 'attach_to' and 'attached_target' must be done in one
  // atomic operation.
  TEN_PROTOCOL_ATTACH_TO attach_to;
  union {
    ten_app_t *app;  // The app where this protocol resides.
    ten_connection_t
        *connection;  // The connection where this protocol attached.
  } attached_target;

  // Used to react the closing request.
  ten_protocol_close_func_t close;

  // Used to react the listening request.
  ten_protocol_listen_func_t listen;

  // Used to react the connect_to request.
  ten_protocol_connect_to_func_t connect_to;

  // Used to react the migration to new runloop request.
  ten_protocol_migrate_func_t migrate;

  // Used to clean the resources bound to the old runloop.
  ten_protocol_clean_func_t clean;

  // TODO(Wei): Have an 'on_input' field here.

  // Used to handle the output TEN messages to the remote.
  ten_protocol_on_output_func_t on_output;

  // This is the callback function when this protocol is migrated to the new
  // runloop.
  ten_protocol_on_migrated_func_t on_migrated;

  // This is the callback function when all the resources bound to the old
  // runloop is cleaned.
  ten_protocol_on_cleaned_for_internal_func_t on_cleaned_for_internal;
  ten_protocol_on_cleaned_for_external_func_t on_cleaned_for_external;

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
   * 'ten_connection_t'), and the connection is paired with a remote (i.e.,
   * 'ten_remote_t') if a remote has been created by the engine. The life cycle
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
  ten_mutex_t *in_lock;
  ten_list_t in_msgs;
  // @}

  // @{
  // These fields is for storing the output data.
  ten_mutex_t *out_lock;
  ten_list_t out_msgs;
  // @}
} ten_protocol_t;

TEN_RUNTIME_PRIVATE_API bool ten_protocol_cascade_close_upward(
    ten_protocol_t *self);

TEN_RUNTIME_PRIVATE_API void ten_protocol_listen(
    ten_protocol_t *self, const char *uri,
    ten_protocol_on_client_accepted_func_t on_client_accepted);

TEN_RUNTIME_PRIVATE_API void ten_protocol_connect_to(
    ten_protocol_t *self, const char *uri,
    ten_protocol_on_server_connected_func_t on_server_connected);

TEN_RUNTIME_PRIVATE_API void ten_protocol_migrate(
    ten_protocol_t *self, ten_engine_t *engine, ten_connection_t *connection,
    ten_shared_ptr_t *cmd, ten_protocol_on_migrated_func_t on_migrated);

TEN_RUNTIME_PRIVATE_API void ten_protocol_clean(
    ten_protocol_t *self,
    ten_protocol_on_cleaned_for_internal_func_t on_cleaned_for_internal);

TEN_RUNTIME_PRIVATE_API void ten_protocol_update_belonging_thread_on_cleaned(
    ten_protocol_t *self);

TEN_RUNTIME_PRIVATE_API void ten_protocol_attach_to_connection(
    ten_protocol_t *self, ten_connection_t *connection);

/**
 * @brief Try to send one message to check if the connection needs to be
 * migrated when handling the first message, or just send one message after the
 * migration is completed.
 */
TEN_RUNTIME_PRIVATE_API void ten_protocol_on_input(ten_protocol_t *self,
                                                   ten_shared_ptr_t *msg);

/**
 * @brief Send messages in batch after the migration is completed.
 *
 * @note The caller side must be responsible to ensure that the migration has
 * been completed.
 */
TEN_RUNTIME_PRIVATE_API void ten_protocol_on_inputs(ten_protocol_t *self,
                                                    ten_list_t *msgs);

TEN_RUNTIME_PRIVATE_API ten_string_t *ten_protocol_uri_to_transport_uri(
    const char *uri);

TEN_RUNTIME_PRIVATE_API void ten_protocol_set_uri(ten_protocol_t *self,
                                                  ten_string_t *uri);

TEN_RUNTIME_PRIVATE_API void ten_protocol_set_addon(
    ten_protocol_t *self, ten_addon_host_t *addon_host);

TEN_RUNTIME_PRIVATE_API void ten_protocol_determine_default_property_value(
    ten_protocol_t *self);

TEN_RUNTIME_API TEN_PROTOCOL_ATTACH_TO
ten_protocol_attach_to(ten_protocol_t *self);
