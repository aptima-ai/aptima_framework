//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension/close.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension/msg_handling.h"
#include "include_internal/axis_runtime/extension_thread/extension_thread.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/axis_runtime/path/path.h"
#include "include_internal/axis_runtime/path/path_table.h"
#include "include_internal/axis_runtime/timer/timer.h"
#include "axis_runtime/msg/cmd_result/cmd_result.h"
#include "axis_utils/log/log.h"

static bool axis_extension_could_be_closed(axis_extension_t *self) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Should not happen.");

  // Check if all the path timers are closed.
  return axis_list_is_empty(&self->path_timers);
}

static void axis_extension_thread_process_remaining_paths(
    axis_extension_t *extension) {
  axis_ASSERT(extension && axis_extension_check_integrity(extension, true),
             "Should not happen.");

  axis_path_table_t *path_table = extension->path_table;
  axis_ASSERT(path_table, "Should not happen.");

  axis_list_t *in_paths = &path_table->in_paths;
  axis_ASSERT(in_paths && axis_list_check_integrity(in_paths),
             "Should not happen.");

  // Clear the _IN_ paths of the extension.
  axis_list_clear(in_paths);

  axis_list_t *out_paths = &path_table->out_paths;
  axis_ASSERT(out_paths && axis_list_check_integrity(out_paths),
             "Should not happen.");

  size_t out_paths_cnt = axis_list_size(out_paths);
  if (out_paths_cnt) {
    // Call axis_extension_handle_in_msg to consume cmd results, so that the
    // _OUT_paths can be removed.
    axis_LOGD("[%s] Flushing %zu remaining out paths.",
             axis_extension_get_name(extension, true), out_paths_cnt);

    axis_list_t cmd_result_list = axis_LIST_INIT_VAL;

    // Generate an error result for each remaining out path.
    axis_list_foreach (out_paths, iter) {
      axis_path_t *path = (axis_path_t *)axis_ptr_listnode_get(iter.node);
      axis_ASSERT(path && axis_path_check_integrity(path, true),
                 "Should not happen.");

      axis_shared_ptr_t *cmd_result =
          axis_cmd_result_create(axis_STATUS_CODE_ERROR);
      axis_ASSERT(cmd_result && axis_cmd_base_check_integrity(cmd_result),
                 "Should not happen.");

      axis_msg_set_property(
          cmd_result, "detail",
          axis_value_create_string(axis_string_get_raw_str(&path->cmd_id)), NULL);
      axis_cmd_base_set_cmd_id(cmd_result,
                              axis_string_get_raw_str(&path->cmd_id));
      axis_list_push_smart_ptr_back(&cmd_result_list, cmd_result);
      axis_shared_ptr_destroy(cmd_result);
    }

    // Send these newly generated error results to the extension.
    axis_list_foreach (&cmd_result_list, iter) {
      axis_shared_ptr_t *cmd_result = axis_smart_ptr_listnode_get(iter.node);
      axis_ASSERT(cmd_result && axis_cmd_base_check_integrity(cmd_result),
                 "Should not happen.");

      axis_extension_handle_in_msg(extension, cmd_result);
    }

    axis_list_clear(&cmd_result_list);
  }
}

// After all the path timers are closed, the closing flow could be proceed.
static void axis_extension_do_close(axis_extension_t *self) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Should not happen.");

  axis_extension_thread_t *extension_thread = self->extension_thread;
  axis_ASSERT(extension_thread &&
                 axis_extension_thread_check_integrity(extension_thread, true),
             "Should not happen.");

  // Important: All the registered result handlers have to be called.
  //
  // Ex: If there are still some _IN_ or _OUT_ paths remaining in the path table
  // of extensions, in order to prevent memory leaks such as the result handler
  // in C++ binding, we need to create the corresponding cmd results and send
  // them into the original source extension.
  axis_extension_thread_process_remaining_paths(self);

  axis_extension_on_deinit(self);
}

void axis_extension_on_timer_closed(axis_timer_t *timer, void *on_closed_data) {
  axis_ASSERT(timer && axis_timer_check_integrity(timer, true),
             "Should not happen.");

  axis_extension_t *extension = (axis_extension_t *)on_closed_data;
  axis_ASSERT(extension && axis_extension_check_integrity(extension, true),
             "Should not happen.");

  axis_list_remove_ptr(&extension->path_timers, timer);

  if (axis_extension_could_be_closed(extension)) {
    axis_extension_do_close(extension);
  }
}

void axis_extension_do_pre_close_action(axis_extension_t *self) {
  axis_ASSERT(self, "Should not happen.");
  axis_ASSERT(axis_extension_check_integrity(self, true), "Should not happen.");
  axis_ASSERT(self->extension_thread, "Should not happen.");

  // Close the timers of the path tables.
  axis_list_foreach (&self->path_timers, iter) {
    axis_timer_t *timer = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(timer, "Should not happen.");

    axis_timer_stop_async(timer);
    axis_timer_close_async(timer);
  }

  if (axis_extension_could_be_closed(self)) {
    axis_extension_do_close(self);
  }
}
