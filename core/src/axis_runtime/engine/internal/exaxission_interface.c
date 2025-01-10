//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/engine/internal/extension_interface.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/common/loc.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/engine/internal/remote_interface.h"
#include "include_internal/axis_runtime/engine/msg_interface/common.h"
#include "include_internal/axis_runtime/engine/msg_interface/start_graph.h"
#include "include_internal/axis_runtime/extension_context/extension_context.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

// The 'cmd' parameter is the command triggers the enabling of extension system.
// If there is no command to trigger the enabling, this parameter would be NULL.
//
// NOLINTNEXTLINE(misc-no-recursion)
bool axis_engine_enable_extension_system(axis_engine_t *self,
                                        axis_shared_ptr_t *cmd,
                                        axis_error_t *err) {
  axis_ASSERT(self && axis_engine_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(cmd && axis_msg_get_type(cmd) == axis_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  if (axis_engine_is_closing(self)) {
    axis_LOGE("Engine is closing, do not enable extension system.");

    axis_engine_return_error_for_cmd_start_graph(
        self, cmd, "Failed to start extension system: %s",
        axis_error_errmsg(err));

    return false;
  }

  if (self->extension_context) {
    // The engine has already started a extension execution context, so
    // returning OK directly.
    axis_engine_return_ok_for_cmd_start_graph(self, cmd);
  } else {
    self->extension_context = axis_extension_context_create(self);
    axis_extension_context_set_on_closed(
        self->extension_context, axis_engine_on_extension_context_closed, self);

    if (!axis_extension_context_start_extension_group(self->extension_context,
                                                     cmd, err)) {
      axis_LOGE(
          "Failed to correctly handle the 'start_graph' command, so stop the "
          "engine.");

      axis_engine_return_error_for_cmd_start_graph(
          self, cmd, "Failed to start extension system: %s",
          axis_error_errmsg(err));

      return false;
    }
  }

  return true;
}

static void axis_engine_on_extension_msgs(axis_engine_t *self) {
  axis_ASSERT(self && axis_engine_check_integrity(self, true),
             "Should not happen.");

  axis_list_t extension_msgs_ = axis_LIST_INIT_VAL;

  axis_UNUSED int rc = axis_mutex_lock(self->extension_msgs_lock);
  axis_ASSERT(!rc, "Should not happen.");

  axis_list_swap(&extension_msgs_, &self->extension_msgs);

  rc = axis_mutex_unlock(self->extension_msgs_lock);
  axis_ASSERT(!rc, "Should not happen.");

  axis_list_foreach (&extension_msgs_, iter) {
    axis_shared_ptr_t *msg = axis_smart_ptr_listnode_get(iter.node);
    axis_ASSERT(msg && axis_msg_get_dest_cnt(msg) == 1,
               "When this function is executed, there should be only one "
               "destination remaining in the message's dest.");

    if (axis_engine_is_closing(self) &&
        !axis_msg_type_to_handle_when_closing(msg)) {
      // Except some special messages, do not handle the message if the engine
      // is closing.
      continue;
    }

    axis_loc_t *dest_loc = axis_msg_get_first_dest_loc(msg);
    axis_ASSERT(dest_loc && axis_loc_check_integrity(dest_loc),
               "Should not happen.");

    if (!axis_string_is_equal_c_str(&dest_loc->app_uri,
                                   axis_app_get_uri(self->app))) {
      axis_ASSERT(!axis_string_is_empty(&dest_loc->app_uri),
                 "Should not happen.");

      // Since the engine dynamically adds/removes remotes, when the extension
      // system needs to deliver a message to a remote, it requests the engine
      // to handle it in order to avoid race conditions. Therefore, this is
      // where we check if such a situation is occurring.

      axis_engine_route_msg_to_remote(self, msg);
    } else {
      // Otherwise, enable the engine to handle the message.

      axis_engine_dispatch_msg(self, msg);
    }
  }

  axis_list_clear(&extension_msgs_);
}

static void axis_engine_on_extension_msgs_task(void *engine_,
                                              axis_UNUSED void *arg) {
  axis_engine_t *engine = (axis_engine_t *)engine_;
  axis_ASSERT(engine && axis_engine_check_integrity(engine, true),
             "Should not happen.");

  axis_engine_on_extension_msgs(engine);
}

static void axis_engine_on_extension_msgs_async(axis_engine_t *self) {
  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in
                 // different threads.
                 axis_engine_check_integrity(self, false),
             "Should not happen.");

  axis_runloop_post_task_tail(axis_engine_get_attached_runloop(self),
                             axis_engine_on_extension_msgs_task, self, NULL);
}

void axis_engine_push_to_extension_msgs_queue(axis_engine_t *self,
                                             axis_shared_ptr_t *msg) {
  axis_ASSERT(msg, "Should not happen.");

  // This function is used to be called from threads other than the engine
  // thread.
  axis_ASSERT(self && axis_engine_check_integrity(self, false),
             "Should not happen.");

  axis_listnode_t *node = axis_smart_ptr_listnode_create(msg);

  axis_UNUSED int rc = axis_mutex_lock(self->extension_msgs_lock);
  axis_ASSERT(!rc, "Should not happen.");

  axis_list_push_back(&self->extension_msgs, node);

  rc = axis_mutex_unlock(self->extension_msgs_lock);
  axis_ASSERT(!rc, "Should not happen.");

  axis_engine_on_extension_msgs_async(self);
}
