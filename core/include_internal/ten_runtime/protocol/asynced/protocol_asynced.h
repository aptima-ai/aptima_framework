//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "include_internal/ten_runtime/common/closeable.h"
#include "include_internal/ten_runtime/connection/connection.h"
#include "include_internal/ten_runtime/protocol/protocol.h"

typedef struct ten_protocol_asynced_t ten_protocol_asynced_t;
typedef struct ten_protocol_asynced_creation_info_t
    ten_protocol_asynced_creation_info_t;

/**
 * @brief Used to trigger the implementation protocol to do something in its
 * thread from the TEN world.
 *
 * @param cb The callback function to be called in the implementation protocol
 * thread later.
 *
 * @param arg The extra argument to be passed to the callback function.
 *
 * @note The implementation protocol should call the callback function in its
 * own thread. Note that increasing the reference count of 'self' and 'arg'
 * before calling this function to ensure those objects are valid when the
 * callback function is called.
 */
typedef void (*ten_protocol_asynced_post_task_to_impl_func_t)(
    ten_protocol_asynced_t *self,
    void (*cb)(ten_protocol_asynced_t *, void *arg), void *arg);

typedef void (*ten_protocol_asynced_on_created_func_t)(
    ten_protocol_asynced_t *protocol,
    ten_protocol_asynced_creation_info_t *info);

typedef struct ten_protocol_asynced_creation_info_t {
  ten_protocol_asynced_on_created_func_t on_created;
  void *user_data;
} ten_protocol_asynced_creation_info_t;

/**
 * @brief As the implementation protocol might have its own runloop, which means
 * that the implementation protocol and ten base protocol (i.e., ten_protocol_t)
 * belongs to different threads. The messages between the implementation
 * protocol and base protocol can not be exchanged directly (i.e., a function
 * should always be called in the same thread). So we provide the
 * 'ten_protocol_asynced_t', which is a bridge between the implementation and
 * base protocols, and the 'ten_protocol_asynced_t' will care about the thread
 * safety of the message exchanging between those two sides. A simple
 * demonstration is as follows.
 *
 * 1) Messages from the implementation protocol to the base protocol:
 *
 * | implementation |  -- sync call --> | ten_protocol_asynced_t |  ---+
 *                                                                     |
 * | base protocol | <-- sync call -- [runloop of base protocol] <-----+
 *
 * 2) Messages from the base protocol to the implementation protocol:
 *
 * | base protocol |  -- sync call --> | ten_protocol_asynced_t |  ----+
 *                                                                     |
 * | implementation | <-- sync call -- [runloop of implementation] <---+
 *
 * Basically, the ten_protocol_asynced_t will hold the runloop of both the
 * implementation protocol and the base protocol.
 *
 * - The runloop of the base protocol will be retrieved by
 *   'ten_protocol_get_attached_runloop(ten_protocol_asynced_t::base)'.
 *
 * - The 'ten_protocol_asynced_t::trigger_impl' is a standard interface that the
 *   implementation must implement, which will be used to submit tasks to the
 *   runloop of the implementation protocol.
 *
 * The apis called from the implementation protocol on the
 * ten_protocol_asynced_t, or the callbacks registered from the implementation
 * protocol in the ten_protocol_asynced_t, are always called synchronously in
 * the implementation thread. In the meanwhile, the apis in the base protocol
 * are always called synchronously in the ten app or engine thread.
 *
 * @note As the ten_protocol_asynced_t is a bridge between two different
 * threads, some fields will be accessed from the implementation thread, and
 * others will be accessed from the ten world. Details are as follows.
 *
 * - The 'base', 'closeable', 'trigger_impl' are accessed fields from the ten
 *   world.
 *
 * - Others are accessed in the implementation thread.
 *
 * Please keep in mind that, _NO_ fields are read or written in both sides.
 */
