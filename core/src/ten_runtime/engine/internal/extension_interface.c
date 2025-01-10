//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/engine/internal/extension_interface.h"

#include <stdbool.h>

#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/common/loc.h"
#include "include_internal/ten_runtime/engine/engine.h"
#include "include_internal/ten_runtime/engine/internal/remote_interface.h"
#include "include_internal/ten_runtime/engine/msg_interface/common.h"
#include "include_internal/ten_runtime/engine/msg_interface/start_graph.h"
#include "include_internal/ten_runtime/extension_context/extension_context.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_node.h"
#include "ten_utils/io/runloop.h"
#include "ten_utils/lib/mutex.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

// The 'cmd' parameter is the command triggers the enabling of extension system.
// If there is no command to trigger the enabling, this parameter would be NULL.
//
// NOLINTNEXTLINE(misc-no-recursion)
bool ten_engine_enable_extension_system(ten_engine_t *self,
                                        ten_shared_ptr_t *cmd,
                                        ten_error_t *err) {
  TEN_ASSERT(self && ten_engine_check_integrity(self, true),
             "Should not happen.");
  TEN_ASSERT(cmd && ten_msg_get_type(cmd) == TEN_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  if (ten_engine_is_closing(self)) {
    TEN_LOGE("Engine is closing, do not enable extension system.");

    ten_engine_return_error_for_cmd_start_graph(
        self, cmd, "Failed to start extension system: %s",
        ten_error_errmsg(err));

    return false;
  }

  if (self->extension_context) {
    // The engine has already started a extension execution context, so
    // returning OK directly.
    ten_engine_return_ok_for_cmd_start_graph(self, cmd);
  } else {
    self->extension_context = ten_extension_context_create(self);
    ten_extension_context_set_on_closed(
        self->extension_context, ten_engine_on_extension_context_closed, self);

    if (!ten_extension_context_start_extension_group(self->extension_context,
                                                     cmd, err)) {
      TEN_LOGE(
          "Failed to correctly handle the 'start_graph' command, so stop the "
          "engine.");

      ten_engine_return_error_for_cmd_start_graph(
          self, cmd, "Failed to start extension system: %s",
          ten_error_errmsg(err));

      return false;
    }
  }

  return true;
}

static void ten_engine_on_extension_msgs(ten_engine_t *self) {
  TEN_ASSERT(self && ten_engine_check_integrity(self, true),
             "Should not happen.");

  ten_list_t extension_msgs_ = TEN_LIST_INIT_VAL;

  TEN_UNUSED int rc = ten_mutex_lock(self->extension_msgs_lock);
  TEN_ASSERT(!rc, "Should not happen.");

  ten_list_swap(&extension_msgs_, &self->extension_msgs);

  rc = ten_mutex_unlock(self->extension_msgs_lock);
  TEN_ASSERT(!rc, "Should not happen.");

  ten_list_foreach (&extension_msgs_, iter) {
    ten_shared_ptr_t *msg = ten_smart_ptr_listnode_get(iter.node);
    TEN_ASSERT(msg && ten_msg_get_dest_cnt(msg) == 1,
               "When this function is executed, there should be only one "
               "destination remaining in the message's dest.");

    if (ten_engine_is_closing(self) &&
        !ten_msg_type_to_handle_when_closing(msg)) {
      // Except some special messages, do not handle the message if the engine
      // is closing.
      continue;
    }

    ten_loc_t *dest_loc = ten_msg_get_first_dest_loc(msg);
    TEN_ASSERT(dest_loc && ten_loc_check_integrity(dest_loc),
               "Should not happen.");

    if (!ten_string_is_equal_c_str(&dest_loc->app_uri,
                                   ten_app_get_uri(self->app))) {
      TEN_ASSERT(!ten_string_is_empty(&dest_loc->app_uri),
                 "Should not happen.");

      // Since the engine dynamically adds/removes remotes, when the extension
      // system needs to deliver a message to a remote, it requests the engine
      // to handle it in order to avoid race conditions. Therefore, this is
      // where we check if such a situation is occurring.

      ten_engine_route_msg_to_remote(self, msg);
    } else {
      // Otherwise, enable the engine to handle the message.

      ten_engine_dispatch_msg(self, msg);
    }
  }

  ten_list_clear(&extension_msgs_);
}

static void ten_engine_on_extension_msgs_task(void *engine_,
                                              TEN_UNUSED void *arg) {
  ten_engine_t *engine = (ten_engine_t *)engine_;
  TEN_ASSERT(engine && ten_engine_check_integrity(engine, true),
             "Should not happen.");

  ten_engine_on_extension_msgs(engine);
}

static void ten_engine_on_extension_msgs_async(ten_engine_t *self) {
  TEN_ASSERT(self &&
                 // TEN_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in
                 // different threads.
                 ten_engine_check_integrity(self, false),
             "Should not happen.");

  ten_runloop_post_task_tail(ten_engine_get_attached_runloop(self),
                             ten_engine_on_extension_msgs_task, self, NULL);
}

void ten_engine_push_to_extension_msgs_queue(ten_engine_t *self,
                                             ten_shared_ptr_t *msg) {
  TEN_ASSERT(msg, "Should not happen.");

  // This function is used to be called from threads other than the engine
  // thread.
  TEN_ASSERT(self && ten_engine_check_integrity(self, false),
             "Should not happen.");

  ten_listnode_t *node = ten_smart_ptr_listnode_create(msg);

  TEN_UNUSED int rc = ten_mutex_lock(self->extension_msgs_lock);
  TEN_ASSERT(!rc, "Should not happen.");

  ten_list_push_back(&self->extension_msgs, node);

  rc = ten_mutex_unlock(self->extension_msgs_lock);
  TEN_ASSERT(!rc, "Should not happen.");

  ten_engine_on_extension_msgs_async(self);
}
