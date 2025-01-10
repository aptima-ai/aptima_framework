//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension/extension.h"

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "include_internal/axis_runtime/addon/addon_host.h"
#include "include_internal/axis_runtime/common/loc.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/extension/base_dir.h"
#include "include_internal/axis_runtime/extension/extension_info/extension_info.h"
#include "include_internal/axis_runtime/extension/msg_dest_info/json.h"
#include "include_internal/axis_runtime/extension/msg_dest_info/msg_dest_info.h"
#include "include_internal/axis_runtime/extension/msg_handling.h"
#include "include_internal/axis_runtime/extension/msg_not_connected_cnt.h"
#include "include_internal/axis_runtime/extension/on_xxx.h"
#include "include_internal/axis_runtime/extension_context/extension_context.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/extension_thread/extension_thread.h"
#include "include_internal/axis_runtime/extension_thread/msg_interface/common.h"
#include "include_internal/axis_runtime/metadata/metadata_info.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd_result/cmd.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/path/path.h"
#include "include_internal/axis_runtime/path/path_group.h"
#include "include_internal/axis_runtime/path/path_in.h"
#include "include_internal/axis_runtime/path/path_table.h"
#include "include_internal/axis_runtime/path/result_return_policy.h"
#include "include_internal/axis_runtime/schema_store/store.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "include_internal/axis_utils/log/log.h"
#include "axis_runtime/addon/addon.h"
#include "axis_runtime/app/app.h"
#include "axis_runtime/common/errno.h"
#include "axis_runtime/msg/cmd_result/cmd_result.h"
#include "axis_runtime/msg/msg.h"
#include "axis_runtime/axis_env/internal/on_xxx_done.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node.h"
#include "axis_utils/container/list_node_smart_ptr.h"
#include "axis_utils/container/list_ptr.h"
#include "axis_utils/container/list_smart_ptr.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/ref.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/value/value.h"

