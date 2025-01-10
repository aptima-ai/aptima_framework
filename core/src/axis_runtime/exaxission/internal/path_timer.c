//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension/path_timer.h"

#include "include_internal/axis_runtime/extension/close.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension/msg_handling.h"
#include "include_internal/axis_runtime/extension_thread/extension_thread.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/axis_runtime/path/path.h"
#include "include_internal/axis_runtime/path/path_table.h"
#include "include_internal/axis_runtime/timer/timer.h"
#include "axis_runtime/msg/cmd_result/cmd_result.h"
#include "axis_runtime/timer/timer.h"
#include "axis_utils/container/list.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/lib/time.h"
#include "axis_utils/log/log.h"

static void axis_extension_in_path_timer_on_triggered(axis_timer_t *self,
                                                     void *on_trigger_data) {
  axis_extension_t *extension = (axis_extension_t *)on_trigger_data;
  axis_ASSERT(extension && axis_extension_check_integrity(extension, true) &&
                 self && axis_timer_check_integrity(self, true),
             "Should not happen.");

  axis_path_table_t *path_table = extension->path_table;
  axis_ASSERT(path_table, "Should not happen.");

  axis_list_t *in_paths = &path_table->in_paths;
  axis_ASSERT(in_paths && axis_list_check_integrity(in_paths),
             "Should not happen.");

  int64_t current_time_us = axis_current_time_us();

  // Remove all the expired paths in the IN path table.
  axis_list_foreach (in_paths, iter) {
    axis_path_t *path = (axis_path_t *)axis_ptr_listnode_get(iter.node);
    axis_ASSERT(path && axis_path_check_integrity(path, true),
               "Should not happen.");

    if (current_time_us >= path->expired_time_us) {
      axis_list_remove_node(in_paths, iter.node);
    }
  }
}

static void axis_extension_out_path_timer_on_triggered(axis_timer_t *self,
                                                      void *on_trigger_data) {
  axis_extension_t *extension = (axis_extension_t *)on_trigger_data;
  axis_ASSERT(extension && axis_extension_check_integrity(extension, true) &&
                 self && axis_timer_check_integrity(self, true),
             "Should not happen.");

  axis_path_table_t *path_table = extension->path_table;
  axis_ASSERT(path_table, "Should not happen.");

  axis_list_t *out_paths = &path_table->out_paths;
  axis_ASSERT(out_paths && axis_list_check_integrity(out_paths),
             "Should not happen.");

  int64_t current_time_us = axis_current_time_us();

  // Create a fake error result for those timed-out commands and send it back to
  // the extension.
  axis_list_t timeout_cmd_result_list = axis_LIST_INIT_VAL;
  axis_list_foreach (out_paths, iter) {
    axis_path_t *path = (axis_path_t *)axis_ptr_listnode_get(iter.node);
    axis_ASSERT(path && axis_path_check_integrity(path, true),
               "Should not happen.");

    if (current_time_us >= path->expired_time_us) {
      axis_shared_ptr_t *cmd_result =
          axis_cmd_result_create(axis_STATUS_CODE_ERROR);
      axis_ASSERT(cmd_result && axis_cmd_base_check_integrity(cmd_result),
                 "Should not happen.");

      axis_msg_set_property(cmd_result, "detail",
                           axis_value_create_string("Path timeout."), NULL);
      axis_cmd_base_set_cmd_id(cmd_result,
                              axis_string_get_raw_str(&path->cmd_id));
      axis_list_push_smart_ptr_back(&timeout_cmd_result_list, cmd_result);
      axis_shared_ptr_destroy(cmd_result);
    }
  }

  if (!axis_list_is_empty(&timeout_cmd_result_list)) {
    axis_LOGE("[%s] %zu paths timeout.", axis_extension_get_name(extension, true),
             axis_list_size(&timeout_cmd_result_list));
  }

  axis_list_foreach (&timeout_cmd_result_list, iter) {
    axis_shared_ptr_t *cmd_result = axis_smart_ptr_listnode_get(iter.node);
    axis_ASSERT(cmd_result && axis_cmd_base_check_integrity(cmd_result),
               "Should not happen.");

    axis_extension_handle_in_msg(extension, cmd_result);
  }

  axis_list_clear(&timeout_cmd_result_list);
}

axis_timer_t *axis_extension_create_timer_for_in_path(axis_extension_t *self) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Should not happen.");

  axis_extension_thread_t *extension_thread = self->extension_thread;
  axis_ASSERT(extension_thread &&
                 axis_extension_thread_check_integrity(extension_thread, true),
             "Should not happen.");

  axis_timer_t *timer = axis_timer_create(
      axis_extension_thread_get_attached_runloop(extension_thread),
      self->path_timeout_info.check_interval, axis_TIMER_INFINITE, true);

  axis_timer_set_on_triggered(timer, axis_extension_in_path_timer_on_triggered,
                             self);
  axis_timer_set_on_closed(timer, axis_extension_on_timer_closed, self);

  return timer;
}

axis_timer_t *axis_extension_create_timer_for_out_path(axis_extension_t *self) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Should not happen.");

  axis_extension_thread_t *extension_thread = self->extension_thread;
  axis_ASSERT(extension_thread &&
                 axis_extension_thread_check_integrity(extension_thread, true),
             "Should not happen.");

  axis_timer_t *timer = axis_timer_create(
      axis_extension_thread_get_attached_runloop(extension_thread),
      self->path_timeout_info.check_interval, axis_TIMER_INFINITE, true);

  axis_timer_set_on_triggered(timer, axis_extension_out_path_timer_on_triggered,
                             self);
  axis_timer_set_on_closed(timer, axis_extension_on_timer_closed, self);

  return timer;
}
