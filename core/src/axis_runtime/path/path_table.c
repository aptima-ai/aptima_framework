//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/path/path_table.h"

#include <inttypes.h>

#include "include_internal/axis_runtime/common/loc.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension_context/extension_context.h"
#include "include_internal/axis_runtime/extension_thread/extension_thread.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd_result/cmd.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/msg_conversion/msg_conversion/base.h"
#include "include_internal/axis_runtime/path/common.h"
#include "include_internal/axis_runtime/path/path.h"
#include "include_internal/axis_runtime/path/path_group.h"
#include "include_internal/axis_runtime/path/path_in.h"
#include "include_internal/axis_runtime/path/path_out.h"
#include "axis_runtime/msg/cmd_result/cmd_result.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/lib/time.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/sanitizer/thread_check.h"

#define PATH_TABLE_REASONABLE_MAX_CNT 1000

static bool axis_path_table_check_integrity(axis_path_table_t *self,
                                           bool check_thread) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) != axis_PATH_TABLE_SIGNATURE) {
    return false;
  }

  if (check_thread) {
    if (self->attach_to == axis_PATH_TABLE_ATTACH_TO_EXTENSION) {
      axis_extension_thread_t *extension_thread =
          self->attached_target.extension->extension_thread;
      return axis_extension_thread_check_integrity(extension_thread, true);
    }

    return axis_sanitizer_thread_check_do_check(&self->thread_check);
  }

  return true;
}