axis_extension_t *axis_extension_create(
    const char *name, axis_extension_on_configure_func_t on_configure,
    axis_extension_on_init_func_t on_init,
    axis_extension_on_start_func_t on_start,
    axis_extension_on_stop_func_t on_stop,
    axis_extension_on_deinit_func_t on_deinit,
    axis_extension_on_cmd_func_t on_cmd, axis_extension_on_data_func_t on_data,
    axis_extension_on_audio_frame_func_t on_audio_frame,
    axis_extension_on_video_frame_func_t on_video_frame,
    axis_UNUSED void *user_data) {
  axis_ASSERT(name, "Should not happen.");

  axis_extension_t *self =
      (axis_extension_t *)axis_MALLOC(sizeof(axis_extension_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_signature_set(&self->signature, (axis_signature_t)axis_EXTENSION_SIGNATURE);
  axis_sanitizer_thread_check_init_with_current_thread(&self->thread_check);

  // --------------------------
  // Public interface.
  self->on_configure = on_configure;
  self->on_init = on_init;
  self->on_start = on_start;
  self->on_stop = on_stop;
  self->on_deinit = on_deinit;
  self->on_cmd = on_cmd;
  self->on_data = on_data;
  self->on_audio_frame = on_audio_frame;
  self->on_video_frame = on_video_frame;

  // --------------------------
  // Private data.
  self->addon_host = NULL;
  axis_string_init_formatted(&self->name, "%s", name);

  self->axis_env = NULL;
  self->binding_handle.me_in_target_lang = self;
  self->extension_thread = NULL;
  self->extension_info = NULL;

  axis_list_init(&self->pending_msgs);
  axis_list_init(&self->path_timers);

  self->path_timeout_info.in_path_timeout = axis_DEFAULT_PATH_TIMEOUT;
  self->path_timeout_info.out_path_timeout = axis_DEFAULT_PATH_TIMEOUT;
  self->path_timeout_info.check_interval =
      axis_DEFAULT_PATH_CHECK_INTERVAL;  // 10 seconds by default.

  self->state = axis_EXTENSION_STATE_INIT;

  axis_value_init_object_with_move(&self->manifest, NULL);
  axis_value_init_object_with_move(&self->property, NULL);
  axis_schema_store_init(&self->schema_store);

  self->manifest_info = NULL;
  self->property_info = NULL;

  self->path_table =
      axis_path_table_create(axis_PATH_TABLE_ATTACH_TO_EXTENSION, self);

  self->axis_env = axis_env_create_for_extension(self);

  axis_hashtable_init(
      &self->msg_not_connected_count_map,
      offsetof(axis_extension_output_msg_not_connected_count_t, hh_in_map));

  self->user_data = user_data;

  return self;
}

bool axis_extension_check_integrity(axis_extension_t *self, bool check_thread) {
  axis_ASSERT(self, "Invalid argument.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_EXTENSION_SIGNATURE) {
    return false;
  }

  axis_extension_thread_t *extension_thread = self->extension_thread;

  if (check_thread) {
    // Utilize the check_integrity of extension_thread to examine cases
    // involving the lock_mode of extension_thread.
    if (extension_thread) {
      if (axis_extension_thread_check_integrity_if_in_lock_mode(
              extension_thread)) {
        return true;
      }
    }
  }

  if (check_thread &&
      !axis_sanitizer_thread_check_do_check(&self->thread_check)) {
    return false;
  }

  return true;
}

void axis_extension_destroy(axis_extension_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: In APTIMA world, the destroy operations need to be performed in
  // any threads.
  axis_ASSERT(axis_extension_check_integrity(self, false),
             "Invalid use of extension %p.", self);

  axis_ASSERT(self->axis_env, "Should not happen.");

  // TODO(xilin): Make sure the thread safety.
  // axis_LOGD("[%s] Destroyed.", axis_extension_get_name(self, true));

  axis_sanitizer_thread_check_deinit(&self->thread_check);
  axis_signature_set(&self->signature, 0);

  axis_env_destroy(self->axis_env);
  axis_string_deinit(&self->name);

  axis_value_deinit(&self->manifest);
  axis_value_deinit(&self->property);
  axis_schema_store_deinit(&self->schema_store);

  if (self->manifest_info) {
    axis_metadata_info_destroy(self->manifest_info);
    self->manifest_info = NULL;
  }
  if (self->property_info) {
    axis_metadata_info_destroy(self->property_info);
    self->property_info = NULL;
  }

  axis_list_clear(&self->pending_msgs);

  axis_path_table_check_empty(self->path_table);
  axis_path_table_destroy(self->path_table);

  axis_ASSERT(axis_list_is_empty(&self->path_timers),
             "The path timers should all be closed before the destroy.");

  if (self->addon_host) {
    // Since the extension has already been destroyed, there is no need to
    // release its resources through the corresponding addon anymore. Therefore,
    // decrement the reference count of the corresponding addon.
    axis_ref_dec_ref(&self->addon_host->ref);
    self->addon_host = NULL;
  }

  axis_hashtable_deinit(&self->msg_not_connected_count_map);

  axis_FREE(self);
}

static bool axis_extension_check_if_msg_dests_have_msg_names(
    axis_extension_t *self, axis_list_t *msg_dests, axis_list_t *msg_names) {
  axis_ASSERT(msg_dests && msg_names, "Invalid argument.");

  axis_list_foreach (msg_dests, iter) {
    axis_shared_ptr_t *shared_msg_dest = axis_smart_ptr_listnode_get(iter.node);
    axis_msg_dest_info_t *msg_dest = axis_shared_ptr_get_data(shared_msg_dest);
    axis_ASSERT(msg_dest && axis_msg_dest_info_check_integrity(msg_dest),
               "Invalid argument.");

    axis_listnode_t *node = axis_list_find_ptr_custom(msg_names, &msg_dest->name,
                                                    axis_string_is_equal);
    if (node) {
      axis_ASSERT(0, "Extension (%s) has duplicated msg name (%s) in dest info.",
                 axis_extension_get_name(self, true),
                 axis_string_get_raw_str(&msg_dest->name));
      return true;
    }
  }

  return false;
}

static bool axis_extension_merge_interface_dest_to_msg(
    axis_extension_t *self, axis_extension_context_t *extension_context,
    axis_list_iterator_t iter, axis_MSG_TYPE msg_type, axis_list_t *msg_dests) {
  axis_ASSERT(
      self && axis_extension_check_integrity(self, true) && extension_context,
      "Should not happen.");

  axis_msg_dest_info_t *interface_dest =
      axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(iter.node));
  axis_ASSERT(interface_dest, "Should not happen.");

  axis_string_t *interface_name = &interface_dest->name;
  axis_ASSERT(!axis_string_is_empty(interface_name), "Should not happen.");

  axis_list_t all_msg_names_in_interface_out = axis_LIST_INIT_VAL;
  bool interface_found = axis_schema_store_get_all_msg_names_in_interface_out(
      &self->schema_store, msg_type, axis_string_get_raw_str(interface_name),
      &all_msg_names_in_interface_out);
  if (!interface_found) {
    axis_ASSERT(0, "Extension uses an undefined output interface (%s).",
               axis_string_get_raw_str(interface_name));
    return false;
  }

  if (axis_list_is_empty(&all_msg_names_in_interface_out)) {
    // The interface does not define any message of this type, it's legal.
    return true;
  }

  if (axis_extension_check_if_msg_dests_have_msg_names(
          self, msg_dests, &all_msg_names_in_interface_out)) {
    axis_ASSERT(0, "Should not happen.");
    return false;
  }

  axis_list_foreach (&all_msg_names_in_interface_out, iter) {
    axis_string_t *msg_name = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(msg_name, "Should not happen.");

    axis_msg_dest_info_t *msg_dest =
        axis_msg_dest_info_create(axis_string_get_raw_str(msg_name));

    axis_list_foreach (&interface_dest->dest, iter_dest) {
      axis_weak_ptr_t *shared_dest_extension_info =
          axis_smart_ptr_listnode_get(iter_dest.node);
      axis_list_push_smart_ptr_back(&msg_dest->dest, shared_dest_extension_info);
    }

    axis_shared_ptr_t *shared_msg_dest =
        axis_shared_ptr_create(msg_dest, axis_msg_dest_info_destroy);
    axis_list_push_smart_ptr_back(msg_dests, shared_msg_dest);
    axis_shared_ptr_destroy(shared_msg_dest);
  }

  axis_list_clear(&all_msg_names_in_interface_out);

  return true;
}

bool axis_extension_determine_and_merge_all_interface_dest_extension(
    axis_extension_t *self) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Invalid argument.");
  axis_ASSERT(self->state == axis_EXTENSION_STATE_ON_CONFIGURE_DONE,
             "Extension should be on_configure_done.");

  if (!self->extension_info) {
    return true;
  }

  axis_list_foreach (&self->extension_info->msg_dest_info.interface, iter) {
    if (!axis_extension_merge_interface_dest_to_msg(
            self, self->extension_context, iter, axis_MSG_TYPE_CMD,
            &self->extension_info->msg_dest_info.cmd)) {
      return false;
    }

    if (!axis_extension_merge_interface_dest_to_msg(
            self, self->extension_context, iter, axis_MSG_TYPE_DATA,
            &self->extension_info->msg_dest_info.data)) {
      return false;
    }

    if (!axis_extension_merge_interface_dest_to_msg(
            self, self->extension_context, iter, axis_MSG_TYPE_VIDEO_FRAME,
            &self->extension_info->msg_dest_info.video_frame)) {
      return false;
    }

    if (!axis_extension_merge_interface_dest_to_msg(
            self, self->extension_context, iter, axis_MSG_TYPE_AUDIO_FRAME,
            &self->extension_info->msg_dest_info.audio_frame)) {
      return false;
    }
  }

  return true;
}