typedef struct ten_protocol_asynced_t {
  ten_protocol_t base;

  /**
   * The ten_protocol_asynced_t is an underlying resource of the base protocol.
   *
   * @note All protocol instances are created and initted in the ten world,
   * including this 'ten_protocol_asynced_t'. So this 'closeable' belongs to the
   * ten world, and the underlying resource (ex: this closeable) and its owner
   * (ex: the base protocol) must be in the same thread.
   */
  ten_closeable_t closeable;

  /**
   * @brief The closeable reference of the implementation belongs to the
   * external thread. As the 'ten_protocol_asynced_t::closeable' and
   * 'impl_closeable' belongs to different threads, the 'impl_closeable' could
   * not be an underlying resource of 'ten_protocol_asynced_t::closeable' by
   * calling 'ten_closeable_add_underlying_resource()'. The correct way to close
   * the implementation protocol is that register a 'intend_to_close' hook in
   * 'ten_protocol_asynced_t::closeable', and switch to the implementation
   * protocol thread to close 'impl_closeable' in the 'intend_to_close' hook.
   *
   * @note Do _NOT_ access this field in the ten world.
   */
  ten_closeable_t *impl_closeable;

  /**
   * According to the comments of 'ten_connection_t::migration_state', there
   * might be race conditions if the asynced protocol reads/writes the
   * 'ten_connection_t::migration_state' in the implementation protocol thread.
   *
   * First, the 'ten_connection_t::migration_state' is assigned to 'INIT' in the
   * app thread when the connection (i.e., the 'ten_connection_t' object) is
   * created.
   *
   * Then the implementation protocol will retrieve the value of
   * 'ten_connection_t::migration_state' in its thread when handling the first
   * message, the value will be 'INIT'. The implementation protocol will
   * retrieve the correct value because it access the memory of
   * 'ten_connection_t' after the libws runloop task
   * (ten_libws_worker_on_protocol_created_task). In other words, the reading of
   * 'ten_connection_t::migration_state' is after the writing of
   * 'ten_connection_t::migration_state'. Refer to
   * 'ten_libws_server_on_protocol_created_async()'.
   *
   * But when the migration is completed in the engine thread, there might be
   * race condition when the implementation protocol reads the value. Ex:
   *
   *      | Engine thread           | Implementation protocol thread |
   *      |-------------------------|--------------------------------|
   *   t1 |                         | read state                     |
   *      |-------------------------|--------------------------------|
   *   t2 | write state             |                                |
   *      |-------------------------|--------------------------------|
   *   t3 | acquire in_lock         |                                |
   *      | pop into in_msgs queue  |                                |
   *      |-------------------------|--------------------------------|
   *   t4 |                         | acquire in_lock                |
   *      |                         | push from in_msgs queue        |
   *      |-------------------------|--------------------------------|
   *
   * As the 'write state' and 'read state' operation is not protected by
   * 'in_lock', the above 'write state' and 'acquire in_lock' in the engine
   * thread operations are not atomic, neither are the 'read state' and 'acquire
   * in_lock' in the implementation protocol thread. So the engine thread might
   * acquire the 'in_lock' before the implementation protocol thread (i.e., t3 <
   * t4). In this case, the engine thread could not retrieve the pending
   * messages from the 'in_msgs' list, as the implementation protocol thread has
   * not put the messages into the list yet. And it's too heavy to use the
   * 'in_lock' to protect the 'migration_state'.
   *
   * So, keep a mirror of 'ten_connection_t::migration_state' in the asynced
   * protocol, and obey the following rules.
   *
   * - The 'ten_connection_t::migration_state' will only be accessed in the TEN
   *   world.
   *
   * - The 'ten_protocol_asynced_t::migration_state' will only be accessed in
   *   the implementation protocol thread.
   *
   * - When the migration is completed in the engine thread, or reset in the app
   *   thread (ex, no engine was found), the 'ten_connection_t::migration_state'
   *   will be updated to 'DONE' or 'INIT'. And then
   *   'ten_protocol_asynced_t::migration_state' will be synced through the
   *   runloop task (i.e., the 'ten_protocol_t::on_cleaned_for_external()'
   *   callback) to ensure that the implementation protocol could retrieve the
   *   correct runloop of the connection.
   *
   * And there is no need to use any mutex lock to protect the
   * 'migration_state' in both sides.
   *
   * Refer 'ten_protocol_asynced_on_input_async()' to know about how the
   * 'migrate_state' will be changed.
   */
  TEN_CONNECTION_MIGRATION_STATE migration_state;

  /**
   * @brief According to the design principle, the implementation protocol only
   * care about its resources such as the physical connections. The life cycle
   * (ex: closing and destroying) of the protocol objects (including the
   * 'ten_protocol_t' and the corresponding implementation protocol) could only
   * be managed by the TEN runtime. In other words, if the physical connection
   * is broken, the implementation protocol should not close itself, instead, it
   * should send an event to the TEN runtime, and the protocol (i.e., the
   * 'ten_protocol_t' object) will be closed from the ten world if needed. So
   * there might be some messages/events from the implementation protocol that
   * might not be able to be transmitted to their desired destination due to
   * the following reasons:
   *
   * - The physical connection is broken in the implementation protocol, but at
   *   the same time, the corresponding 'ten_connection_t' object is in the
   *   migration. It's not safe to retrieve the runloop of the
   *   'ten_connection_t'.
   *
   * - The implementation protocol receives messages from the client side, but
   *   at the same time, the corresponding 'ten_connection_t' object is in the
   *   migration.
   *
   * The 'pending_task_queue' is used to cache those messages/events.
   *
   * @note This queue _must_ be read/written in the external protocol thread,
   * this queue will _not_ be protected by any mutex lock.
   */
  ten_list_t pending_task_queue;  // ten_protocol_asynced_task_t

  ten_protocol_asynced_post_task_to_impl_func_t post_task_to_impl;
} ten_protocol_asynced_t;

