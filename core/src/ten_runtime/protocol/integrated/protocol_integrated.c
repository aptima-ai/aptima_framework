//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/protocol/integrated/protocol_integrated.h"

#include "include_internal/ten_runtime/addon/addon.h"
#include "include_internal/ten_runtime/addon/protocol/protocol.h"
#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/app/migration.h"
#include "include_internal/ten_runtime/common/closeable.h"
#include "include_internal/ten_runtime/connection/connection.h"
#include "include_internal/ten_runtime/connection/migration.h"
#include "include_internal/ten_runtime/engine/engine.h"
#include "include_internal/ten_runtime/engine/internal/migration.h"
#include "include_internal/ten_runtime/engine/internal/thread.h"
#include "include_internal/ten_runtime/engine/msg_interface/common.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_runtime/protocol/close.h"
#include "include_internal/ten_runtime/protocol/integrated/close.h"
#include "include_internal/ten_runtime/protocol/protocol.h"
#include "include_internal/ten_runtime/remote/remote.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "include_internal/ten_utils/log/log.h"
#include "ten_runtime/protocol/close.h"
#include "ten_utils/container/list.h"
#include "ten_utils/io/runloop.h"
#include "ten_utils/io/stream.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/mutex.h"
#include "ten_utils/lib/ref.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

static void ten_protocol_close_task(void *self_, TEN_UNUSED void *arg) {
  ten_protocol_t *self = (ten_protocol_t *)self_;
  TEN_ASSERT(self && ten_protocol_check_integrity(self, true),
             "Access across threads.");

  ten_protocol_close(self);

  ten_ref_dec_ref(&self->ref);
}

static void ten_protocol_on_inputs_based_on_migration_state(
    ten_protocol_t *self, ten_list_t *msgs) {
  TEN_ASSERT(self && ten_protocol_check_integrity(self, true),
             "Should not happen.");
  TEN_ASSERT(msgs, "Should not happen.");
  TEN_ASSERT(ten_protocol_attach_to(self) == TEN_PROTOCOL_ATTACH_TO_CONNECTION,
             "Should not happen.");

  ten_connection_t *connection = self->attached_target.connection;
  TEN_ASSERT(connection && ten_connection_check_integrity(connection, true),
             "Should not happen.");

  // The stream will be frozen before the migration, and this function is only
  // called when the integrated protocol retrieves data from the stream. So the
  // 'ten_connection_t::migration_state' here is only read or written in one
  // thread at any time.
  //
  // - Before the migration, this function is called in the app thread, the
  //   'ten_connection_t::migration_state' will be read and written in the app
  //   thread.
  //
  // - No messages during the migration, which means this function will not be
  //   called once the migration is started.
  //
  // - When the migration is completed, this function is always called in the
  //   engine thread, and the messages will be transferred to the corresponding
  //   remote (i.e., the 'ten_remote_t' object) directly, the app thread will
  //   not access the 'migration_state' any more.
  TEN_CONNECTION_MIGRATION_STATE migration_state =
      ten_connection_get_migration_state(connection);

  if (migration_state == TEN_CONNECTION_MIGRATION_STATE_INIT) {
    {
      // Feed the very first message to the TEN world.

      ten_listnode_t *first_msg_node = ten_list_pop_front(msgs);
      ten_shared_ptr_t *first_msg = ten_smart_ptr_listnode_get(first_msg_node);
      TEN_ASSERT(first_msg && ten_msg_check_integrity(first_msg),
                 "Invalid argument.");

      ten_protocol_on_input(self, first_msg);
      ten_listnode_destroy(first_msg_node);
    }

    // Cache the rest messages.
    if (!ten_list_is_empty(msgs)) {
      // TODO(Liu): The 'in_msgs' queue only accessed by one thread at any time,
      // does it need to be protected by 'in_lock'?
      TEN_DO_WITH_MUTEX_LOCK(self->in_lock,
                             { ten_list_concat(&self->in_msgs, msgs); });
    }
  } else if (migration_state == TEN_CONNECTION_MIGRATION_STATE_DONE) {
    ten_protocol_on_inputs(self, msgs);
  } else {
    TEN_ASSERT(
        0, "The stream should be frozen before the migration is completed.");
  }
}

