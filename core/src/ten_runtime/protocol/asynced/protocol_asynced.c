//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/protocol/asynced/protocol_asynced.h"

#include "include_internal/ten_runtime/addon/addon.h"
#include "include_internal/ten_runtime/addon/protocol/protocol.h"
#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/app/migration.h"
#include "include_internal/ten_runtime/app/msg_interface/common.h"
#include "include_internal/ten_runtime/common/closeable.h"
#include "include_internal/ten_runtime/connection/connection.h"
#include "include_internal/ten_runtime/connection/migration.h"
#include "include_internal/ten_runtime/engine/engine.h"
#include "include_internal/ten_runtime/engine/internal/migration.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_runtime/protocol/asynced/internal.h"
#include "include_internal/ten_runtime/protocol/close.h"
#include "include_internal/ten_runtime/protocol/protocol.h"
#include "include_internal/ten_runtime/remote/remote.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_runtime/app/app.h"
#include "ten_runtime/protocol/close.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_node_ptr.h"
#include "ten_utils/container/list_ptr.h"
#include "ten_utils/io/runloop.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/ref.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"
#include "ten_utils/macro/memory.h"
#include "ten_utils/sanitizer/thread_check.h"

static void ten_protocol_asynced_migrate(ten_protocol_asynced_t *self,
                                         ten_engine_t *engine,
                                         ten_connection_t *connection,
                                         TEN_UNUSED ten_shared_ptr_t *cmd) {
  TEN_ASSERT(self && ten_protocol_check_integrity(&self->base, true),
             "Should not happen.");
  TEN_ASSERT(engine->app && ten_app_check_integrity(engine->app, true),
             "The function is called in the app thread, and will migrate the "
             "protocol to the protocol thread.");

  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: This function will be called when starting to migrate the
  // connection from the TEN app to the TEN engine. So the
  // 'ten_engine_on_connection_cleaned_async()' function uses the async tasks to
  // ensure the thread safety.
  TEN_ASSERT(engine && ten_engine_check_integrity(engine, false),
             "Should not happen.");
  TEN_ASSERT(connection && ten_connection_check_integrity(connection, true),
             "Should not happen.");

  // We are in the app thread now, so we can call 'ten_app_clean_connection()'
  // here directly.
  ten_app_clean_connection(engine->app, connection);

  // We need to switch to the engine thread to do some operations which need to
  // be happened in the engine thread.
  ten_engine_on_connection_cleaned_async(engine, connection, cmd);
}

static void ten_protocol_asynced_on_input(void *self_, void *arg) {
  ten_protocol_asynced_t *self = (ten_protocol_asynced_t *)self_;
  TEN_ASSERT(self && ten_protocol_check_integrity(&self->base, true),
             "Should not happen.");

  ten_shared_ptr_t *msg = (ten_shared_ptr_t *)arg;
  TEN_ASSERT(msg && ten_msg_check_integrity(msg), "Invalid argument.");

  if (!ten_protocol_is_closing(&self->base)) {
    ten_protocol_on_input(&self->base, msg);
  }

  ten_shared_ptr_destroy(msg);

  // The task is completed, so delete a reference to the 'protocol' to reflect
  // this fact.
  ten_ref_dec_ref(&self->base.ref);
}