static axis_msg_dest_info_t *axis_extension_get_msg_dests_from_graph_internal(
    axis_list_t *dest_info_list, axis_shared_ptr_t *msg) {
  axis_ASSERT(dest_info_list && msg, "Should not happen.");

  const char *msg_name = axis_msg_get_name(msg);

  // TODO(Wei): Use hash table to speed up the findings.
  axis_listnode_t *msg_dest_info_node = axis_list_find_shared_ptr_custom(
      dest_info_list, msg_name, axis_msg_dest_info_qualified);
  if (msg_dest_info_node) {
    axis_msg_dest_info_t *msg_dest =
        axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(msg_dest_info_node));

    return msg_dest;
  }

  return NULL;
}

static axis_msg_dest_info_t *axis_extension_get_msg_dests_from_graph(
    axis_extension_t *self, axis_shared_ptr_t *msg) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true) && msg,
             "Should not happen.");

  if (axis_msg_is_cmd_and_result(msg)) {
    return axis_extension_get_msg_dests_from_graph_internal(
        &self->extension_info->msg_dest_info.cmd, msg);
  } else {
    switch (axis_msg_get_type(msg)) {
      case axis_MSG_TYPE_DATA:
        return axis_extension_get_msg_dests_from_graph_internal(
            &self->extension_info->msg_dest_info.data, msg);
      case axis_MSG_TYPE_VIDEO_FRAME:
        return axis_extension_get_msg_dests_from_graph_internal(
            &self->extension_info->msg_dest_info.video_frame, msg);
      case axis_MSG_TYPE_AUDIO_FRAME:
        return axis_extension_get_msg_dests_from_graph_internal(
            &self->extension_info->msg_dest_info.audio_frame, msg);
      default:
        axis_ASSERT(0, "Should not happen.");
        return NULL;
    }
  }
}

static bool need_to_clone_msg_when_sending(axis_shared_ptr_t *msg,
                                           size_t dest_index) {
  axis_ASSERT(msg && axis_msg_check_integrity(msg), "Should not happen.");

  // Because when a message is sent to other extensions, these extensions
  // might be located in different extension groups. Therefore, after the
  // message is sent, if it is addressed to more than two extensions, the APTIMA
  // runtime needs to clone a copy of the message for extensions beyond the
  // first one, to avoid creating a thread safety issue. This principle applies
  // to all message types.
  //
  // A potential future optimization is that for the second and subsequent
  // extensions, if they are in the same extension group as the current
  // extension, then the cloning process can be omitted.

  return dest_index > 0;
}