static void ten_stream_on_data(ten_stream_t *stream, void *data, int size) {
  TEN_ASSERT(stream, "Should not happen.");

  ten_protocol_integrated_t *protocol =
      (ten_protocol_integrated_t *)stream->user_data;
  TEN_ASSERT(protocol, "Invalid argument.");

  ten_protocol_t *base_protocol = &protocol->base;
  TEN_ASSERT(ten_protocol_check_integrity(base_protocol, true),
             "Should not happen.");
  TEN_ASSERT(ten_protocol_attach_to(base_protocol) ==
                 TEN_PROTOCOL_ATTACH_TO_CONNECTION,
             "Should not happen.");
  TEN_ASSERT(ten_protocol_role_is_communication(base_protocol),
             "Should not happen.");

  ten_connection_t *connection = base_protocol->attached_target.connection;
  TEN_ASSERT(connection && ten_connection_check_integrity(connection, true),
             "Should not happen.");

  if (size < 0) {
    // Something unexpected happened, close the protocol.
    TEN_LOGV("Failed to receive data, close the protocol: %d", size);

    // This branch means that the client side closes the physical connection
    // first, and then the corresponding protocol will be closed. An example of
    // this case is 'ExtensionTest.BasicMultiAppCloseThroughEngine'.
    //
    // But the closing of the protocol should always be an _async_ operation.
    // Take 'ExtensionTest.BasicMultiAppCloseThroughEngine' as an example, the
    // reason is as follows:
    //
    // 1. The client sends a custom cmd to the engine, and the corresponding
    //    remote (i.e., a 'ten_remote_t' object) is created, then the connection
    //    (i.e., a 'ten_connection_t' object) attached to the remote.
    //
    // 2. The client sends a 'close_app' cmd through the same connection, the
    //    cmd will be handled by the engine as the connection attaches to the
    //    remote now. And then the 'close_app' cmd will be submitted to the
    //    app's message queue as an eventloop task.
    //
    // 3. Then the client closes the physical connection, this function will be
    //    called from the app thread (because the engine doesn't have its own
    //    engine thread in this case). If the protocol (i.e., the
    //    'ten_protocol_t' object) is closed synchronized, the connection maybe
    //    destroyed before the 'close_app' cmd (in step 2) is executed. As the
    //    'close_app' cmd is sent from the client, the 'original_connection'
    //    field of the 'close_app' cmd is not NULL, then there will be memory
    //    access issue when handling the cmd.
    //
    // So the closing of the protocol should always be an _async_ operation, as
    // the stream is being closed and there is no chance to receive data from
    // the stream. The 'ten_protocol_close_task_()' will the last task related
    // to the connection.
    ten_ref_inc_ref(&base_protocol->ref);
    ten_runloop_post_task_tail(ten_connection_get_attached_runloop(connection),
                               ten_protocol_close_task, base_protocol, NULL);
  } else if (size > 0) {
    ten_list_t msgs = TEN_LIST_INIT_VAL;

    protocol->on_input(
        protocol, TEN_BUF_STATIC_INIT_WITH_DATA_UNOWNED(data, size), &msgs);

    ten_protocol_on_inputs_based_on_migration_state(base_protocol, &msgs);

    ten_list_clear(&msgs);
  }
}

static void ten_protocol_integrated_send_buf(ten_protocol_integrated_t *self,
                                             ten_buf_t buf) {
  TEN_ASSERT(self && ten_protocol_check_integrity(&self->base, true),
             "Should not happen.");

  TEN_UNUSED int rc =
      ten_stream_send(self->role_facility.communication_stream,
                      (const char *)buf.data, buf.content_size, buf.data);
  TEN_ASSERT(!rc, "ten_stream_send() failed: %d", rc);
}