axis_path_table_t *axis_path_table_create(axis_PATH_TABLE_ATTACH_TO attach_to,
                                        void *attached_target) {
  axis_path_table_t *self =
      (axis_path_table_t *)axis_MALLOC(sizeof(axis_path_table_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_signature_set(&self->signature, axis_PATH_TABLE_SIGNATURE);

  axis_list_init(&self->in_paths);
  axis_list_init(&self->out_paths);

  self->attach_to = attach_to;
  switch (attach_to) {
    case axis_PATH_TABLE_ATTACH_TO_ENGINE:
      self->attached_target.engine = attached_target;
      break;
    case axis_PATH_TABLE_ATTACH_TO_EXTENSION:
      self->attached_target.extension = attached_target;
      break;
    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  axis_sanitizer_thread_check_init_with_current_thread(&self->thread_check);

  return self;
}

void axis_path_table_destroy(axis_path_table_t *self) {
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function would be called from the engine thread, so we
  // do not check the execution thread context of the path table.
  axis_ASSERT(self && axis_path_table_check_integrity(self, false),
             "Should not happen.");

  axis_signature_set(&self->signature, 0);

  axis_list_clear(&self->in_paths);
  axis_list_clear(&self->out_paths);

  axis_sanitizer_thread_check_deinit(&self->thread_check);

  axis_FREE(self);
}

void axis_path_table_check_empty(axis_path_table_t *self) {
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function would be called when extensions are destroying,
  // So at that time, the extension thread had already been deleted. Therefore,
  // this function is invoked within the engine thread and is safe.
  axis_ASSERT(self && axis_path_table_check_integrity(self, false),
             "Should not happen.");

  axis_ASSERT(axis_list_is_empty(&self->in_paths), "There should be no IN path.");
  axis_ASSERT(axis_list_is_empty(&self->out_paths),
             "There should be no OUT path.");
}

axis_listnode_t *axis_path_table_find_path_from_cmd_id(axis_path_table_t *self,
                                                     axis_PATH_TYPE type,
                                                     const char *cmd_id) {
  axis_ASSERT(self && axis_path_table_check_integrity(self, true),
             "Should not happen.");

  axis_list_t *list = type == axis_PATH_IN ? &self->in_paths : &self->out_paths;

  size_t path_cnt = axis_list_size(list);
  if (path_cnt > PATH_TABLE_REASONABLE_MAX_CNT) {
    axis_LOGE("Too many paths, there might be some issues.");
  }

  axis_list_foreach (list, paths_iter) {
    axis_path_t *path = (axis_path_t *)axis_ptr_listnode_get(paths_iter.node);
    axis_ASSERT(path && axis_path_check_integrity(path, true),
               "Should not happen.");

    if ((cmd_id ? axis_string_is_equal_c_str(&path->cmd_id, cmd_id) : true)) {
      return paths_iter.node;
    }
  }

  return NULL;
}

static uint64_t get_expired_time(uint64_t timeout_duration) {
  int64_t current_time_us_ = axis_current_time_us();
  axis_ASSERT(current_time_us_ >= 0, "Should not happen. %" PRId64,
             current_time_us_);

  uint64_t current_time_us = current_time_us_;
  if (timeout_duration > UINT64_MAX - current_time_us) {
    return UINT64_MAX;
  }

  return current_time_us + timeout_duration;
}

static uint64_t axis_path_table_get_path_timeout_duration(
    axis_path_table_t *self, axis_PATH_TYPE path_type) {
  axis_ASSERT(self && axis_path_table_check_integrity(self, true),
             "Invalid argument.");

  uint64_t timeout_time = UINT64_MAX;

  if (self->attach_to == axis_PATH_TABLE_ATTACH_TO_EXTENSION) {
    axis_extension_t *extension = self->attached_target.extension;
    axis_ASSERT(extension && axis_extension_check_integrity(extension, true),
               "Invalid argument.");

    // Try to get general path timeout time in us from the extension.
    switch (path_type) {
      case axis_PATH_IN:
        timeout_time = extension->path_timeout_info.in_path_timeout;
        break;
      case axis_PATH_OUT:
        timeout_time = extension->path_timeout_info.out_path_timeout;
        break;
      default:
        break;
    }
  }

  return timeout_time;
}

/**
 * --> Extension
 *  ^
 *
 * TEN records this kind of path to determine where the messages (ex: the status
 * commands) should go when they follow the backward path.
 */
axis_path_in_t *axis_path_table_add_in_path(
    axis_path_table_t *self, axis_shared_ptr_t *cmd,
    axis_msg_conversion_t *result_conversion) {
  axis_ASSERT(self && axis_path_table_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(cmd && axis_cmd_base_check_integrity(cmd) &&
                 axis_msg_get_dest_cnt(cmd) == 1,
             "Should not happen.");

  axis_listnode_t *old_node = axis_path_table_find_path_from_cmd_id(
      self, axis_PATH_IN, axis_cmd_base_get_cmd_id(cmd));
  if (old_node) {
    // The presence of the command's path in the path table indicates a
    // potential circular reference, exemplified by the following structure:
    //
    // A --> B --> C
    //       ^     |
    //       |     V
    //       <---- D
    //
    // Such a circular path scenario necessitates the generation of a unique
    // command ID for this specific command. This approach helps prevent path
    // conflicts and ensures accurate identification of the correct path entry
    // in the table. This is essential for maintaining the integrity of path
    // tracking and avoiding erroneous command executions or data processing.
    if (axis_msg_is_cmd(cmd) && axis_msg_get_type(cmd) == axis_MSG_TYPE_CMD) {
      axis_cmd_base_save_cmd_id_to_parent_cmd_id(cmd);
      axis_cmd_base_gen_new_cmd_id_forcibly(cmd);
      old_node = axis_path_table_find_path_from_cmd_id(
          self, axis_PATH_IN, axis_cmd_base_get_cmd_id(cmd));
    }
  }

  axis_ASSERT(!old_node,
             "There should be no two commands with the same command ID.");

  axis_loc_t src_loc;
  axis_loc_init_from_loc(&src_loc, axis_msg_get_src_loc(cmd));

  axis_loc_t dest_loc;
  axis_loc_init_from_loc(&dest_loc, axis_msg_get_first_dest_loc(cmd));

  axis_path_in_t *in_path = axis_path_in_create(
      self, axis_msg_get_name(cmd), axis_cmd_base_get_parent_cmd_id(cmd),
      axis_cmd_base_get_cmd_id(cmd), &src_loc, &dest_loc, result_conversion);
  axis_ASSERT(in_path && axis_path_check_integrity((axis_path_t *)in_path, true),
             "Invalid argument.");

  uint64_t timeout_duration_us =
      axis_path_table_get_path_timeout_duration(self, axis_PATH_IN);
  axis_path_set_expired_time(&in_path->base,
                            get_expired_time(timeout_duration_us));

  // The parent_cmd_id has been saved to the path entry, so clear it from the
  // command itself.
  axis_cmd_base_reset_parent_cmd_id(cmd);

  axis_list_push_ptr_back(&self->in_paths, in_path,
                         (axis_ptr_listnode_destroy_func_t)axis_path_in_destroy);

  axis_loc_deinit(&src_loc);
  axis_loc_deinit(&dest_loc);

  return in_path;
}

/**
 * Extension -->
 *       ^
 *
 * TEN records this kind of path to enable the cmd result to get some
 * original information, ex: the result handler.
 */
axis_path_out_t *axis_path_table_add_out_path(axis_path_table_t *self,
                                            axis_shared_ptr_t *cmd) {
  axis_ASSERT(self && axis_path_table_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(cmd && axis_cmd_base_check_integrity(cmd) &&
                 axis_msg_get_dest_cnt(cmd) == 1,
             "Should not happen.");

  axis_loc_t src_loc;
  axis_loc_init_from_loc(&src_loc, axis_msg_get_src_loc(cmd));

  axis_loc_t dest_loc;
  axis_loc_init_from_loc(&dest_loc, axis_msg_get_first_dest_loc(cmd));

  // Create a path.
  axis_path_out_t *out_path = axis_path_out_create(
      self, axis_msg_get_name(cmd), axis_cmd_base_get_parent_cmd_id(cmd),
      axis_cmd_base_get_cmd_id(cmd), &src_loc, &dest_loc,
      axis_cmd_base_get_raw_cmd_base(cmd)->result_handler,
      axis_cmd_base_get_raw_cmd_base(cmd)->result_handler_data);
  axis_ASSERT(out_path && axis_path_check_integrity((axis_path_t *)out_path, true),
             "Invalid argument.");

  uint64_t timeout_duration_us =
      axis_path_table_get_path_timeout_duration(self, axis_PATH_OUT);
  axis_path_set_expired_time(&out_path->base,
                            get_expired_time(timeout_duration_us));

  // The parent_cmd_id has been saved to the path entry, so clear it from the
  // command itself.
  axis_cmd_base_reset_parent_cmd_id(cmd);

  // Reset the result handler after saving it into the path.
  axis_cmd_base_set_result_handler(cmd, NULL, NULL);

  // Save the created path to the path table.
  axis_list_push_ptr_back(&self->out_paths, out_path,
                         (axis_ptr_listnode_destroy_func_t)axis_path_out_destroy);

  axis_loc_deinit(&src_loc);
  axis_loc_deinit(&dest_loc);

  return out_path;
}

static axis_listnode_t *axis_path_table_find_path_from_cmd(
    axis_path_table_t *self, axis_PATH_TYPE path_type, axis_shared_ptr_t *cmd) {
  axis_ASSERT(self && axis_path_table_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(cmd && axis_cmd_base_check_integrity(cmd), "Should not happen.");

  axis_listnode_t *old_node = axis_path_table_find_path_from_cmd_id(
      self, path_type, axis_cmd_base_get_cmd_id(cmd));
  if (!old_node) {
    // The path is gone, so discard the command.
    return NULL;
  }

  axis_path_t *old_path = axis_ptr_listnode_get(old_node);
  axis_ASSERT(old_path && axis_path_check_integrity(old_path, true),
             "Should not happen.");

  return old_node;
}

static axis_path_out_t *axis_path_table_find_out_path(axis_path_table_t *self,
                                                    axis_shared_ptr_t *cmd) {
  axis_ASSERT(self && axis_path_table_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(cmd && axis_cmd_base_check_integrity(cmd), "Should not happen.");

  axis_listnode_t *old_node =
      axis_path_table_find_path_from_cmd(self, axis_PATH_OUT, cmd);
  if (!old_node) {
    // The reason for a missing OUT path is because that OUT path was in a path
    // group, and that path group is cleared before because the group condition
    // is met.
    return NULL;
  }

  return axis_ptr_listnode_get(old_node);
}

/**
 * @brief TEN finds this kind of path to determine where the messages (ex: the
 * cmd results) should go when they are returning.
 */
static axis_path_in_t *axis_path_table_find_in_path(axis_path_table_t *self,
                                                  axis_shared_ptr_t *cmd) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_path_table_check_integrity(self, true), "Invalid argument.");
  axis_ASSERT(cmd && axis_cmd_base_check_integrity(cmd), "Invalid argument.");

  // Find the corresponding IN path according to the "command ID".
  axis_listnode_t *old_node =
      axis_path_table_find_path_from_cmd(self, axis_PATH_IN, cmd);
  if (!old_node) {
    // The returned path is disappear, so drop the command.
    return NULL;
  }

  return axis_ptr_listnode_get(old_node);
}

static bool axis_path_table_remove_path_from_group(axis_path_table_t *self,
                                                  axis_PATH_TYPE type,
                                                  axis_path_t *path) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_path_table_check_integrity(self, true), "Invalid argument.");
  axis_ASSERT(path && axis_path_check_integrity(path, true), "Invalid argument.");
  axis_ASSERT(axis_path_is_in_a_group(path), "Invalid argument.");

  // Remove path from group members.
  axis_list_t *group_members = axis_path_group_get_members(path);
  axis_listnode_t *group_member_node = axis_list_find_ptr(group_members, path);
  axis_ASSERT(group_member_node, "Should not happen.");
  axis_list_remove_node(group_members, group_member_node);

  bool last_one = axis_list_size(group_members) == 0;

  // Remove path from the path table.
  axis_list_t *paths = type == axis_PATH_IN ? &self->in_paths : &self->out_paths;
  axis_listnode_t *paths_node = axis_list_find_ptr(paths, path);
  axis_ASSERT(paths_node, "Should not happen.");
  axis_list_remove_node(paths, paths_node);

  return last_one;
}

static void axis_path_table_remove_group_and_all_its_paths(
    axis_path_table_t *self, axis_PATH_TYPE type, axis_path_t *path) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_path_table_check_integrity(self, true), "Invalid argument.");
  axis_ASSERT(path && axis_path_check_integrity(path, true), "Invalid argument.");
  axis_ASSERT(axis_path_is_in_a_group(path), "Invalid argument.");

  axis_list_t *paths = type == axis_PATH_IN ? &self->in_paths : &self->out_paths;

  axis_list_t *group_members = axis_path_group_get_members(path);

  // If all paths in the group have received the cmd result, we should remove
  // the group, and all its contained paths.
  axis_list_foreach (group_members, iter) {
    axis_path_t *group_path = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(group_path && axis_path_check_integrity(group_path, true),
               "Invalid argument.");

    axis_listnode_t *group_path_node = axis_list_find_ptr(paths, group_path);
    axis_ASSERT(group_path_node, "Should not happen.");

    axis_list_remove_node(paths, group_path_node);
  }
}

// Search the specified path table for the corresponding path entry.
static axis_path_t *axis_path_table_find_path(axis_path_table_t *self,
                                            axis_PATH_TYPE path_type,
                                            axis_shared_ptr_t *cmd) {
  axis_ASSERT(self && axis_path_table_check_integrity(self, true),
             "Invalid argument.");
  axis_ASSERT(cmd && axis_msg_check_integrity(cmd), "Invalid argument.");

  switch (path_type) {
    case axis_PATH_IN:
      return (axis_path_t *)axis_path_table_find_in_path(self, cmd);
    case axis_PATH_OUT:
      return (axis_path_t *)axis_path_table_find_out_path(self, cmd);
    default:
      axis_ASSERT(0, "Should not happen.");
      return NULL;
  }
}

// There are 2 cases where a cmd result interacted with an extension.
//
//              (1)                                     (2)
//    ↙-- cmd_result (a path)                 ↙-- cmd_result (a path)
// <-o<-- cmd_result (a path) <- Extension <-o<-- cmd_result (a path)
//    ↖-- cmd_result (a path)                 ↖-- cmd_result (a path)
//
// (1) When a cmd result leaves an extension
//     Multiple cmd results might be related to a single original command due to
//     the command conversion mechanism. Each cmd result would flow through a
//     _IN_ path to the previous node in the graph.
//
// (2) When a cmd result enters an extension
//     Multiple cmd results might be related to a single original command due to
//     the graph 'dests' dispatching mechanism. Each cmd result would flow
//     through a _OUT_ path to the current extension.
//
// The handling of these 2 cases are equal:
//
// a. Save the cmd result to the corresponding path.
//    If there is a result_conversion attached to that path, convert the cmd
//    result according to that rule, and save the generated cmd result to that
//    path instead.
//
// b. If the path does _not_ belong to a path group:
//    (1) transmit the cmd result to the TEN runtime.
//    (2) transmit the cmd result to the extension.
//
// c. Otherwise, if the path _does_ belong to a path group, check if the
//    condition of the path group is met or not:
//    > If yes, decide the resulting cmd result from the cmd results in
//      the path group, and transmit the determined cmd result backward.
//    > If no, do nothing.
//
// Note: This function will be called after the cmd result is linked to the
// corresponding path.
axis_shared_ptr_t *axis_path_table_determine_actual_cmd_result(
    axis_path_table_t *self, axis_PATH_TYPE path_type, axis_path_t *path,
    bool remove_path) {
  axis_ASSERT(self && axis_path_table_check_integrity(self, true),
             "Invalid argument.");
  axis_ASSERT(path_type != axis_PATH_INVALID, "Invalid argument.");
  axis_ASSERT(path && axis_path_check_integrity(path, true), "Invalid argument.");

  if (axis_path_is_in_a_group(path)) {
    path = axis_path_group_resolve(path, path_type);
  }

  if (!path) {
    // The return path has not been decided, so no cmd result needs to be sent
    // to the extension.
    return NULL;
  }

  axis_shared_ptr_t *cmd_result = path->cached_cmd_result;
  axis_ASSERT(cmd_result && axis_cmd_base_check_integrity(cmd_result),
             "Invalid argument.");

  // The `cached_cmd_result` is the only criterion used to determine whether a
  // path has completed its task. Therefore, it is set here to ensure that
  // other validation logic can function properly.
  path->cached_cmd_result = axis_shared_ptr_clone(cmd_result);

  // We need to use the original cmd name to find the schema definition of the
  // cmd result.
  axis_cmd_result_set_original_cmd_name(cmd_result,
                                       axis_string_get_raw_str(&path->cmd_name));

  // The command ID should be reverted to an old one when flows through this
  // path.
  if (!axis_string_is_empty(&path->original_cmd_id)) {
    axis_cmd_base_set_cmd_id(cmd_result,
                            axis_string_get_raw_str(&path->original_cmd_id));
  }

  // Set the dest of the cmd result to the source location of the path.
  axis_msg_clear_and_set_dest_to_loc(cmd_result, &path->src_loc);

  if (path_type == axis_PATH_OUT) {
    // Restore the settings of the result handler, so that the extension
    // could call the result handler for the result.
    axis_cmd_base_set_result_handler(
        cmd_result, ((axis_path_out_t *)path)->result_handler,
        ((axis_path_out_t *)path)->result_handler_data);
  }

  if (axis_path_is_in_a_group(path)) {
    axis_path_group_t *path_group = axis_path_get_group(path);

    switch (path_group->policy) {
      case axis_RESULT_RETURN_POLICY_EACH_OK_AND_ERROR: {
        bool last_one =
            axis_path_table_remove_path_from_group(self, path_type, path);

        axis_cmd_result_set_completed(cmd_result, last_one, NULL);
        break;
      }
      case axis_RESULT_RETURN_POLICY_FIRST_ERROR_OR_FIRST_OK:
      case axis_RESULT_RETURN_POLICY_FIRST_ERROR_OR_LAST_OK:
        // The path group has completed its task, so clean up the path group and
        // all paths it contains.
        axis_path_table_remove_group_and_all_its_paths(self, path_type, path);

        axis_cmd_result_set_completed(cmd_result, true, NULL);
        break;

      default:
        axis_ASSERT(0, "Should not happen.");
        break;
    }
  } else {
    if (remove_path) {
      // This path is not in any group, and we have already decided on the cmd
      // result to send to the extension, so this path can be deleted.
      //
      // Remove the corresponding path from the path table, because the
      // purpose of that path is completed.
      axis_list_remove_ptr(
          path_type == axis_PATH_IN ? &self->in_paths : &self->out_paths, path);
    }

    axis_cmd_result_set_completed(
        cmd_result, axis_cmd_result_is_final(cmd_result, NULL), NULL);
  }

  return cmd_result;
}

axis_path_t *axis_path_table_set_result(axis_path_table_t *self,
                                      axis_PATH_TYPE path_type,
                                      axis_shared_ptr_t *cmd_result) {
  axis_ASSERT(self && axis_path_table_check_integrity(self, true),
             "Invalid argument.");
  axis_ASSERT(cmd_result &&
                 axis_msg_get_type(cmd_result) == axis_MSG_TYPE_CMD_RESULT &&
                 axis_msg_check_integrity(cmd_result),
             "Invalid argument.");

  axis_path_t *path = axis_path_table_find_path(self, path_type, cmd_result);
  if (!path) {
    // The path for the cmd result to return is no longer available.
    return NULL;
  }

  // Associate the cmd result with the corresponding path entry.
  axis_path_set_result(path, cmd_result);

  return path;
}

axis_string_t *axis_path_table_get_graph_id(axis_path_table_t *self) {
  axis_ASSERT(self && axis_path_table_check_integrity(self, true),
             "Invalid argument.");
  if (self->attach_to == axis_PATH_TABLE_ATTACH_TO_EXTENSION) {
    axis_extension_context_t *extension_context =
        self->attached_target.extension->extension_context;
    axis_ASSERT(
        extension_context &&
            // axis_NOLINTNEXTLINE(thread-check)
            // thread-check: We are in the extension thread, and all the uses of
            // the extension context in this function would not cause thread
            // safety issues.
            axis_extension_context_check_integrity(extension_context, false),
        "Invalid argument.");

    axis_engine_t *engine = extension_context->engine;
    axis_ASSERT(
        engine &&
            // axis_NOLINTNEXTLINE(thread-check)
            // thread-check: We are in the extension thread, and all the uses of
            // the engine in this function would not cause thread safety issues.
            axis_engine_check_integrity(engine, false),
        "Invalid argument.");

    return &engine->graph_id;
  } else {
    axis_engine_t *engine = self->attached_target.engine;
    axis_ASSERT(engine && axis_engine_check_integrity(engine, true),
               "Invalid argument.");
    return &engine->graph_id;
  }
}