static void axis_extension_determine_out_msg_dest_from_msg(
    axis_extension_t *self, axis_shared_ptr_t *msg, axis_list_t *result_msgs) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Invalid argument.");
  axis_ASSERT(msg && axis_msg_check_integrity(msg) && axis_msg_get_dest_cnt(msg),
             "Invalid argument.");
  axis_ASSERT(result_msgs && axis_list_size(result_msgs) == 0,
             "Should not happen.");

  axis_list_t dests = axis_LIST_INIT_VAL;

  axis_list_swap(&dests, axis_msg_get_dest(msg));

  axis_list_foreach (&dests, iter) {
    axis_loc_t *dest_loc = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(dest_loc && axis_loc_check_integrity(dest_loc),
               "Should not happen.");

    bool need_to_clone_msg = need_to_clone_msg_when_sending(msg, iter.index);

    axis_shared_ptr_t *curr_msg = NULL;
    if (need_to_clone_msg) {
      curr_msg = axis_msg_clone(msg, NULL);
    } else {
      curr_msg = msg;
    }

    axis_msg_clear_and_set_dest_to_loc(curr_msg, dest_loc);

    axis_list_push_smart_ptr_back(result_msgs, curr_msg);

    if (need_to_clone_msg) {
      axis_shared_ptr_destroy(curr_msg);
    }
  }

  axis_list_clear(&dests);
}

/**
 * @return true if a destination is specified within the graph, false otherwise.
 */
static bool axis_extension_determine_out_msg_dest_from_graph(
    axis_extension_t *self, axis_shared_ptr_t *msg, axis_list_t *result_msgs,
    axis_RESULT_RETURN_POLICY *result_return_policy, axis_error_t *err) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Invalid argument.");
  axis_ASSERT(
      msg && axis_msg_check_integrity(msg) && axis_msg_get_dest_cnt(msg) == 0,
      "Invalid argument.");
  axis_ASSERT(result_msgs && axis_list_size(result_msgs) == 0,
             "Invalid argument.");
  axis_ASSERT(result_return_policy, "Invalid argument.");

  axis_UNUSED axis_extension_thread_t *extension_thread = self->extension_thread;
  axis_ASSERT(extension_thread, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(extension_thread, true),
             "Invalid use of extension_thread %p.", extension_thread);

  // Fetch the destinations from the graph.
  axis_msg_dest_info_t *msg_dest_info =
      axis_extension_get_msg_dests_from_graph(self, msg);
  if (msg_dest_info) {
    *result_return_policy = msg_dest_info->policy;
    axis_list_t *dests = &msg_dest_info->dest;

    if (dests && axis_list_size(dests) > 0) {
      axis_list_foreach (dests, iter) {
        bool need_to_clone_msg =
            need_to_clone_msg_when_sending(msg, iter.index);

        axis_shared_ptr_t *curr_msg = NULL;
        if (need_to_clone_msg) {
          curr_msg = axis_msg_clone(msg, NULL);
        } else {
          curr_msg = msg;
        }

        axis_extension_info_t *dest_extension_info =
            axis_smart_ptr_get_data(axis_smart_ptr_listnode_get(iter.node));
        axis_ASSERT(dest_extension_info, "Should not happen.");

        // axis_NOLINTNEXTLINE(thread-check)
        // thread-check: The graph-related information of the extension remains
        // unchanged during the lifecycle of engine/graph, allowing safe
        // cross-thread access.
        axis_ASSERT(
            axis_extension_info_check_integrity(dest_extension_info, false),
            "Invalid use of extension_info %p.", dest_extension_info);

        axis_msg_clear_and_set_dest_from_extension_info(curr_msg,
                                                       dest_extension_info);

        axis_list_push_smart_ptr_back(result_msgs, curr_msg);

        if (need_to_clone_msg) {
          axis_shared_ptr_destroy(curr_msg);
        }
      }

      return true;
    }
  }

  // Graph doesn't specify how to route the messages.

  axis_MSG_TYPE msg_type = axis_msg_get_type(msg);
  const char *msg_name = axis_msg_get_name(msg);

  // In any case, the user needs to be informed about the error where the graph
  // does not have a specified destination for the message.
  axis_ASSERT(err, "Should not happen.");
  axis_error_set(err, axis_ERRNO_MSG_NOT_CONNECTED,
                "Failed to find destination of a '%s' message '%s' from graph.",
                axis_msg_type_to_string(msg_type), msg_name);

  return false;
}

typedef enum axis_EXTENSION_DETERMINE_OUT_MSGS_RESULT {
  axis_EXTENSION_DETERMINE_OUT_MSGS_SUCCESS,
  axis_EXTENSION_DETERMINE_OUT_MSGS_NOT_FOUND_IN_GRAPH,
  axis_EXTENSION_DETERMINE_OUT_MSGS_DROPPING,
  axis_EXTENSION_DETERMINE_OUT_MSGS_CACHING_IN_PATH_IN_GROUP,
} axis_EXTENSION_DETERMINE_OUT_MSGS_RESULT;