static void ten_protocol_integrated_on_output(ten_protocol_integrated_t *self) {
  TEN_ASSERT(self && ten_protocol_check_integrity(&self->base, true),
             "Should not happen.");
  TEN_ASSERT(ten_protocol_role_is_communication(&self->base),
             "Should not happen.");

  if (ten_protocol_is_closing(&self->base)) {
    TEN_LOGD("Protocol is closing, do not actually send msgs.");
    return;
  }

  ten_list_t out_msgs_ = TEN_LIST_INIT_VAL;

  TEN_UNUSED int rc = ten_mutex_lock(self->base.out_lock);
  TEN_ASSERT(!rc, "Should not happen.");

  ten_list_swap(&out_msgs_, &self->base.out_msgs);

  rc = ten_mutex_unlock(self->base.out_lock);
  TEN_ASSERT(!rc, "Should not happen.");

  if (ten_list_size(&out_msgs_)) {
    ten_buf_t output_buf = TEN_BUF_STATIC_INIT_UNOWNED;

    if (self->on_output) {
      output_buf = self->on_output(self, &out_msgs_);
    }

    if (output_buf.data) {
      // If the connection is a TCP channel, and is reset by peer, the following
      // send operation would cause a SIGPIPE signal, and the default behavior
      // is to terminate the whole process. That's why we need to ignore SIGPIPE
      // signal when the app is initializing itself. Please see ten_app_create()
      // for more detail.
      ten_protocol_integrated_send_buf(self, output_buf);
    }
  }
}

static void ten_stream_on_data_sent(ten_stream_t *stream, int status,
                                    TEN_UNUSED void *user_data) {
  TEN_ASSERT(stream, "Should not happen.");

  ten_protocol_integrated_t *protocol =
      (ten_protocol_integrated_t *)stream->user_data;
  TEN_ASSERT(protocol && ten_protocol_check_integrity(&protocol->base, true),
             "Should not happen.");
  TEN_ASSERT(ten_protocol_attach_to(&protocol->base) ==
                     TEN_PROTOCOL_ATTACH_TO_CONNECTION &&
                 ten_protocol_role_is_communication(&protocol->base),
             "Should not happen.");

  if (status) {
    TEN_LOGI("Failed to send data, close the protocol: %d", status);

    ten_protocol_close(&protocol->base);
  } else {
    // Trigger myself again to send out more messages if possible.
    ten_protocol_integrated_on_output(protocol);
  }
}

static void ten_stream_on_data_free(TEN_UNUSED ten_stream_t *stream,
                                    TEN_UNUSED int status, void *user_data) {
  TEN_FREE(user_data);
}

static void ten_protocol_integrated_set_stream(ten_protocol_integrated_t *self,
                                               ten_stream_t *stream) {
  TEN_ASSERT(self && ten_protocol_check_integrity(&self->base, true),
             "Should not happen.");
  TEN_ASSERT(stream, "Should not happen.");
  TEN_ASSERT(ten_protocol_role_is_communication(&self->base),
             "Should not happen.");

  self->role_facility.communication_stream = stream;

  stream->user_data = self;
  stream->on_message_read = ten_stream_on_data;
  stream->on_message_sent = ten_stream_on_data_sent;
  stream->on_message_free = ten_stream_on_data_free;

  ten_stream_set_on_closed(stream, ten_protocol_integrated_on_stream_closed,
                           self);
}

