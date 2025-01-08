//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/protocol/integrated/protocol_integrated.h"

#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/addon/addon_host.h"
#include "include_internal/axis_runtime/addon/protocol/protocol.h"
#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/app/migration.h"
#include "include_internal/axis_runtime/connection/connection.h"
#include "include_internal/axis_runtime/connection/migration.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/engine/internal/migration.h"
#include "include_internal/axis_runtime/engine/internal/thread.h"
#include "include_internal/axis_runtime/engine/msg_interface/common.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/protocol/close.h"
#include "include_internal/axis_runtime/protocol/integrated/close.h"
#include "include_internal/axis_runtime/protocol/protocol.h"
#include "include_internal/axis_runtime/remote/remote.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "include_internal/axis_utils/log/log.h"
#include "axis_utils/container/list.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/io/stream.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/lib/ref.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

static void axis_protocol_close_task(void *self_, axis_UNUSED void *arg) {
  axis_protocol_t *self = (axis_protocol_t *)self_;
  axis_ASSERT(self && axis_protocol_check_integrity(self, true),
             "Access across threads.");

  axis_protocol_close(self);

  axis_ref_dec_ref(&self->ref);
}

static void axis_protocol_on_inputs_based_on_migration_state(
    axis_protocol_t *self, axis_list_t *msgs) {
  axis_ASSERT(self && axis_protocol_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(msgs, "Should not happen.");
  axis_ASSERT(axis_protocol_attach_to(self) == axis_PROTOCOL_ATTACH_TO_CONNECTION,
             "Should not happen.");

  axis_connection_t *connection = self->attached_target.connection;
  axis_ASSERT(connection && axis_connection_check_integrity(connection, true),
             "Should not happen.");

  // The stream will be frozen before the migration, and this function is only
  // called when the integrated protocol retrieves data from the stream. So the
  // 'axis_connection_t::migration_state' here is only read or written in one
  // thread at any time.
  //
  // - Before the migration, this function is called in the app thread, the
  //   'axis_connection_t::migration_state' will be read and written in the app
  //   thread.
  //
  // - No messages during the migration, which means this function will not be
  //   called once the migration is started.
  //
  // - When the migration is completed, this function is always called in the
  //   engine thread, and the messages will be transferred to the corresponding
  //   remote (i.e., the 'axis_remote_t' object) directly, the app thread will
  //   not access the 'migration_state' any more.
  axis_CONNECTION_MIGRATION_STATE migration_state =
      axis_connection_get_migration_state(connection);

  if (migration_state == axis_CONNECTION_MIGRATION_STATE_INIT) {
    {
      // Feed the very first message to the TEN world.

      axis_listnode_t *first_msg_node = axis_list_pop_front(msgs);
      axis_shared_ptr_t *first_msg = axis_smart_ptr_listnode_get(first_msg_node);
      axis_ASSERT(first_msg && axis_msg_check_integrity(first_msg),
                 "Invalid argument.");

      axis_protocol_on_input(self, first_msg);
      axis_listnode_destroy(first_msg_node);
    }

    // Cache the rest messages.
    if (!axis_list_is_empty(msgs)) {
      // TODO(Liu): The 'in_msgs' queue only accessed by one thread at any time,
      // does it need to be protected by 'in_lock'?
      axis_DO_WITH_MUTEX_LOCK(self->in_lock,
                             { axis_list_concat(&self->in_msgs, msgs); });
    }
  } else if (migration_state == axis_CONNECTION_MIGRATION_STATE_DONE) {
    axis_protocol_on_inputs(self, msgs);
  } else {
    axis_ASSERT(
        0, "The stream should be frozen before the migration is completed.");
  }
}

static void axis_stream_on_data(axis_stream_t *stream, void *data, int size) {
  axis_ASSERT(stream, "Should not happen.");

  axis_protocol_integrated_t *protocol =
      (axis_protocol_integrated_t *)stream->user_data;
  axis_ASSERT(protocol, "Invalid argument.");

  axis_protocol_t *base_protocol = &protocol->base;
  axis_ASSERT(axis_protocol_check_integrity(base_protocol, true),
             "Should not happen.");
  axis_ASSERT(axis_protocol_attach_to(base_protocol) ==
                 axis_PROTOCOL_ATTACH_TO_CONNECTION,
             "Should not happen.");
  axis_ASSERT(axis_protocol_role_is_communication(base_protocol),
             "Should not happen.");

  axis_connection_t *connection = base_protocol->attached_target.connection;
  axis_ASSERT(connection && axis_connection_check_integrity(connection, true),
             "Should not happen.");

  if (size < 0) {
    // Something unexpected happened, close the protocol.
    axis_LOGD("Failed to receive data, close the protocol: %d", size);

    // This branch means that the client side closes the physical connection
    // first, and then the corresponding protocol will be closed. An example of
    // this case is 'ExtensionTest.BasicMultiAppCloseThroughEngine'.
    //
    // But the closing of the protocol should always be an _async_ operation.
    // Take 'ExtensionTest.BasicMultiAppCloseThroughEngine' as an example, the
    // reason is as follows:
    //
    // 1. The client sends a custom cmd to the engine, and the corresponding
    //    remote (i.e., a 'axis_remote_t' object) is created, then the connection
    //    (i.e., a 'axis_connection_t' object) attached to the remote.
    //
    // 2. The client sends a 'close_app' cmd through the same connection, the
    //    cmd will be handled by the engine as the connection attaches to the
    //    remote now. And then the 'close_app' cmd will be submitted to the
    //    app's message queue as an eventloop task.
    //
    // 3. Then the client closes the physical connection, this function will be
    //    called from the app thread (because the engine doesn't have its own
    //    engine thread in this case). If the protocol (i.e., the
    //    'axis_protocol_t' object) is closed synchronized, the connection maybe
    //    destroyed before the 'close_app' cmd (in step 2) is executed. As the
    //    'close_app' cmd is sent from the client, the 'original_connection'
    //    field of the 'close_app' cmd is not NULL, then there will be memory
    //    access issue when handling the cmd.
    //
    // So the closing of the protocol should always be an _async_ operation, as
    // the stream is being closed and there is no chance to receive data from
    // the stream. The 'axis_protocol_close_task_()' will the last task related
    // to the connection.
    axis_ref_inc_ref(&base_protocol->ref);
    axis_runloop_post_task_tail(axis_connection_get_attached_runloop(connection),
                               axis_protocol_close_task, base_protocol, NULL);
  } else if (size > 0) {
    axis_list_t msgs = axis_LIST_INIT_VAL;

    protocol->on_input(
        protocol, axis_BUF_STATIC_INIT_WITH_DATA_UNOWNED(data, size), &msgs);

    axis_protocol_on_inputs_based_on_migration_state(base_protocol, &msgs);

    axis_list_clear(&msgs);
  }
}

static void axis_protocol_integrated_send_buf(axis_protocol_integrated_t *self,
                                             axis_buf_t buf) {
  axis_ASSERT(self && axis_protocol_check_integrity(&self->base, true),
             "Should not happen.");

  axis_UNUSED int rc =
      axis_stream_send(self->role_facility.communication_stream,
                      (const char *)buf.data, buf.content_size, buf.data);
  axis_ASSERT(!rc, "axis_stream_send() failed: %d", rc);
}

static void axis_protocol_integrated_on_output(axis_protocol_integrated_t *self) {
  axis_ASSERT(self && axis_protocol_check_integrity(&self->base, true),
             "Should not happen.");
  axis_ASSERT(axis_protocol_role_is_communication(&self->base),
             "Should not happen.");

  if (axis_protocol_is_closing(&self->base)) {
    axis_LOGD("Protocol is closing, do not actually send msgs.");
    return;
  }

  axis_list_t out_msgs_ = axis_LIST_INIT_VAL;

  axis_UNUSED int rc = axis_mutex_lock(self->base.out_lock);
  axis_ASSERT(!rc, "Should not happen.");

  axis_list_swap(&out_msgs_, &self->base.out_msgs);

  rc = axis_mutex_unlock(self->base.out_lock);
  axis_ASSERT(!rc, "Should not happen.");

  if (axis_list_size(&out_msgs_)) {
    axis_buf_t output_buf = axis_BUF_STATIC_INIT_UNOWNED;

    if (self->on_output) {
      output_buf = self->on_output(self, &out_msgs_);
    }

    if (output_buf.data) {
      // If the connection is a TCP channel, and is reset by peer, the following
      // send operation would cause a SIGPIPE signal, and the default behavior
      // is to terminate the whole process. That's why we need to ignore SIGPIPE
      // signal when the app is initializing itself. Please see axis_app_create()
      // for more detail.
      axis_protocol_integrated_send_buf(self, output_buf);
    }
  }
}

static void axis_stream_on_data_sent(axis_stream_t *stream, int status,
                                    axis_UNUSED void *user_data) {
  axis_ASSERT(stream, "Should not happen.");

  axis_protocol_integrated_t *protocol =
      (axis_protocol_integrated_t *)stream->user_data;
  axis_ASSERT(protocol && axis_protocol_check_integrity(&protocol->base, true),
             "Should not happen.");
  axis_ASSERT(axis_protocol_attach_to(&protocol->base) ==
                     axis_PROTOCOL_ATTACH_TO_CONNECTION &&
                 axis_protocol_role_is_communication(&protocol->base),
             "Should not happen.");

  if (status) {
    axis_LOGI("Failed to send data, close the protocol: %d", status);

    axis_protocol_close(&protocol->base);
  } else {
    // Trigger myself again to send out more messages if possible.
    axis_protocol_integrated_on_output(protocol);
  }
}

static void axis_stream_on_data_free(axis_UNUSED axis_stream_t *stream,
                                    axis_UNUSED int status, void *user_data) {
  axis_FREE(user_data);
}

static void axis_protocol_integrated_set_stream(axis_protocol_integrated_t *self,
                                               axis_stream_t *stream) {
  axis_ASSERT(self && axis_protocol_check_integrity(&self->base, true),
             "Should not happen.");
  axis_ASSERT(stream, "Should not happen.");
  axis_ASSERT(axis_protocol_role_is_communication(&self->base),
             "Should not happen.");

  self->role_facility.communication_stream = stream;

  stream->user_data = self;
  stream->on_message_read = axis_stream_on_data;
  stream->on_message_sent = axis_stream_on_data_sent;
  stream->on_message_free = axis_stream_on_data_free;

  axis_stream_set_on_closed(stream, axis_protocol_integrated_on_stream_closed,
                           self);
}

static void axis_app_thread_on_client_protocol_created(axis_env_t *axis_env,
                                                      axis_protocol_t *instance,
                                                      void *cb_data) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_protocol_integrated_t *protocol = (axis_protocol_integrated_t *)instance;
  axis_ASSERT(protocol && axis_protocol_check_integrity(&protocol->base, true),
             "Should not happen.");

  axis_stream_t *stream = cb_data;
  axis_ASSERT(stream, "Should not happen.");

  axis_protocol_on_client_accepted_func_t on_client_accepted = stream->user_data;
  axis_ASSERT(on_client_accepted, "Should not happen.");

  axis_app_t *app = axis_env_get_attached_app(axis_env);
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Should not happen.");

  axis_protocol_t *listening_base_protocol = app->endpoint_protocol;
  axis_ASSERT(listening_base_protocol &&
                 axis_protocol_check_integrity(listening_base_protocol, true),
             "Should not happen.");

  axis_protocol_determine_default_property_value(&protocol->base);

  axis_protocol_t *new_communication_base_protocol = &protocol->base;
  axis_ASSERT(
      new_communication_base_protocol &&
          axis_protocol_check_integrity(new_communication_base_protocol, true),
      "Should not happen.");

  // Attach the newly created protocol to app first.
  axis_protocol_attach_to_app(new_communication_base_protocol,
                             listening_base_protocol->attached_target.app);

  axis_UNUSED axis_connection_t *connection = on_client_accepted(
      listening_base_protocol, new_communication_base_protocol);
  axis_ASSERT(connection && axis_connection_check_integrity(connection, true),
             "Should not happen.");

  axis_protocol_integrated_set_stream(protocol, stream);

  axis_LOGD("Start read from stream");

  axis_UNUSED int rc = axis_stream_start_read(stream);
  axis_ASSERT(!rc, "axis_stream_start_read() failed: %d", rc);
}