/**
 * @brief The time to find the destination of a message is when that message
 * is going to leave an extension. There are 2 directions of sending
 * messages out of an extension:
 *
 *  msg <- (backward) <-
 *                      Extension
 *                                -> (forward) -> msg
 *
 * - Forward path:
 *   This is for all messages except the cmd results. The destination of
 * the message should be determined in the message itself, or in the graph.
 *
 * - Backward path:
 *   This is for cmd results only. The destination of the cmd result
 *   should be determined in the IN path table according to the command ID
 * of the cmd result.
 */
static axis_EXTENSION_DETERMINE_OUT_MSGS_RESULT axis_extension_determine_out_msgs(
    axis_extension_t *self, axis_shared_ptr_t *msg, axis_list_t *result_msgs,
    axis_RESULT_RETURN_POLICY *result_return_policy, axis_path_t *in_path,
    axis_error_t *err) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Invalid argument.");
  axis_ASSERT(msg && axis_msg_check_integrity(msg), "Invalid argument.");
  axis_ASSERT(result_msgs, "Invalid argument.");
  axis_ASSERT(result_return_policy, "Invalid argument.");

  if (axis_msg_get_dest_cnt(msg) > 0) {
    // Because the messages has already had destinations, no matter it is a
    // backward path or a forward path, dispatch the message according to the
    // destinations specified in the message.

    axis_extension_determine_out_msg_dest_from_msg(self, msg, result_msgs);

    return axis_EXTENSION_DETERMINE_OUT_MSGS_SUCCESS;
  } else {
    // Need to find the destinations from 2 databases:
    // - graph: all messages without the cmd results.
    // - IN path table: cmd results only.
    if (axis_msg_is_cmd_and_result(msg)) {
      if (axis_msg_get_type(msg) == axis_MSG_TYPE_CMD_RESULT) {
        // Find the destinations of a cmd result from the path table.
        if (!in_path) {
          axis_LOGD("[%s] IN path is missing, discard cmd result.",
                   axis_extension_get_name(self, true));

          return axis_EXTENSION_DETERMINE_OUT_MSGS_DROPPING;
        }

        axis_ASSERT(axis_path_check_integrity(in_path, true),
                   "Should not happen.");

        msg = axis_path_table_determine_actual_cmd_result(
            self->path_table, axis_PATH_IN, in_path,
            axis_cmd_result_is_final(msg, NULL));
        if (msg) {
          // The cmd result is resolved to be transmitted to the previous node.

          axis_extension_determine_out_msg_dest_from_msg(self, msg, result_msgs);
          axis_shared_ptr_destroy(msg);

          return axis_EXTENSION_DETERMINE_OUT_MSGS_SUCCESS;
        } else {
          return axis_EXTENSION_DETERMINE_OUT_MSGS_CACHING_IN_PATH_IN_GROUP;
        }
      } else {
        // All command types except the cmd results, should find its
        // destination in the graph.
        if (axis_extension_determine_out_msg_dest_from_graph(
                self, msg, result_msgs, result_return_policy, err)) {
          return axis_EXTENSION_DETERMINE_OUT_MSGS_SUCCESS;
        } else {
          return axis_EXTENSION_DETERMINE_OUT_MSGS_NOT_FOUND_IN_GRAPH;
        }
      }
    } else {
      // The message is not a command, so the only source to find its
      // destination is in the graph.
      if (axis_extension_determine_out_msg_dest_from_graph(
              self, msg, result_msgs, result_return_policy, err)) {
        return axis_EXTENSION_DETERMINE_OUT_MSGS_SUCCESS;
      } else {
        return axis_EXTENSION_DETERMINE_OUT_MSGS_NOT_FOUND_IN_GRAPH;
      }
    }
  }
}