static void ten_app_thread_on_client_protocol_created(ten_env_t *ten_env,
                                                      void *instance,
                                                      void *cb_data) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_protocol_integrated_t *protocol = instance;
  TEN_ASSERT(protocol && ten_protocol_check_integrity(&protocol->base, true),
             "Should not happen.");

  ten_stream_t *stream = cb_data;
  TEN_ASSERT(stream, "Should not happen.");

  ten_app_t *app = ten_env_get_attached_app(ten_env);
  TEN_ASSERT(app && ten_app_check_integrity(app, true), "Should not happen.");

  ten_protocol_t *listening_base_protocol = app->endpoint_protocol;
  TEN_ASSERT(listening_base_protocol &&
                 ten_protocol_check_integrity(listening_base_protocol, true),
             "Should not happen.");

  ten_protocol_determine_default_property_value(&protocol->base);

  ten_protocol_t *new_communication_base_protocol = &protocol->base;
  TEN_ASSERT(
      new_communication_base_protocol &&
          ten_protocol_check_integrity(new_communication_base_protocol, true),
      "Should not happen.");

  // Setup important fields of the newly created protocol.
  new_communication_base_protocol->on_connected =
      listening_base_protocol->on_connected;

  // We can _not_ know whether the protocol role is
  // 'TEN_PROTOCOL_ROLE_IN_INTERNAL' or 'TEN_PROTOCOL_ROLE_IN_EXTERNAL' until
  // the message received from the protocol is processed. Refer to
  // 'ten_connection_on_msgs()' and
  // 'ten_connection_handle_command_from_external_client()'.
  new_communication_base_protocol->role = TEN_PROTOCOL_ROLE_IN_DEFAULT;

  // Attach the newly created protocol to app first.
  ten_protocol_attach_to_app(new_communication_base_protocol,
                             listening_base_protocol->attached_target.app);

  TEN_ASSERT(listening_base_protocol->on_accepted, "Should not happen.");
  TEN_UNUSED ten_connection_t *connection =
      listening_base_protocol->on_accepted(new_communication_base_protocol);
  TEN_ASSERT(connection && ten_connection_check_integrity(connection, true),
             "Should not happen.");

  ten_protocol_integrated_set_stream(protocol, stream);

  TEN_LOGD("Start read from stream");

  TEN_UNUSED int rc = ten_stream_start_read(stream);
  TEN_ASSERT(!rc, "ten_stream_start_read() failed: %d", rc);
}

static void ten_protocol_integrated_on_client_accepted(
    ten_transport_t *transport, ten_stream_t *stream, TEN_UNUSED int status) {
  TEN_ASSERT(transport && stream, "Should not happen.");

  ten_protocol_integrated_t *listening_protocol = transport->user_data;
  TEN_ASSERT(listening_protocol, "Should not happen.");

  ten_protocol_t *listening_base_protocol = &listening_protocol->base;
  TEN_ASSERT(listening_base_protocol &&
                 ten_protocol_check_integrity(listening_base_protocol, true),
             "Should not happen.");

  ten_app_t *app = listening_base_protocol->attached_target.app;
  TEN_ASSERT(app && ten_app_check_integrity(app, true), "Should not happen.");

  bool rc = ten_addon_create_instance_async(
      app->ten_env,
      ten_string_get_raw_str(&listening_base_protocol->addon_host->name),
      ten_string_get_raw_str(&listening_base_protocol->addon_host->name),
      TEN_ADDON_TYPE_PROTOCOL, ten_app_thread_on_client_protocol_created,
      stream);
  TEN_ASSERT(rc, "Should not happen.");
}

static void ten_protocol_integrated_listen(ten_protocol_integrated_t *self,
                                           const char *uri) {
  TEN_ASSERT(self && ten_protocol_check_integrity(&self->base, true),
             "Should not happen.");
  TEN_ASSERT(uri, "Should not happen.");
  // Only when the protocol attached to a app could start listening.
  TEN_ASSERT(ten_protocol_attach_to(&self->base) == TEN_PROTOCOL_ATTACH_TO_APP,
             "Should not happen.");

  ten_runloop_t *loop =
      ten_app_get_attached_runloop(self->base.attached_target.app);
  TEN_ASSERT(loop, "Should not happen.");

  ten_transport_t *transport = ten_transport_create(loop);
  TEN_ASSERT(transport, "Should not happen.");

  self->base.role = TEN_PROTOCOL_ROLE_LISTEN;
  self->role_facility.listening_transport = transport;

  transport->user_data = self;
  transport->on_client_accepted = ten_protocol_integrated_on_client_accepted;
  ten_transport_set_close_cb(transport,
                             ten_protocol_integrated_on_transport_closed, self);

  ten_string_t *transport_uri = ten_protocol_uri_to_transport_uri(uri);

  TEN_LOGI("%s start listening.", ten_string_get_raw_str(transport_uri));

  TEN_UNUSED int rc = ten_transport_listen(transport, transport_uri);
  if (rc) {
    TEN_LOGE("Failed to create a listening endpoint (%s): %d",
             ten_string_get_raw_str(transport_uri), rc);
  }

  ten_string_destroy(transport_uri);
}