TEN_RUNTIME_API ten_protocol_asynced_creation_info_t *
ten_protocol_asynced_creation_info_create(
    ten_protocol_asynced_on_created_func_t on_created, void *user_data);

TEN_RUNTIME_API void ten_protocol_asynced_creation_info_destroy(
    ten_protocol_asynced_creation_info_t *self);

TEN_RUNTIME_API void ten_protocol_asynced_init(
    ten_protocol_asynced_t *self, const char *name,
    ten_addon_host_t *addon_host, ten_protocol_on_output_func_t on_output,
    ten_protocol_listen_func_t on_listen,
    ten_protocol_connect_to_func_t on_connect_to,
    ten_protocol_asynced_post_task_to_impl_func_t post_task_to_impl);

TEN_RUNTIME_API void ten_protocol_asynced_deinit(ten_protocol_asynced_t *self);

/**
 * @brief Call this function when the protocol receives a TEN message, and want
 * to send that message into the TEN world. The protocol can be a server
 * (listening protocol) or a client (communication protocol). The message may be
 * a request received by a server, or the result received by a client.
 */
TEN_RUNTIME_API bool ten_protocol_asynced_on_input_async(
    ten_protocol_asynced_t *self, ten_shared_ptr_t *msg);

/**
 * @brief The protocol acts as a client, call this function after connecting to
 * server or disconnecting from server.
 * @param is_connected true if the connection has been established, otherwise
 * false.
 */
TEN_RUNTIME_API void ten_protocol_asynced_on_connected_async(
    ten_protocol_asynced_t *self, bool is_connected);

/**
 * @brief Create a new protocol when a client request is accepted.
 *
 * @return Whether the 'protocol creating' task is submitted to the TEN app
 * runloop.
 */
TEN_RUNTIME_API bool ten_protocol_asynced_on_client_accepted_async(
    ten_protocol_asynced_t *listening_protocol,
    ten_protocol_asynced_creation_info_t *info);

/**
 * @brief Get the protocol name from its manifest.
 */
TEN_RUNTIME_API const char *ten_protocol_asynced_get_name(
    ten_protocol_asynced_t *self);

TEN_RUNTIME_API void ten_protocol_asynced_set_impl_closeable(
    ten_protocol_asynced_t *self, ten_closeable_t *impl);

/**
 * @brief The implementation protocol has been closed from its thread, then
 * switch to the ten world to continue to close the base protocol.
 */
TEN_RUNTIME_API void ten_protocol_asynced_on_impl_closed_async(
    ten_protocol_asynced_t *self);

/**
 * @brief The closeable of the implementation protocol (i.e., @a impl) could not
 * be the directly underlying resource of the ten_protocol_asynced_t::closeable,
 * as they belong to different threads. So we need to set the necessary hooks in
 * those two closeable objects to ensure that the behaviors will be correct. Ex:
 * the 'intend_to_close', 'is_closing_root' behaviors. In some cases, the
 * relevant ten_protocol_asynced_t of the implementation protocol is not created
 * in time (ex: the libws worker), the implementation protocol could not call
 * the above 'ten_protocol_asynced_set_impl_closeable()' once it is created. So
 * we provide this function for the implementation protocol to set the default
 * behaviors once it is created.
 */
TEN_RUNTIME_PRIVATE_API void
ten_protocol_asynced_set_default_closeable_behavior(ten_closeable_t *impl);