static void axis_transport_on_client_accepted(axis_transport_t *transport,
                                             axis_stream_t *stream,
                                             axis_UNUSED int status) {
  axis_ASSERT(transport && stream, "Should not happen.");

  axis_protocol_integrated_t *listening_protocol = transport->user_data;
  axis_ASSERT(listening_protocol, "Should not happen.");

  // The `on_client_accepted_data` in transport stores the `on_client_accepted`
  // callback function set by the TEN runtime.
  axis_protocol_on_client_accepted_func_t on_client_accepted =
      transport->on_client_accepted_data;
  axis_ASSERT(on_client_accepted, "Should not happen.");

  stream->user_data = on_client_accepted;

  axis_protocol_t *listening_base_protocol = &listening_protocol->base;
  axis_ASSERT(listening_base_protocol &&
                 axis_protocol_check_integrity(listening_base_protocol, true),
             "Should not happen.");

  axis_app_t *app = listening_base_protocol->attached_target.app;
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  // We can _not_ know whether the protocol role is
  // 'axis_PROTOCOL_ROLE_IN_INTERNAL' or 'axis_PROTOCOL_ROLE_IN_EXTERNAL' until
  // the message received from the protocol is processed. Refer to
  // 'axis_connection_on_msgs()' and
  // 'axis_connection_handle_command_from_external_client()'.
  bool rc = axis_addon_create_protocol(
      app->axis_env,
      axis_string_get_raw_str(&listening_base_protocol->addon_host->name),
      axis_string_get_raw_str(&listening_base_protocol->addon_host->name),
      axis_PROTOCOL_ROLE_IN_DEFAULT, axis_app_thread_on_client_protocol_created,
      stream, &err);
  axis_ASSERT(rc, "Failed to create protocol, err: %s", axis_error_errmsg(&err));

  axis_error_deinit(&err);
}