static void ten_protocol_integrated_on_output_task(void *self_,
                                                   TEN_UNUSED void *arg) {
  ten_protocol_integrated_t *self = (ten_protocol_integrated_t *)self_;
  TEN_ASSERT(self && ten_protocol_check_integrity(&self->base, true),
             "Should not happen.");

  if (!ten_protocol_is_closing(&self->base)) {
    // Execute the actual task if the 'protocol' is still alive.
    ten_protocol_integrated_on_output(self);
  }

  // The task is completed, so delete a reference to the 'protocol' to reflect
  // this fact.
  ten_ref_dec_ref(&self->base.ref);
}

/**
 * @brief Extension threads might directly call this function to send out
 * messages. For example, if extension threads want to send out data/
 * video_frame/ audio_frame messages, because it's unnecessary to perform some
 * bookkeeping operations for command-type messages, extension threads would
 * _not_ transfer the messages to the engine thread first, extension threads
 * will call this function directly to send out those messages. Therefore, in
 * order to maintain thread safety, this function will use runloop task to add
 * an async task to the attached runloop of the protocol to send out those
 * messages in the correct thread.
 */
static void ten_protocol_integrated_on_output_async(
    ten_protocol_integrated_t *self, ten_list_t *msgs) {
  TEN_ASSERT(self, "Should not happen.");
  TEN_ASSERT(msgs, "Invalid argument.");

  ten_protocol_t *protocol = &self->base;
  TEN_ASSERT(protocol &&
                 // TEN_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be used in
                 // different threads.
                 ten_protocol_check_integrity(protocol, false) &&
                 ten_protocol_role_is_communication(protocol),
             "Should not happen.");

  TEN_UNUSED int rc = ten_mutex_lock(protocol->out_lock);
  TEN_ASSERT(!rc, "Should not happen.");

  ten_list_concat(&protocol->out_msgs, msgs);

  rc = ten_mutex_unlock(protocol->out_lock);
  TEN_ASSERT(!rc, "Should not happen.");

  // Before posting a runloop task, we have to add a reference to the
  // 'protocol', so that it will not be destroyed _before_ the runloop task is
  // executed.
  ten_ref_inc_ref(&protocol->ref);

  ten_runloop_t *loop = ten_protocol_get_attached_runloop(protocol);
  TEN_ASSERT(loop, "Should not happen.");

  ten_runloop_post_task_tail(loop, ten_protocol_integrated_on_output_task, self,
                             NULL);
}

static void on_server_connected(ten_transport_t *transport,
                                ten_stream_t *stream, int status) {
  ten_protocol_integrated_t *protocol = transport->user_data;
  TEN_ASSERT(protocol && ten_protocol_check_integrity(&protocol->base, true),
             "Should not happen.");
  TEN_ASSERT(ten_protocol_role_is_communication(&protocol->base),
             "Should not happen.");

  // If the protocol is closing, do not handle this connect success event.
  if (ten_protocol_is_closing(&protocol->base)) {
    return;
  }

  bool success = status >= 0;

  if (success) {
    ten_protocol_integrated_set_stream(protocol, stream);
  }

  if (protocol->base.on_connected) {
    protocol->base.on_connected(&protocol->base, success);
  }

  if (success) {
    ten_stream_start_read(stream);
  } else {
    ten_stream_close(stream);
  }
}