bool axis_extension_dispatch_msg(axis_extension_t *self, axis_shared_ptr_t *msg,
                                axis_error_t *err) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(msg && axis_msg_check_integrity(msg), "Should not happen.");
  axis_ASSERT(err && axis_error_check_integrity(err), "Invalid argument.");

  // The source of the out message is the current extension.
  axis_msg_set_src_to_extension(msg, self);

  bool result = true;
  const bool msg_is_cmd = axis_msg_is_cmd_and_result(msg);
  bool msg_is_cmd_result = false;

  if (axis_msg_get_type(msg) == axis_MSG_TYPE_CMD_RESULT) {
    msg_is_cmd_result = true;

    // The backward path should strictly follow the information recorded
    // in the path table, therefore, users should not 'override' the
    // destination location in this case.
    axis_msg_clear_dest(msg);
  }

  axis_msg_correct_dest(msg, self->extension_context->engine);

  // Before the schema validation, the origin cmd name is needed to retrieve the
  // schema definition if the msg is a cmd result.
  axis_path_t *in_path = NULL;
  if (axis_msg_get_type(msg) == axis_MSG_TYPE_CMD_RESULT) {
    // We do not need to resolve in the path group even if the `in_path` is in a
    // group, as we only want the cmd name here.
    in_path = axis_path_table_set_result(self->path_table, axis_PATH_IN, msg);
    if (!in_path) {
      axis_LOGD("[%s] IN path is missing, discard cmd result.",
               axis_extension_get_name(self, true));
      return true;
    }

    axis_ASSERT(axis_path_check_integrity(in_path, true), "Should not happen.");
    axis_ASSERT(!axis_string_is_empty(&in_path->cmd_name), "Should not happen.");

    // We need to use the original cmd name to find the schema definition of the
    // cmd result.
    axis_cmd_result_set_original_cmd_name(
        msg, axis_string_get_raw_str(&in_path->cmd_name));
  }

  // The schema validation of the `msg` must be happened before
  // `axis_extension_determine_out_msgs()`, the reasons are as follows:
  //
  // 1. The schema of the msg sending out or returning from the extension is
  // defined by the extension itself, however there might have some conversions
  // defined by users of the extension (ex: the conversion will be defined in a
  // graph). So the schema validation should be happened before the conversions
  // as the structure of the msg might be changed after conversions.
  //
  // 2. For cmd result, its path will be removed in
  // `axis_extension_determine_out_msgs()`. But users can call `return_result()`
  // twice if the schema validation fails in the first time. In other words, the
  // path can not be removed if the schema validation fails.
  if (!axis_extension_validate_msg_schema(self, msg, true, err)) {
    return false;
  }

  axis_list_t result_msgs = axis_LIST_INIT_VAL;  // axis_shared_ptr_t*
  axis_RESULT_RETURN_POLICY result_return_policy =
      axis_RESULT_RETURN_POLICY_INVALID;

  switch (axis_extension_determine_out_msgs(
      self, msg, &result_msgs, &result_return_policy, in_path, err)) {
    case axis_EXTENSION_DETERMINE_OUT_MSGS_NOT_FOUND_IN_GRAPH:
      result = false;
    case axis_EXTENSION_DETERMINE_OUT_MSGS_CACHING_IN_PATH_IN_GROUP:
    case axis_EXTENSION_DETERMINE_OUT_MSGS_DROPPING:
      goto done;

    case axis_EXTENSION_DETERMINE_OUT_MSGS_SUCCESS:
      break;

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  if (msg_is_cmd && !msg_is_cmd_result) {
    // Need to add all out path entries before _dispatching_ messages,
    // otherwise, the checking of receiving all results would be misjudgment.

    axis_list_t result_out_paths = axis_LIST_INIT_VAL;

    axis_list_foreach (&result_msgs, iter) {
      axis_shared_ptr_t *result_msg = axis_smart_ptr_listnode_get(iter.node);
      axis_ASSERT(result_msg && axis_msg_check_integrity(result_msg),
                 "Invalid argument.");

      axis_path_t *path = (axis_path_t *)axis_path_table_add_out_path(
          self->path_table, result_msg);
      axis_ASSERT(path && axis_path_check_integrity(path, true),
                 "Should not happen.");

      axis_list_push_ptr_back(&result_out_paths, path, NULL);
    }

    if (axis_list_size(&result_out_paths) > 1) {
      // Create a path group in this case.
      axis_paths_create_group(&result_out_paths, result_return_policy);
    }

    axis_list_clear(&result_out_paths);
  }

  // The handling of the OUT path table is completed, it's time to send the
  // message out of the extension.

  axis_list_foreach (&result_msgs, iter) {
    axis_shared_ptr_t *result_msg = axis_smart_ptr_listnode_get(iter.node);
    axis_ASSERT(result_msg && axis_msg_check_integrity(result_msg),
               "Invalid argument.");

    axis_extension_thread_dispatch_msg(self->extension_thread, result_msg);
  }

done:
  axis_list_clear(&result_msgs);
  return result;
}

axis_runloop_t *axis_extension_get_attached_runloop(axis_extension_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads when
  // the extension is alive.
  axis_ASSERT(axis_extension_check_integrity(self, false),
             "Invalid use of extension %p.", self);

  return self->extension_thread->runloop;
}

static void axis_extension_on_configure(axis_env_t *axis_env) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");
  axis_ASSERT(axis_env_get_attach_to(axis_env) == axis_ENV_ATTACH_TO_EXTENSION,
             "Invalid argument.");

  axis_extension_t *self = axis_env_get_attached_extension(axis_env);
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  axis_LOGD("[%s] on_configure().", axis_extension_get_name(self, true));

  self->manifest_info =
      axis_metadata_info_create(axis_METADATA_ATTACH_TO_MANIFEST, self->axis_env);
  self->property_info =
      axis_metadata_info_create(axis_METADATA_ATTACH_TO_PROPERTY, self->axis_env);

  if (self->on_configure) {
    self->on_configure(self, self->axis_env);
  } else {
    axis_extension_on_configure_done(self->axis_env);
  }
}