static void axis_protocol_integrated_listen(
    axis_protocol_t *self_, const char *uri,
    axis_protocol_on_client_accepted_func_t on_client_accepted) {
  axis_protocol_integrated_t *self = (axis_protocol_integrated_t *)self_;
  axis_ASSERT(self && axis_protocol_check_integrity(&self->base, true),
             "Should not happen.");
  axis_ASSERT(uri, "Should not happen.");
  // Only when the protocol attached to a app could start listening.
  axis_ASSERT(axis_protocol_attach_to(&self->base) == axis_PROTOCOL_ATTACH_TO_APP,
             "Should not happen.");

  axis_runloop_t *loop =
      axis_app_get_attached_runloop(self->base.attached_target.app);
  axis_ASSERT(loop, "Should not happen.");

  axis_transport_t *transport = axis_transport_create(loop);
  axis_ASSERT(transport, "Should not happen.");

  self->base.role = axis_PROTOCOL_ROLE_LISTEN;
  self->role_facility.listening_transport = transport;

  transport->user_data = self;
  // When a client connects, it is first handled using
  // `axis_transport_on_client_accepted`, and only afterward is the
  // `on_client_accepted` defined from TEN runtime. This way, some tasks can be
  // performed within the protocol at the transport/stream layer first, before
  // switching to the TEN runtime's `on_client_accepted` callback.
  transport->on_client_accepted = axis_transport_on_client_accepted;
  transport->on_client_accepted_data = on_client_accepted;

  axis_transport_set_close_cb(transport,
                             axis_protocol_integrated_on_transport_closed, self);

  axis_string_t *transport_uri = axis_protocol_uri_to_transport_uri(uri);

  axis_LOGI("%s start listening.", axis_string_get_raw_str(transport_uri));

  axis_UNUSED int rc = axis_transport_listen(transport, transport_uri);
  if (rc) {
    axis_LOGE("Failed to create a listening endpoint (%s): %d",
             axis_string_get_raw_str(transport_uri), rc);

    // TODO(xilin): Handle the error.
  }

  axis_string_destroy(transport_uri);
}