static bool ten_protocol_integrated_connect_to(ten_protocol_integrated_t *self,
                                               const char *uri) {
  TEN_ASSERT(self && ten_protocol_check_integrity(&self->base, true),
             "Should not happen.");
  TEN_ASSERT(uri, "Should not happen.");
  TEN_ASSERT(
      ten_protocol_attach_to(&self->base) == TEN_PROTOCOL_ATTACH_TO_CONNECTION,
      "Should not happen.");

  // For integrated protocols, this function must be called in the engine
  // thread.
  TEN_ASSERT(
      ten_engine_check_integrity(
          self->base.attached_target.connection->attached_target.remote->engine,
          true),
      "Should not happen.");

  ten_runloop_t *loop = ten_remote_get_attached_runloop(
      self->base.attached_target.connection->attached_target.remote);
  TEN_ASSERT(loop, "Should not happen.");

  ten_transport_t *transport = ten_transport_create(loop);
  transport->user_data = self;
  transport->on_server_connected = on_server_connected;

  ten_string_t *transport_uri = ten_protocol_uri_to_transport_uri(uri);
  TEN_ASSERT(transport_uri, "Should not happen.");
  if (ten_transport_connect(transport, transport_uri)) {
    TEN_LOGW("Failed to connect to %s", ten_string_get_raw_str(transport_uri));
    // Something wrong when connecting.
    ten_transport_close(transport);
    return false;
  }
  ten_string_destroy(transport_uri);
  return true;
}

static void ten_protocol_integrated_on_stream_cleaned(
    ten_protocol_integrated_t *self) {
  TEN_ASSERT(self && ten_protocol_check_integrity(&self->base, true),
             "We are in the app thread now.");

  // Integrated protocol has been cleaned, call on_cleaned_for_internal callback
  // now.
  self->base.on_cleaned_for_internal(&self->base);
}

static void ten_protocol_integrated_clean(ten_protocol_integrated_t *self) {
  TEN_ASSERT(self && ten_protocol_check_integrity(&self->base, true),
             "Should not happen.");

  // Integrated protocol needs to _clean_ the containing stream. Because the
  // containing stream will be recreated in the engine thread again.

  ten_stream_set_on_closed(self->role_facility.communication_stream,
                           ten_protocol_integrated_on_stream_cleaned, self);
  ten_stream_close(self->role_facility.communication_stream);
}

static void ten_stream_migrated(ten_stream_t *stream, void **user_data) {
  ten_engine_t *engine = user_data[0];
  TEN_ASSERT(engine && ten_engine_check_integrity(engine, true),
             "The 'stream' has already been migrated to the engine thread. "
             "Therefore, this function is called in the engine thread.");

  ten_connection_t *connection = user_data[1];
  TEN_ASSERT(connection &&
                 // TEN_NOLINTNEXTLINE(thread-check)
                 ten_connection_check_integrity(connection, false),
             "The connection is created in the app thread, and _before_ the "
             "cleaning is completed, the connection should still belongs to "
             "the app thread, and only until the cleaning is completed, we can "
             "transfer the connection to the target thread (e.g.: the engine "
             "thread or the client thread). Because this function is called in "
             "the engine thread, so we can not perform thread checking here, "
             "and need to be careful to consider thread safety when access the "
             "connection instance before the cleaning is completed.");

  ten_protocol_integrated_t *protocol =
      (ten_protocol_integrated_t *)connection->protocol;
  TEN_ASSERT(
      protocol &&
          // TEN_NOLINTNEXTLINE(thread-check)
          ten_protocol_check_integrity(&protocol->base, false),
      "The reason is the same as the above comments about 'connection'.");

  ten_shared_ptr_t *cmd = user_data[2];
  TEN_ASSERT(cmd && ten_cmd_base_check_integrity(cmd), "Should not happen.");

  TEN_FREE(user_data);

  if (stream) {
    // The 'connection' belongs to the app thread, so we can not call
    // ten_connection_clean() here directly, we have to switch to the app thread
    // to do it.
    ten_app_clean_connection_async(engine->app, connection);

    ten_engine_on_connection_cleaned(engine, connection, cmd);
    ten_shared_ptr_destroy(cmd);

    // The clean up of the connection is done, it's time to create new stream
    // and async which are bound to the engine event loop to the connection.
    ten_protocol_integrated_set_stream(protocol, stream);

    // Re-start to read from this stream when we have set it up correctly with
    // the correct newly created eventloop.
    ten_stream_start_read(stream);
  } else {
    TEN_ASSERT(0, "Failed to migrate protocol.");
  }
}

/**
 * @brief Migrate 'protocol' from the app thread to the engine thread.
 * 'integrated' protocol needs this because the containing 'stream' needs to be
 * transferred from the app thread to the engine thread.
 */