ten_protocol_asynced_creation_info_t *ten_protocol_asynced_creation_info_create(
    ten_protocol_asynced_on_created_func_t on_created, void *user_data) {
  TEN_ASSERT(on_created, "Invalid argument.");

  ten_protocol_asynced_creation_info_t *self =
      (ten_protocol_asynced_creation_info_t *)TEN_MALLOC(
          sizeof(ten_protocol_asynced_creation_info_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  self->on_created = on_created;
  self->user_data = user_data;

  return self;
}

void ten_protocol_asynced_creation_info_destroy(
    ten_protocol_asynced_creation_info_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  TEN_FREE(self);
}

static void ten_protocol_asynced_on_base_protocol_cleaned_task(
    ten_protocol_asynced_t *self, void *arg) {
  TEN_ASSERT(self, "Invalid argument.");

  ten_protocol_t *base_protocol = &self->base;
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: This function must be called in the implementation protocol
  // thread after the migration is completed, as the
  // 'ten_protocol_asynced_t::migration_state' field must be accessed from the
  // implementation protocol thread. So do not check thread integrity of the
  // base protocol.
  TEN_ASSERT(ten_protocol_check_integrity(base_protocol, false),
             "Invalid argument.");

  bool is_migration_state_reset = arg;

  if (is_migration_state_reset) {
    TEN_LOGD("The connection migration is reset.");
    self->migration_state = TEN_CONNECTION_MIGRATION_STATE_INIT;
  } else {
    TEN_LOGD("The connection migration is completed.");
    self->migration_state = TEN_CONNECTION_MIGRATION_STATE_DONE;
  }

  ten_list_foreach (&self->pending_task_queue, iter) {
    ten_protocol_asynced_task_t *task = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(task && task->handler, "Should not happen.");

    task->handler(self, task->arg);
  }

  ten_list_clear(&self->pending_task_queue);

  ten_ref_dec_ref(&base_protocol->ref);
}

static void ten_protocol_asynced_on_base_protocol_cleaned(
    ten_protocol_asynced_t *self, bool is_migration_state_reset) {
  TEN_ASSERT(self, "Invalid argument.");

  ten_protocol_t *base_protocol = &self->base;
  TEN_ASSERT(ten_protocol_check_integrity(base_protocol, true),
             "This function is always called in the engine thread when the "
             "migration in the TEN world is completed.");

  ten_sanitizer_thread_check_set_belonging_thread_to_current_thread(
      &self->closeable.thread_check);

  // The below 'trigger_impl()' function will post a runloop task, so increase
  // the reference count of the base protocol here.
  ten_ref_inc_ref(&base_protocol->ref);

  self->post_task_to_impl(self,
                          ten_protocol_asynced_on_base_protocol_cleaned_task,
                          (void *)is_migration_state_reset);  // NOLINT
}

void ten_protocol_asynced_init(
    ten_protocol_asynced_t *self, const char *name,
    ten_addon_host_t *addon_host, ten_protocol_on_output_func_t on_output,
    ten_protocol_listen_func_t listen,
    ten_protocol_connect_to_func_t connect_to,
    ten_protocol_asynced_post_task_to_impl_func_t post_task_to_impl) {
  TEN_ASSERT(self && name && addon_host, "Should not happen.");
  TEN_ASSERT(post_task_to_impl,
             "The 'trigger_impl' could not be NULL, it's used to notify the "
             "implementation protocol from the TEN world.");

  // The 'ten_protocol_asynced_t::closeable' is an underlying resource of
  // 'ten_protocol_t::closeable', so the 'ten_protocol_asynced_t' does _not_
  // need to register the 'ten_protocol_t::close' callback.
  ten_protocol_init(&self->base, name, NULL, on_output, listen, connect_to,
                    (ten_protocol_migrate_func_t)ten_protocol_asynced_migrate,
                    /* clean */ NULL);

  ten_protocol_asynced_init_closeable(self);

  // Note that the value of 'ten_protocol_asynced_t::migration_state' is only
  // _meaningful_ in the flow of the input messages handling (i.e., when the
  // implementation protocol receives messages from the client side), in other
  // words, we only need to consider the migration flow only if there is any
  // input messages coming from this protocol, therefore, in the very beginning,
  // we set the default value to 'DONE', but not 'INIT' here.
  //
  // When there is any client connects to this protocol, it implies that there
  // will be some input messages coming from this protocol, so we will set
  // 'ten_protocol_asynced_t::migration_state' to INIT in
  // 'ten_protocol_asynced_on_client_accepted()'.
  self->migration_state = TEN_CONNECTION_MIGRATION_STATE_DONE;

  ten_list_init(&self->pending_task_queue);

  self->post_task_to_impl = post_task_to_impl;

  self->base.on_cleaned_for_external =
      (ten_protocol_on_cleaned_for_external_func_t)
          ten_protocol_asynced_on_base_protocol_cleaned;
}

void ten_protocol_asynced_deinit(ten_protocol_asynced_t *self) {
  TEN_ASSERT(self &&
                 // TEN_NOLINTNEXTLINE(thread-check)
                 // thread-check: The belonging thread of the 'protocol' is
                 // ended when this function is called, so we can not check
                 // thread integrity here.
                 ten_protocol_check_integrity(&self->base, false),
             "Should not happen.");

  TEN_ASSERT(ten_list_is_empty(&self->pending_task_queue),
             "The pending tasks should be processed before the protocol is "
             "destroyed.");

  ten_closeable_deinit(&self->closeable);
  ten_protocol_deinit(&self->base);
}

/**
 * @brief Handles the cached messages once the connection migration is completed
 * or reset.
 *
 * @note Only the first message will be transferred to the TEN world before
 * the migration is completed. And it's safe to retrieve the attached runloop of
 * the base protocol (i.e., the base class of ten_protocol_asynced_t --
 * ten_protocol_t) only if the 'migration_state' is INIT (i.e., no message has
 * been received yet) or DONE (i.e., the migration has been completed),
 * otherwise, the messages should be cached until the migration is completed.
 *
 * The reason why do not use the 'ten_protocol_t::in_msgs' queue to cache
 * those messages is that the 'ten_protocol_t::in_msgs' queue will be accessed
 * both in the TEN world and the implementation protocol, in other words, the
 * reading and writing of 'ten_protocol_t::in_msgs' _must_ be protected by
 * 'ten_protocol_t::in_lock'. However, the queue for cached messages will be
 * only accessed in the implementation protocol world, no mutex lock is
 * needed. So it's better to use a separate queue for cached messages instead.
 */
static void ten_protocol_asynced_on_input_task(ten_protocol_asynced_t *self,
                                               void *arg) {
  TEN_ASSERT(self && arg, "Invalid argument.");

  ten_protocol_t *base_protocol = &self->base;

  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is called from the implementation protocol
  // thread to handle the pending task once the migration is completed or reset,
  // so do not check the thread integrity of the base protocol.
  TEN_ASSERT(ten_protocol_check_integrity(base_protocol, false),
             "Invalid argument.");

  ten_shared_ptr_t *msg = (ten_shared_ptr_t *)arg;

  ten_protocol_asynced_on_input_async(self, msg);
  ten_shared_ptr_destroy(msg);
}

/**
 * This function is called from the implementation protocol thread when it
 * receives a message from the client side, and it wants to transfer the message
 * to the TEN world through the runloop of the related connection (i.e., the
 * 'ten_connection_t' object). The brief flow is as follows:
 *
 * - When the first message comes, i.e., the
 *   'ten_protocol_asynced_t::migration_state' is 'INIT' now, the
 *   'ten_connection_t::migration_state' must be 'INIT' too, as the connection
 *   migration will be started only if the connection handles messages. So it's
 *   safe to retrieve the runloop of the related connection in this stage. And
 *   only one message could be transferred to the TEN world as the connection
 *   migration is always asynchronous and the migration must _not_ be executed
 *   twice. So 'ten_protocol_asynced_t::migration_state' will be set to
 *   'FIRST_MSG' to ensure that the connection only handle one message before
 *   the migration is completed.
 *
 * - Before the migration is completed, i.e., the
 *   'ten_protocol_asynced_t::migration_state' is not 'DONE', all the subsequent
 *   messages should be cached, and the closing flow of the asynced protocol
 *   should be frozen.
 *
 * - Once the migration is completed in the TEN world, the implementation
 *   protocol thread will receive an event, and the
 *   'ten_protocol_asynced_on_base_protocol_cleaned_task()' function will be
 *   executed. Then the 'ten_protocol_asynced_t::migration_state' will be set to
 *   'DONE', and it's time to handle the pending closing flow and messages.
 *
 * - The subsequent messages will be transferred to the TEN world directly as
 *   the migration is completed, the runloop of the related connection will be
 *   correct.
 *
 */
bool ten_protocol_asynced_on_input_async(ten_protocol_asynced_t *self,
                                         ten_shared_ptr_t *msg) {
  TEN_ASSERT(self && msg, "Should not happen.");

  ten_protocol_t *base_protocol = &self->base;
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: By design, this function is used to send messages to the
  // runtime from the implemented protocol. And the implemented protocol has its
  // own thread.
  TEN_ASSERT(ten_protocol_check_integrity(base_protocol, false),
             "Should not happen.");

  // The connection protocol is created in the implemented thread, so we can
  // access its fields directly here. Refer to
  // 'ten_protocol_asynced_on_client_accepted()'.
  //
  // Note that the 'ten_protocol_t::role' will be updated when the engine or app
  // handles the message, so we can _not_ read it here. Refer to
  // 'ten_connection_handle_command_from_external_client()'.
  TEN_ASSERT(ten_protocol_attach_to(base_protocol) ==
                 TEN_PROTOCOL_ATTACH_TO_CONNECTION,
             "Should not happen.");

  if (ten_protocol_is_closing(&self->base)) {
    TEN_LOGD(
        "Protocol asynced[%p] is closing, could not handle messages any more.",
        self);
    return false;
  }

  msg = ten_shared_ptr_clone(msg);
  ten_protocol_asynced_post_task_to_ten(self,
                                        ten_protocol_asynced_on_input_task,
                                        ten_protocol_asynced_on_input, msg);

  if (self->migration_state == TEN_CONNECTION_MIGRATION_STATE_INIT) {
    // Only transfer one message to the TEN world before the migration is
    // completed, as the protocol side could not know whether the connection
    // needs to be migrated or not. And the migration is always asynchronous, so
    // transfer only one message to determine whether the connection needs to be
    // migrated.
    self->migration_state = TEN_CONNECTION_MIGRATION_STATE_FIRST_MSG;
  }

  return true;
}

static void ten_protocol_asynced_on_connected(void *self_, void *arg) {
  ten_protocol_asynced_t *protocol = (ten_protocol_asynced_t *)self_;
  TEN_ASSERT(protocol && ten_protocol_check_integrity(&protocol->base, true),
             "Should not happen.");

  if (protocol->base.on_server_connected) {
    protocol->base.on_server_connected(&protocol->base, (bool)arg);
  }

  // The task is completed, so delete a reference to the 'protocol' to reflect
  // this fact.
  ten_ref_dec_ref(&protocol->base.ref);
}

void ten_protocol_asynced_on_connected_async(ten_protocol_asynced_t *self,
                                             bool is_connected) {
  TEN_ASSERT(self, "Invalid argument.");

  ten_protocol_t *base_protocol = &self->base;

  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in
  // different threads.
  TEN_ASSERT(ten_protocol_check_integrity(base_protocol, false),
             "Should not happen.");

  // TODO(Wei): Does the access to the 'is_closed' field is safe among different
  // threads?
  TEN_ASSERT(!base_protocol->is_closed,
             "The protocol could not connect to remote after it is closed.");

  // This function is called from the 'external protocol' thread, when the
  // 'connect_to' request is processed. But the TEN protocol maybe closing when
  // this function is called, because the TEN engine is closing. Even if the TEN
  // protocol is closing, the following 'on_connected' task still has to be
  // submitted, otherwise, there will be memory leaks, because the 'connect_to'
  // cmd can not be destroyed (this action will be triggered in
  // 'ten_protocol_asynced_on_connected').
  //
  // And it's fine to execute the 'on_connected' task, because the asynced
  // protocol has not been closed, the TEN protocol and remote won't be closed.

  // Before posting a runloop task, we have to add a reference to the
  // 'protocol', so that it will not be destroyed _before_ the runloop task is
  // executed.
  ten_ref_inc_ref(&base_protocol->ref);

  ten_runloop_t *loop = ten_protocol_get_attached_runloop(base_protocol);
  TEN_ASSERT(loop,
             "The connection migration is completed by default in 'connect_to' "
             "scenario, so the runloop could not be NULL.");

  ten_runloop_post_task_tail(loop, ten_protocol_asynced_on_connected, self,
                             (void *)is_connected);  // NOLINT
}

static void ten_app_thread_on_client_protocol_created(ten_env_t *ten_env,
                                                      ten_protocol_t *instance,
                                                      void *cb_data) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_app_t *app = ten_env_get_attached_app(ten_env);
  TEN_ASSERT(app && ten_app_check_integrity(app, true), "Should not happen.");

  ten_protocol_asynced_creation_info_t *info =
      (ten_protocol_asynced_creation_info_t *)cb_data;
  TEN_ASSERT(info, "Invalid argument.");

  ten_protocol_asynced_t *listening_protocol = info->user_data;
  TEN_ASSERT(listening_protocol &&
                 ten_protocol_check_integrity(&listening_protocol->base, true),
             "Invalid argument.");

  ten_protocol_asynced_t *protocol = (ten_protocol_asynced_t *)instance;
  if (!protocol) {
    TEN_LOGE("Failed to create the protocol instance.");
    info->on_created(NULL, info);
    return;
  }

  // Those implementation protocols are used for handling the messages from
  // the client side, and the related connection (i.e., the 'ten_connection_t'
  // object) might need to be migrated, so set the value to 'INIT' as the
  // default value is 'DONE'. Refer to 'ten_protocol_asynced_init()'.
  protocol->migration_state = TEN_CONNECTION_MIGRATION_STATE_INIT;

  ten_protocol_attach_to_app_and_thread(&protocol->base, app);

  if (listening_protocol->base.on_client_accepted) {
    listening_protocol->base.on_client_accepted(&listening_protocol->base,
                                                &protocol->base);
  }

  info->on_created(protocol, info);
}

static void ten_protocol_asynced_on_client_accepted(void *self, void *info_) {
  ten_protocol_asynced_t *listening_protocol = (ten_protocol_asynced_t *)self;
  TEN_ASSERT(listening_protocol, "Should not happen.");

  ten_protocol_asynced_creation_info_t *info =
      (ten_protocol_asynced_creation_info_t *)info_;
  TEN_ASSERT(info, "Invalid argument.");

  if (ten_protocol_is_closing(&listening_protocol->base)) {
    info->on_created(NULL, info);
  } else {
    ten_protocol_t *listening_base_protocol = &listening_protocol->base;
    TEN_ASSERT(listening_base_protocol &&
                   ten_protocol_check_integrity(listening_base_protocol, true),
               "Invalid argument.");
    TEN_ASSERT(listening_base_protocol->role == TEN_PROTOCOL_ROLE_LISTEN &&
                   ten_protocol_attach_to(listening_base_protocol) ==
                       TEN_PROTOCOL_ATTACH_TO_APP,
               "Should not happen.");

    ten_addon_host_t *addon_host = listening_base_protocol->addon_host;
    TEN_ASSERT(addon_host && ten_addon_host_check_integrity(addon_host) &&
                   addon_host->type == TEN_ADDON_TYPE_PROTOCOL,
               "Should not happen.");

    ten_app_t *app = listening_base_protocol->attached_target.app;
    TEN_ASSERT(app && ten_app_check_integrity(app, true), "Should not happen.");

    ten_error_t err;
    ten_error_init(&err);

    // We can _not_ know whether the protocol role is
    // 'TEN_PROTOCOL_ROLE_IN_INTERNAL' or 'TEN_PROTOCOL_ROLE_IN_EXTERNAL'
    // until the message received from the protocol is processed. Refer to
    // 'ten_connection_on_msgs()' and
    // 'ten_connection_handle_command_from_external_client()'.
    bool rc = ten_addon_create_protocol(
        app->ten_env, ten_string_get_raw_str(&addon_host->name),
        ten_string_get_raw_str(&addon_host->name), TEN_PROTOCOL_ROLE_IN_DEFAULT,
        ten_app_thread_on_client_protocol_created, info, NULL);
    TEN_ASSERT(rc, "Failed to create protocol, err: %s",
               ten_error_errmsg(&err));

    ten_error_deinit(&err);
  }

  // The task is completed, so delete a reference to the 'protocol' to reflect
  // this fact.
  ten_ref_dec_ref(&listening_protocol->base.ref);
}

bool ten_protocol_asynced_on_client_accepted_async(
    ten_protocol_asynced_t *listening_protocol,
    ten_protocol_asynced_creation_info_t *info) {
  TEN_ASSERT(listening_protocol, "Should not happen.");

  if (ten_protocol_is_closing(&listening_protocol->base)) {
    TEN_LOGD(
        "Protocol asynced[%p] is closing, could not receive client request.",
        listening_protocol);
    return false;
  }

  // Before posting a runloop task, we have to add a reference to the
  // 'protocol', so that it will not be destroyed _before_ the runloop task is
  // executed.
  ten_ref_inc_ref(&listening_protocol->base.ref);

  // TODO(xilin): Replace pushing a task to the runloop with wrapping it into
  // 'ten_env' apis.

  ten_runloop_t *loop =
      ten_protocol_get_attached_runloop(&listening_protocol->base);
  TEN_ASSERT(loop,
             "The attached runloop of the listen protocol is always the app's, "
             "it could not be NULL.");

  ten_runloop_post_task_tail(loop, ten_protocol_asynced_on_client_accepted,
                             listening_protocol, info);

  return true;
}

const char *ten_protocol_asynced_get_name(ten_protocol_asynced_t *self) {
  TEN_ASSERT(self && ten_protocol_check_integrity(&self->base, true),
             "Access across threads.");

  ten_addon_host_t *addon_host = self->base.addon_host;
  TEN_ASSERT(addon_host && ten_addon_host_check_integrity(addon_host),
             "Invalid argument.");

  ten_value_t *item = ten_value_object_peek(&addon_host->manifest, "name");
  TEN_ASSERT(item, "Failed to get protocol name from its manifest.");

  const char *name = ten_value_peek_raw_str(item);
  return name;
}

bool ten_protocol_asynced_safe_to_retrieve_runtime_runloop(
    ten_protocol_asynced_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is always called in the implementation protocol
  // thread.
  TEN_ASSERT(ten_protocol_check_integrity(&self->base, false),
             "Invalid argument.");

  return self->migration_state == TEN_CONNECTION_MIGRATION_STATE_INIT ||
         self->migration_state == TEN_CONNECTION_MIGRATION_STATE_DONE;
}

static ten_protocol_asynced_task_t *ten_protocol_asynced_task_create(
    ten_protocol_asynced_task_handler_func_t handler, void *arg) {
  ten_protocol_asynced_task_t *self =
      TEN_MALLOC(sizeof(ten_protocol_asynced_task_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  self->handler = handler;
  self->arg = arg;

  return self;
}

static void ten_protocol_asynced_task_destroy(
    ten_protocol_asynced_task_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  self->handler = NULL;
  self->arg = NULL;

  TEN_FREE(self);
}

void ten_protocol_asynced_post_task_to_ten(
    ten_protocol_asynced_t *self,
    ten_protocol_asynced_task_handler_func_t handler_if_in_migration,
    void (*runloop_task_handler)(void *, void *), void *arg) {
  TEN_ASSERT(self && runloop_task_handler, "Invalid argument.");

  ten_protocol_t *base_protocol = &self->base;

  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is always called in the implementation protocol
  // thread. All fields of the 'ten_protocol_asynced_t' accessed here are only
  // read/written in the implementation protocol thread.
  TEN_ASSERT(ten_protocol_check_integrity(base_protocol, false),
             "Invalid argument.");

  if (ten_protocol_asynced_safe_to_retrieve_runtime_runloop(self)) {
    ten_runloop_t *loop = ten_protocol_get_attached_runloop(base_protocol);
    TEN_ASSERT(loop, "Invalid argument.");

    ten_ref_inc_ref(&base_protocol->ref);

    ten_runloop_post_task_tail(loop, runloop_task_handler, self, arg);
  } else {
    if (handler_if_in_migration) {
      // Create a pending task in the implementation protocol, and will continue
      // to handle it in the implementation protocol when the migration is
      // completed.
      ten_protocol_asynced_task_t *pending_task =
          ten_protocol_asynced_task_create(handler_if_in_migration, arg);
      ten_list_push_ptr_back(
          &self->pending_task_queue, pending_task,
          (ten_ptr_listnode_destroy_func_t)ten_protocol_asynced_task_destroy);
    } else {
      TEN_ASSERT(0,
                 "The 'handler_if_in_migration' is required if the connection "
                 "is in migration.");
    }
  }
}