static void axis_protocol_integrated_on_output_task(void *self_,
                                                   axis_UNUSED void *arg) {
  axis_protocol_integrated_t *self = (axis_protocol_integrated_t *)self_;
  axis_ASSERT(self && axis_protocol_check_integrity(&self->base, true),
             "Should not happen.");

  if (!axis_protocol_is_closing(&self->base)) {
    // Execute the actual task if the 'protocol' is still alive.
    axis_protocol_integrated_on_output(self);
  }

  // The task is completed, so delete a reference to the 'protocol' to reflect
  // this fact.
  axis_ref_dec_ref(&self->base.ref);
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
static void axis_protocol_integrated_on_output_async(
    axis_protocol_integrated_t *self, axis_list_t *msgs) {
  axis_ASSERT(self, "Should not happen.");
  axis_ASSERT(msgs, "Invalid argument.");

  axis_protocol_t *protocol = &self->base;
  axis_ASSERT(protocol && axis_protocol_check_integrity(protocol, true) &&
                 axis_protocol_role_is_communication(protocol),
             "Should not happen.");

  axis_UNUSED int rc = axis_mutex_lock(protocol->out_lock);
  axis_ASSERT(!rc, "Should not happen.");

  axis_list_concat(&protocol->out_msgs, msgs);

  rc = axis_mutex_unlock(protocol->out_lock);
  axis_ASSERT(!rc, "Should not happen.");

  // Before posting a runloop task, we have to add a reference to the
  // 'protocol', so that it will not be destroyed _before_ the runloop task is
  // executed.
  axis_ref_inc_ref(&protocol->ref);

  axis_runloop_t *loop = axis_protocol_get_attached_runloop(protocol);
  axis_ASSERT(loop, "Should not happen.");

  axis_runloop_post_task_tail(loop, axis_protocol_integrated_on_output_task, self,
                             NULL);
}

static void axis_protocol_integrated_on_server_finally_connected(
    axis_protocol_integrated_connect_to_context_t *cb_data, bool success) {
  axis_ASSERT(cb_data, "Should not happen.");
  axis_ASSERT(cb_data->on_server_connected, "Should not happen.");

  axis_protocol_integrated_t *protocol = cb_data->protocol;
  axis_ASSERT(protocol && axis_protocol_check_integrity(&protocol->base, true),
             "Should not happen.");

  cb_data->on_server_connected(&cb_data->protocol->base, success);
  cb_data->on_server_connected = NULL;

  axis_protocol_integrated_connect_to_context_destroy(cb_data);
}

static void axis_transport_on_server_connected_after_retry(
    axis_transport_t *transport, axis_stream_t *stream, int status) {
  axis_protocol_integrated_t *protocol = transport->user_data;
  // Since the transport is created with the runloop of the engine, it is
  // currently in the engine thread.
  axis_ASSERT(protocol && axis_protocol_check_integrity(&protocol->base, true),
             "Should not happen.");
  axis_ASSERT(axis_protocol_role_is_communication(&protocol->base),
             "Should not happen.");

  axis_protocol_integrated_connect_to_context_t *connect_to_context =
      transport->on_server_connected_data;
  axis_ASSERT(connect_to_context, "Should not happen.");
  axis_ASSERT(connect_to_context->on_server_connected, "Should not happen.");

  if (axis_protocol_is_closing(&protocol->base)) {
    axis_stream_close(stream);
    // The ownership of the 'connect_to_context' is transferred to the timer, so
    // the 'connect_to_context' will be freed when the timer is closed.
    return;
  }

  axis_ASSERT(protocol->retry_timer, "Should not happen.");

  bool success = status >= 0;

  if (success) {
    axis_protocol_integrated_set_stream(protocol, stream);

    connect_to_context->on_server_connected(&protocol->base, success);
    transport->on_server_connected_data = NULL;
    // Set 'on_server_connected' to NULL to indicate that this callback has
    // already been called and to prevent it from being called again.
    connect_to_context->on_server_connected = NULL;

    axis_stream_start_read(stream);

    axis_LOGD("Connect to %s successfully after retry",
             axis_string_get_raw_str(&connect_to_context->server_uri));

    axis_timer_stop_async(protocol->retry_timer);
    axis_timer_close_async(protocol->retry_timer);
  } else {
    axis_stream_close(stream);

    // Reset the timer to retry or close the timer if the retry times are
    // exhausted.
    axis_timer_enable(protocol->retry_timer);

    axis_LOGD("Failed to connect to %s after retry",
             axis_string_get_raw_str(&connect_to_context->server_uri));
  }
}

static void axis_protocol_integrated_on_retry_timer_triggered(
    axis_UNUSED axis_timer_t *self, void *on_trigger_data) {
  axis_protocol_integrated_connect_to_context_t *connect_to_context =
      on_trigger_data;
  axis_ASSERT(connect_to_context, "Should not happen.");

  axis_protocol_integrated_t *protocol = connect_to_context->protocol;
  axis_ASSERT(protocol && axis_protocol_check_integrity(&protocol->base, true),
             "Should not happen.");

  axis_runloop_t *loop = axis_protocol_get_attached_runloop(&protocol->base);
  axis_ASSERT(loop, "Should not happen.");

  axis_transport_t *transport = axis_transport_create(loop);
  transport->user_data = protocol;
  transport->on_server_connected =
      axis_transport_on_server_connected_after_retry;
  transport->on_server_connected_data = connect_to_context;

  int rc = axis_transport_connect(transport, &connect_to_context->server_uri);
  if (rc) {
    // If the 'axis_transport_connect' directly returns error, it could be due to
    // invalid parameters or other errors which cannot be solved by retrying.

    axis_LOGW(
        "Failed to connect to %s due to invalid parameters or other fatal "
        "errors.",
        axis_string_get_raw_str(&connect_to_context->server_uri));

    transport->on_server_connected_data = NULL;
    axis_transport_close(transport);

    connect_to_context->on_server_connected(&protocol->base, false);
    // Set 'on_server_connected' to NULL to indicate that this callback has
    // already been called and to prevent it from being called again.
    connect_to_context->on_server_connected = NULL;

    // If the 'axis_transport_connect' directly returns error, it could be due to
    // invalid parameters or other errors which cannot be solved by retrying. So
    // we close the timer here.
    axis_timer_stop_async(protocol->retry_timer);
    axis_timer_close_async(protocol->retry_timer);
  }
}

static void axis_protocol_integrated_on_retry_timer_closed(axis_timer_t *timer,
                                                          void *user_data) {
  axis_ASSERT(timer, "Should not happen.");

  axis_protocol_integrated_connect_to_context_t *connect_to_context = user_data;
  axis_ASSERT(connect_to_context, "Should not happen.");

  axis_protocol_integrated_t *protocol = connect_to_context->protocol;
  axis_ASSERT(protocol && axis_protocol_check_integrity(&protocol->base, true),
             "Should not happen.");

  if (connect_to_context->on_server_connected) {
    axis_LOGD(
        "Retry timer is closed, but the connection to %s is not established "
        "yet",
        axis_string_get_raw_str(&connect_to_context->server_uri));
    axis_protocol_integrated_on_server_finally_connected(connect_to_context,
                                                        false);
  } else {
    axis_protocol_integrated_connect_to_context_destroy(connect_to_context);
  }

  protocol->retry_timer = NULL;

  if (axis_protocol_is_closing(&protocol->base)) {
    axis_protocol_integrated_on_close(protocol);
  }
}

static void axis_transport_on_server_connected(axis_transport_t *transport,
                                              axis_stream_t *stream,
                                              int status) {
  axis_protocol_integrated_t *protocol = transport->user_data;

  // Since the transport is created with the runloop of the engine, it is
  // currently in the engine thread.
  axis_ASSERT(protocol && axis_protocol_check_integrity(&protocol->base, true),
             "Should not happen.");
  axis_ASSERT(axis_protocol_role_is_communication(&protocol->base),
             "Should not happen.");

  axis_ASSERT(!protocol->retry_timer, "Should not happen.");

  axis_protocol_integrated_connect_to_context_t *cb_data =
      transport->on_server_connected_data;
  axis_ASSERT(cb_data, "Should not happen.");
  axis_ASSERT(cb_data->on_server_connected, "Should not happen.");

  if (axis_protocol_is_closing(&protocol->base)) {
    axis_stream_close(stream);

    axis_protocol_integrated_on_server_finally_connected(cb_data, false);
    return;
  }

  bool success = status >= 0;

  if (success) {
    axis_protocol_integrated_on_server_finally_connected(cb_data, success);

    axis_protocol_integrated_set_stream(protocol, stream);
    axis_stream_start_read(stream);
  } else {
    axis_stream_close(stream);

    bool need_retry =
        protocol->retry_config.enable && protocol->retry_config.max_retries > 0;

    if (!need_retry) {
      axis_protocol_integrated_on_server_finally_connected(cb_data, success);
      return;
    }

    axis_runloop_t *loop = axis_protocol_get_attached_runloop(&protocol->base);
    axis_ASSERT(loop, "Should not happen.");

    axis_timer_t *timer = axis_timer_create(
        loop, (uint64_t)protocol->retry_config.interval_ms * 1000,
        (int32_t)protocol->retry_config.max_retries, false);
    axis_ASSERT(timer, "Should not happen.");

    protocol->retry_timer = timer;

    // Note that the ownership of the 'cb_data' is transferred to the timer.
    // The 'cb_data' will be freed when the timer is closed.
    axis_timer_set_on_triggered(
        timer, axis_protocol_integrated_on_retry_timer_triggered, cb_data);
    axis_timer_set_on_closed(
        timer, axis_protocol_integrated_on_retry_timer_closed, cb_data);

    axis_timer_enable(timer);
  }
}

static void axis_protocol_integrated_connect_to(
    axis_protocol_t *self_, const char *uri,
    axis_protocol_on_server_connected_func_t on_server_connected) {
  axis_protocol_integrated_t *self = (axis_protocol_integrated_t *)self_;
  axis_ASSERT(self && axis_protocol_check_integrity(&self->base, true),
             "Should not happen.");
  axis_ASSERT(uri, "Should not happen.");
  axis_ASSERT(
      axis_protocol_attach_to(&self->base) == axis_PROTOCOL_ATTACH_TO_CONNECTION,
      "Should not happen.");

  // For integrated protocols, this function must be called in the engine
  // thread.
  axis_ASSERT(
      axis_engine_check_integrity(
          self->base.attached_target.connection->attached_target.remote->engine,
          true),
      "Should not happen.");
  axis_ASSERT(!self->retry_timer, "Should not happen.");

  axis_runloop_t *loop = axis_remote_get_attached_runloop(
      self->base.attached_target.connection->attached_target.remote);
  axis_ASSERT(loop, "Should not happen.");

  axis_string_t *transport_uri = axis_protocol_uri_to_transport_uri(uri);
  axis_ASSERT(transport_uri, "Should not happen.");

  // Note that if connection fails, the transport needs to be closed.
  axis_transport_t *transport = axis_transport_create(loop);
  transport->user_data = self;
  transport->on_server_connected = axis_transport_on_server_connected;

  // The 'connect_to_server_context' will be freed once the
  // 'on_server_connected' callback is called.
  axis_protocol_integrated_connect_to_context_t *connect_to_server_context =
      axis_protocol_integrated_connect_to_context_create(
          self, axis_string_get_raw_str(transport_uri), on_server_connected,
          NULL);
  transport->on_server_connected_data = connect_to_server_context;

  int rc = axis_transport_connect(transport, transport_uri);
  axis_string_destroy(transport_uri);

  if (rc) {
    axis_LOGW("Failed to connect to %s", axis_string_get_raw_str(transport_uri));
    // If the 'axis_transport_connect' directly returns error, it could be due to
    // invalid parameters or other errors which cannot be solved by retrying. So
    // we don't need to retry here.
    axis_protocol_integrated_on_server_finally_connected(
        connect_to_server_context, false);

    axis_transport_close(transport);
  }
}

static void axis_protocol_integrated_on_stream_cleaned(
    axis_protocol_integrated_t *self) {
  axis_ASSERT(self && axis_protocol_check_integrity(&self->base, true),
             "We are in the app thread now.");

  // Integrated protocol has been cleaned, call on_cleaned_for_internal callback
  // now.
  self->base.on_cleaned_for_internal(&self->base);
}

static void axis_protocol_integrated_clean(axis_protocol_integrated_t *self) {
  axis_ASSERT(self && axis_protocol_check_integrity(&self->base, true),
             "Should not happen.");

  // Integrated protocol needs to _clean_ the containing stream. Because the
  // containing stream will be recreated in the engine thread again.

  axis_stream_set_on_closed(self->role_facility.communication_stream,
                           axis_protocol_integrated_on_stream_cleaned, self);
  axis_stream_close(self->role_facility.communication_stream);
}

static void axis_stream_migrated(axis_stream_t *stream, void **user_data) {
  axis_engine_t *engine = user_data[0];
  axis_ASSERT(engine && axis_engine_check_integrity(engine, true),
             "The 'stream' has already been migrated to the engine thread. "
             "Therefore, this function is called in the engine thread.");

  axis_connection_t *connection = user_data[1];
  // The connection is created in the app thread, and _before_ the cleaning is
  // completed, the connection should still belongs to the app thread, and only
  // until the cleaning is completed, we can transfer the connection to the
  // target thread (e.g.: the engine thread or the client thread). Because this
  // function is called in the engine thread, so we can not perform thread
  // checking here, and need to be careful to consider thread safety when access
  // the connection instance before the cleaning is completed.
  axis_ASSERT(connection &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 axis_connection_check_integrity(connection, false),
             "Invalid argument.");

  axis_protocol_integrated_t *protocol =
      (axis_protocol_integrated_t *)connection->protocol;
  axis_ASSERT(
      protocol &&
          // axis_NOLINTNEXTLINE(thread-check)
          axis_protocol_check_integrity(&protocol->base, false),
      "The reason is the same as the above comments about 'connection'.");

  axis_shared_ptr_t *cmd = user_data[2];
  axis_ASSERT(cmd && axis_cmd_base_check_integrity(cmd), "Should not happen.");

  axis_FREE(user_data);

  if (stream) {
    // The 'connection' belongs to the app thread, so we can not call
    // axis_connection_clean() here directly, we have to switch to the app thread
    // to do it.
    axis_app_clean_connection_async(engine->app, connection);

    axis_engine_on_connection_cleaned(engine, connection, cmd);
    axis_shared_ptr_destroy(cmd);

    // The clean up of the connection is done, it's time to create new stream
    // and async which are bound to the engine event loop to the connection.
    axis_protocol_integrated_set_stream(protocol, stream);

    // Re-start to read from this stream when we have set it up correctly with
    // the correct newly created eventloop.
    axis_stream_start_read(stream);
  } else {
    axis_ASSERT(0, "Failed to migrate protocol.");
  }
}

/**
 * @brief Migrate 'protocol' from the app thread to the engine thread.
 * 'integrated' protocol needs this because the containing 'stream' needs to be
 * transferred from the app thread to the engine thread.
 */
static void axis_protocol_integrated_migrate(axis_protocol_integrated_t *self,
                                            axis_engine_t *engine,
                                            axis_connection_t *connection,
                                            axis_shared_ptr_t *cmd) {
  axis_ASSERT(self && axis_protocol_check_integrity(&self->base, true),
             "Should not happen.");
  axis_ASSERT(engine &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 axis_engine_check_integrity(engine, false),
             "The function is called in the app thread, and will migrate the "
             "protocol to the engine thread.");
  axis_ASSERT(engine->app && axis_app_check_integrity(engine->app, true),
             "The function is called in the app thread, and will migrate the "
             "protocol to the engine thread.");
  axis_ASSERT(connection && axis_connection_check_integrity(connection, true),
             "'connection' belongs to app thread now.");
  axis_ASSERT(cmd && axis_cmd_base_check_integrity(cmd), "Should not happen.");

  axis_stream_t *stream = self->role_facility.communication_stream;
  axis_ASSERT(stream && axis_stream_check_integrity(stream),
             "Should not happen.");

  // Stop reading from the stream _before_ migration.
  axis_stream_stop_read(stream);

  void **user_data = (void **)axis_MALLOC(3 * sizeof(void *));
  axis_ASSERT(user_data, "Failed to allocate memory.");

  user_data[0] = engine;
  user_data[1] = connection;
  user_data[2] = axis_shared_ptr_clone(cmd);

  axis_stream_migrate(stream, axis_runloop_current(), engine->loop, user_data,
                     axis_stream_migrated);
}

static void axis_protocol_integrated_on_base_protocol_cleaned(
    axis_protocol_t *self, axis_UNUSED bool is_migration_state_reset) {
  axis_ASSERT(self && axis_protocol_check_integrity(self, true),
             "Should not happen.");

  // The integrated protocol determines the closing event based on the message
  // size read from the stream, and the stream will be frozen during the
  // migration. So there is no closing event after the migration is completed
  // here.

  axis_list_t msgs = axis_LIST_INIT_VAL;
  axis_DO_WITH_MUTEX_LOCK(self->in_lock,
                         { axis_list_swap(&msgs, &self->in_msgs); });

  if (axis_list_is_empty(&msgs)) {
    return;
  }

  axis_protocol_on_inputs_based_on_migration_state(self, &msgs);
  axis_list_clear(&msgs);
}

void axis_protocol_integrated_init(
    axis_protocol_integrated_t *self, const char *name,
    axis_protocol_integrated_on_input_func_t on_input,
    axis_protocol_integrated_on_output_func_t on_output) {
  axis_ASSERT(self && name, "Should not happen.");

  axis_protocol_init(
      &self->base, name,
      (axis_protocol_close_func_t)axis_protocol_integrated_close,
      (axis_protocol_on_output_func_t)axis_protocol_integrated_on_output_async,
      axis_protocol_integrated_listen, axis_protocol_integrated_connect_to,
      (axis_protocol_migrate_func_t)axis_protocol_integrated_migrate,
      (axis_protocol_clean_func_t)axis_protocol_integrated_clean);

  self->base.role = axis_PROTOCOL_ROLE_INVALID;
  self->base.on_cleaned_for_external =
      axis_protocol_integrated_on_base_protocol_cleaned;

  self->role_facility.communication_stream = NULL;
  self->role_facility.listening_transport = NULL;

  self->on_input = on_input;
  self->on_output = on_output;
  axis_protocol_integrated_retry_config_init(&self->retry_config);
  self->retry_timer = NULL;
}

axis_protocol_integrated_connect_to_context_t *
axis_protocol_integrated_connect_to_context_create(
    axis_protocol_integrated_t *self, const char *server_uri,
    axis_protocol_on_server_connected_func_t on_server_connected,
    void *user_data) {
  axis_ASSERT(server_uri, "Invalid argument.");
  axis_ASSERT(on_server_connected, "Invalid argument.");

  axis_protocol_integrated_connect_to_context_t *context =
      (axis_protocol_integrated_connect_to_context_t *)axis_MALLOC(
          sizeof(axis_protocol_integrated_connect_to_context_t));
  axis_ASSERT(context, "Failed to allocate memory.");

  axis_string_init_from_c_str(&context->server_uri, server_uri,
                             strlen(server_uri));
  context->on_server_connected = on_server_connected;
  context->user_data = user_data;
  context->protocol = self;

  return context;
}

void axis_protocol_integrated_connect_to_context_destroy(
    axis_protocol_integrated_connect_to_context_t *context) {
  axis_ASSERT(context, "Invalid argument.");
  // Ensure the callback has been called and reset to NULL.
  axis_ASSERT(!context->on_server_connected, "Invalid argument.");

  axis_string_deinit(&context->server_uri);
  axis_FREE(context);
}