static void ten_protocol_integrated_migrate(ten_protocol_integrated_t *self,
                                            ten_engine_t *engine,
                                            ten_connection_t *connection,
                                            ten_shared_ptr_t *cmd) {
  TEN_ASSERT(self && ten_protocol_check_integrity(&self->base, true),
             "Should not happen.");
  TEN_ASSERT(engine &&
                 // TEN_NOLINTNEXTLINE(thread-check)
                 ten_engine_check_integrity(engine, false),
             "The function is called in the app thread, and will migrate the "
             "protocol to the engine thread.");
  TEN_ASSERT(engine->app && ten_app_check_integrity(engine->app, true),
             "The function is called in the app thread, and will migrate the "
             "protocol to the engine thread.");
  TEN_ASSERT(connection && ten_connection_check_integrity(connection, true),
             "'connection' belongs to app thread now.");
  TEN_ASSERT(cmd && ten_cmd_base_check_integrity(cmd), "Should not happen.");

  ten_stream_t *stream = self->role_facility.communication_stream;
  TEN_ASSERT(stream && ten_stream_check_integrity(stream),
             "Should not happen.");

  // Stop reading from the stream _before_ migration.
  ten_stream_stop_read(stream);

  void **user_data = (void **)TEN_MALLOC(3 * sizeof(void *));
  TEN_ASSERT(user_data, "Failed to allocate memory.");

  user_data[0] = engine;
  user_data[1] = connection;
  user_data[2] = ten_shared_ptr_clone(cmd);

  ten_stream_migrate(stream, ten_runloop_current(), engine->loop, user_data,
                     ten_stream_migrated);
}

static void ten_protocol_integrated_on_base_protocol_cleaned(
    ten_protocol_t *self, TEN_UNUSED bool is_migration_state_reset) {
  TEN_ASSERT(self && ten_protocol_check_integrity(self, true),
             "Should not happen.");

  // The integrated protocol determines the closing event based on the message
  // size read from the stream, and the stream will be frozen during the
  // migration. So there is no closing event after the migration is completed
  // here.

  ten_list_t msgs = TEN_LIST_INIT_VAL;
  TEN_DO_WITH_MUTEX_LOCK(self->in_lock,
                         { ten_list_swap(&msgs, &self->in_msgs); });

  if (ten_list_is_empty(&msgs)) {
    return;
  }

  ten_protocol_on_inputs_based_on_migration_state(self, &msgs);
  ten_list_clear(&msgs);
}

void ten_protocol_integrated_init(
    ten_protocol_integrated_t *self, const char *name,
    ten_protocol_integrated_on_input_func_t on_input,
    ten_protocol_integrated_on_output_func_t on_output) {
  TEN_ASSERT(self && name, "Should not happen.");

  ten_protocol_init(
      &self->base, name,
      (ten_protocol_close_func_t)ten_protocol_integrated_close,
      (ten_protocol_on_output_func_t)ten_protocol_integrated_on_output_async,
      (ten_protocol_listen_func_t)ten_protocol_integrated_listen,
      (ten_protocol_connect_to_func_t)ten_protocol_integrated_connect_to,
      (ten_protocol_migrate_func_t)ten_protocol_integrated_migrate,
      (ten_protocol_clean_func_t)ten_protocol_integrated_clean);

  // TODO(Wei): If the implementation protocol implements 'ten_closeable_t'
  // interface, change the following logic accordingly.
  ten_closeable_set_action_to_close_myself(
      &self->base.closeable, ten_protocol_action_to_close_myself, NULL);

  ten_closeable_add_be_notified(&self->base.closeable, &self->base.closeable,
                                NULL, NULL, NULL, NULL,
                                ten_protocol_on_impl_closed_all_done, NULL);

  self->base.role = TEN_PROTOCOL_ROLE_INVALID;
  self->base.on_cleaned_for_external =
      ten_protocol_integrated_on_base_protocol_cleaned;

  self->role_facility.communication_stream = NULL;
  self->role_facility.listening_transport = NULL;

  self->on_input = on_input;
  self->on_output = on_output;
}