void axis_extension_on_init(axis_env_t *axis_env) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");
  axis_ASSERT(axis_env_get_attach_to(axis_env) == axis_ENV_ATTACH_TO_EXTENSION,
             "Invalid argument.");

  axis_extension_t *self = axis_env_get_attached_extension(axis_env);
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  axis_LOGD("[%s] on_init().", axis_extension_get_name(self, true));

  if (self->on_init) {
    self->on_init(self, self->axis_env);
  } else {
    (void)axis_extension_on_init_done(self->axis_env);
  }
}

void axis_extension_on_start(axis_extension_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  axis_LOGI("[%s] on_start().", axis_extension_get_name(self, true));

  self->state = axis_EXTENSION_STATE_ON_START;

  if (self->on_start) {
    self->on_start(self, self->axis_env);
  } else {
    axis_extension_on_start_done(self->axis_env);
  }
}

void axis_extension_on_stop(axis_extension_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  axis_LOGI("[%s] on_stop().", axis_extension_get_name(self, true));

  if (self->on_stop) {
    self->on_stop(self, self->axis_env);
  } else {
    axis_extension_on_stop_done(self->axis_env);
  }
}

void axis_extension_on_deinit(axis_extension_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  axis_LOGD("[%s] on_deinit().", axis_extension_get_name(self, true));

  self->state = axis_EXTENSION_STATE_ON_DEINIT;

  if (self->on_deinit) {
    self->on_deinit(self, self->axis_env);
  } else {
    axis_extension_on_deinit_done(self->axis_env);
  }
}

void axis_extension_on_cmd(axis_extension_t *self, axis_shared_ptr_t *msg) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  axis_LOGV("[%s] on_cmd(%s).", axis_extension_get_name(self, true),
           axis_msg_get_name(msg));

  if (self->on_cmd) {
    self->on_cmd(self, self->axis_env, msg);
  } else {
    // The default behavior of 'on_cmd' is to _not_ forward this command out,
    // and return an 'OK' result to the previous stage.
    axis_shared_ptr_t *cmd_result =
        axis_cmd_result_create_from_cmd(axis_STATUS_CODE_OK, msg);
    axis_env_return_result(self->axis_env, cmd_result, msg, NULL, NULL, NULL);
    axis_shared_ptr_destroy(cmd_result);
  }
}

void axis_extension_on_data(axis_extension_t *self, axis_shared_ptr_t *msg) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  axis_LOGV("[%s] on_data(%s).", axis_extension_get_name(self, true),
           axis_msg_get_name(msg));

  if (self->on_data) {
    self->on_data(self, self->axis_env, msg);
  } else {
    // Bypass the data.
    axis_env_send_data(self->axis_env, msg, NULL, NULL, NULL);
  }
}

void axis_extension_on_video_frame(axis_extension_t *self,
                                  axis_shared_ptr_t *msg) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  axis_LOGV("[%s] on_video_frame(%s).", axis_extension_get_name(self, true),
           axis_msg_get_name(msg));

  if (self->on_video_frame) {
    self->on_video_frame(self, self->axis_env, msg);
  } else {
    // Bypass the video frame.
    axis_env_send_video_frame(self->axis_env, msg, NULL, NULL, NULL);
  }
}

void axis_extension_on_audio_frame(axis_extension_t *self,
                                  axis_shared_ptr_t *msg) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  axis_LOGV("[%s] on_audio_frame(%s).", axis_extension_get_name(self, true),
           axis_msg_get_name(msg));

  if (self->on_audio_frame) {
    self->on_audio_frame(self, self->axis_env, msg);
  } else {
    // Bypass the audio frame.
    axis_env_send_audio_frame(self->axis_env, msg, NULL, NULL, NULL);
  }
}

void axis_extension_load_metadata(axis_extension_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  axis_LOGD("[%s] Load metadata.", axis_extension_get_name(self, true));

  // This function is safe to be called from the extension main threads, because
  // all the resources it accesses are not be modified after the app
  // initialization phase.
  axis_UNUSED axis_extension_thread_t *extension_thread = self->extension_thread;
  axis_ASSERT(extension_thread, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(extension_thread, true),
             "Invalid use of extension_thread %p.", extension_thread);

  axis_metadata_load(axis_extension_on_configure, self->axis_env);
}

void axis_extension_set_me_in_target_lang(axis_extension_t *self,
                                         void *me_in_target_lang) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  self->binding_handle.me_in_target_lang = me_in_target_lang;
}

axis_env_t *axis_extension_get_axis_env(axis_extension_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: Need to pay attention to the thread safety of the using side
  // of this function.
  axis_ASSERT(axis_extension_check_integrity(self, false),
             "Invalid use of extension %p.", self);

  return self->axis_env;
}

axis_addon_host_t *axis_extension_get_addon(axis_extension_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  return self->addon_host;
}

const char *axis_extension_get_name(axis_extension_t *self, bool check_thread) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_check_integrity(self, check_thread),
             "Invalid use of extension %p.", self);

  return axis_string_get_raw_str(&self->name);
}

void axis_extension_set_addon(axis_extension_t *self,
                             axis_addon_host_t *addon_host) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  axis_ASSERT(addon_host, "Should not happen.");
  axis_ASSERT(axis_addon_host_check_integrity(addon_host), "Should not happen.");

  // Since the extension requires the corresponding addon to release
  // its resources, therefore, hold on to a reference count of the corresponding
  // addon.
  axis_ASSERT(!self->addon_host, "Should not happen.");
  self->addon_host = addon_host;
  axis_ref_inc_ref(&addon_host->ref);
}

axis_path_in_t *axis_extension_get_cmd_return_path_from_itself(
    axis_extension_t *self, const char *cmd_id) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true) && cmd_id,
             "Should not happen.");

  axis_listnode_t *returned_node = axis_path_table_find_path_from_cmd_id(
      self->path_table, axis_PATH_IN, cmd_id);

  if (!returned_node) {
    return NULL;
  }

  return axis_ptr_listnode_get(returned_node);
}

bool axis_extension_validate_msg_schema(axis_extension_t *self,
                                       axis_shared_ptr_t *msg, bool is_msg_out,
                                       axis_error_t *err) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Invalid argument.");
  axis_ASSERT(msg && axis_msg_check_integrity(msg), "Invalid argument.");

  bool validated =
      axis_msg_validate_schema(msg, &self->schema_store, is_msg_out, err);
  if (!validated) {
    axis_LOGW("[%s] See %s %s::%s with invalid schema: %s.",
             axis_extension_get_name(self, true), is_msg_out ? "out" : "in",
             axis_msg_get_type_string(msg), axis_msg_get_name(msg),
             axis_error_errmsg(err));

    if (!is_msg_out) {
      // Only when a schema checking error occurs before a message is sent to
      // the extension should an automatic generated error cmd result. This is
      // because, at this time, the extension has not had the opportunity to do
      // anything. Conversely, if a schema checking error occurs after a message
      // is sent out from the extension, the extension will be notified of the
      // error, giving it a chance to handle the error. Otherwise, if an
      // automatic error cmd result is also sent at this time, the act of
      // notifying the extension of a message error would be meaningless.

      switch (axis_msg_get_type(msg)) {
        case axis_MSG_TYPE_CMD_TIMER:
        case axis_MSG_TYPE_CMD_TIMEOUT:
        case axis_MSG_TYPE_CMD_STOP_GRAPH:
        case axis_MSG_TYPE_CMD_CLOSE_APP:
        case axis_MSG_TYPE_CMD_START_GRAPH:
        case axis_MSG_TYPE_CMD: {
          axis_shared_ptr_t *cmd_result =
              axis_cmd_result_create_from_cmd(axis_STATUS_CODE_ERROR, msg);
          axis_msg_set_property(cmd_result, "detail",
                               axis_value_create_string(axis_error_errmsg(err)),
                               NULL);
          axis_env_return_result(self->axis_env, cmd_result, msg, NULL, NULL,
                                NULL);
          axis_shared_ptr_destroy(cmd_result);
          break;
        }

        case axis_MSG_TYPE_CMD_RESULT:
          // TODO(Liu): The detail or property in the cmd result might be
          // invalid, we should adjust the value according to the schema
          // definition. Ex: set the value to default after the schema system
          // supports `default` keyword.
          //
          // Set the status_code of the cmd result to an error code to notify to
          // the target extension that something wrong.
          //
          // TODO(Wei): Do we really need to set the status_code to error?
          axis_cmd_result_set_status_code(msg, axis_STATUS_CODE_ERROR);

          // No matter what happens, the flow of the cmd result should
          // continue. Otherwise, the sender will not know what is happening,
          // and the entire command flow will be blocked.
          validated = true;
          break;

        case axis_MSG_TYPE_DATA:
        case axis_MSG_TYPE_VIDEO_FRAME:
        case axis_MSG_TYPE_AUDIO_FRAME:
          // TODO(Liu): Provide a better way to let users know about this error
          // as there is no ack for msgs except cmd. We might consider dropping
          // this type of message at this point, and sending an event into the
          // extension.
          break;

        default:
          axis_ASSERT(0, "Should not happen.");
          break;
      }
    }
  }

  return validated;
}

axis_extension_t *axis_extension_from_smart_ptr(
    axis_smart_ptr_t *extension_smart_ptr) {
  axis_ASSERT(extension_smart_ptr, "Invalid argument.");
  return axis_smart_ptr_get_data(extension_smart_ptr);
}
