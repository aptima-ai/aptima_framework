//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/extension/extension.h"

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "include_internal/ten_runtime/addon/addon.h"
#include "include_internal/ten_runtime/common/loc.h"
#include "include_internal/ten_runtime/engine/engine.h"
#include "include_internal/ten_runtime/extension/extension_info/extension_info.h"
#include "include_internal/ten_runtime/extension/msg_dest_info/json.h"
#include "include_internal/ten_runtime/extension/msg_dest_info/msg_dest_info.h"
#include "include_internal/ten_runtime/extension/msg_handling.h"
#include "include_internal/ten_runtime/extension/on_xxx.h"
#include "include_internal/ten_runtime/extension_context/extension_context.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/extension_thread/extension_thread.h"
#include "include_internal/ten_runtime/extension_thread/msg_interface/common.h"
#include "include_internal/ten_runtime/metadata/metadata_info.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd_result/cmd.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_runtime/path/path.h"
#include "include_internal/ten_runtime/path/path_group.h"
#include "include_internal/ten_runtime/path/path_in.h"
#include "include_internal/ten_runtime/path/path_table.h"
#include "include_internal/ten_runtime/schema_store/store.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "include_internal/ten_utils/log/log.h"
#include "ten_runtime/addon/addon.h"
#include "ten_runtime/app/app.h"
#include "ten_runtime/common/errno.h"
#include "ten_runtime/msg/cmd_result/cmd_result.h"
#include "ten_runtime/msg/msg.h"
#include "ten_runtime/ten_env/internal/on_xxx_done.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_node.h"
#include "ten_utils/container/list_node_smart_ptr.h"
#include "ten_utils/container/list_ptr.h"
#include "ten_utils/container/list_smart_ptr.h"
#include "ten_utils/io/runloop.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/ref.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"
#include "ten_utils/value/value.h"

ten_extension_t *ten_extension_create(
    const char *name, ten_extension_on_configure_func_t on_configure,
    ten_extension_on_init_func_t on_init,
    ten_extension_on_start_func_t on_start,
    ten_extension_on_stop_func_t on_stop,
    ten_extension_on_deinit_func_t on_deinit,
    ten_extension_on_cmd_func_t on_cmd, ten_extension_on_data_func_t on_data,
    ten_extension_on_audio_frame_func_t on_audio_frame,
    ten_extension_on_video_frame_func_t on_video_frame,
    TEN_UNUSED void *user_data) {
  TEN_ASSERT(name, "Should not happen.");

  ten_extension_t *self =
      (ten_extension_t *)TEN_MALLOC(sizeof(ten_extension_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  ten_signature_set(&self->signature, (ten_signature_t)TEN_EXTENSION_SIGNATURE);
  ten_sanitizer_thread_check_init_with_current_thread(&self->thread_check);

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
  ten_string_init_formatted(&self->name, "%s", name);

  ten_string_init(&self->base_dir);

  self->ten_env = NULL;
  self->binding_handle.me_in_target_lang = self;
  self->extension_thread = NULL;
  self->extension_info = NULL;

  ten_list_init(&self->pending_msgs);
  ten_list_init(&self->path_timers);

  self->path_timeout_info.in_path_timeout = TEN_DEFAULT_PATH_TIMEOUT;
  self->path_timeout_info.out_path_timeout = TEN_DEFAULT_PATH_TIMEOUT;
  self->path_timeout_info.check_interval =
      TEN_DEFAULT_PATH_CHECK_INTERVAL;  // 10 seconds by default.

  self->state = TEN_EXTENSION_STATE_INIT;

  ten_value_init_object_with_move(&self->manifest, NULL);
  ten_value_init_object_with_move(&self->property, NULL);
  ten_schema_store_init(&self->schema_store);

  self->manifest_info = NULL;
  self->property_info = NULL;

  self->path_table =
      ten_path_table_create(TEN_PATH_TABLE_ATTACH_TO_EXTENSION, self);

  self->ten_env = ten_env_create_for_extension(self);

  return self;
}

bool ten_extension_check_integrity(ten_extension_t *self, bool check_thread) {
  TEN_ASSERT(self, "Invalid argument.");

  if (ten_signature_get(&self->signature) !=
      (ten_signature_t)TEN_EXTENSION_SIGNATURE) {
    return false;
  }

  ten_extension_thread_t *extension_thread = self->extension_thread;

  if (check_thread) {
    // Utilize the check_integrity of extension_thread to examine cases
    // involving the lock_mode of extension_thread.
    if (extension_thread) {
      if (ten_extension_thread_check_integrity_if_in_lock_mode(
              extension_thread)) {
        return true;
      }
    }
  }

  if (check_thread &&
      !ten_sanitizer_thread_check_do_check(&self->thread_check)) {
    return false;
  }

  return true;
}

void ten_extension_destroy(ten_extension_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: In TEN world, the destroy operations need to be performed in
  // any threads.
  TEN_ASSERT(ten_extension_check_integrity(self, false),
             "Invalid use of extension %p.", self);

  TEN_ASSERT(self->ten_env, "Should not happen.");

  // TODO(xilin): Make sure the thread safety.
  // TEN_LOGD("[%s] Destroyed.", ten_extension_get_name(self));

  ten_sanitizer_thread_check_deinit(&self->thread_check);
  ten_signature_set(&self->signature, 0);

  ten_env_destroy(self->ten_env);
  ten_string_deinit(&self->name);

  ten_value_deinit(&self->manifest);
  ten_value_deinit(&self->property);
  ten_schema_store_deinit(&self->schema_store);

  if (self->manifest_info) {
    ten_metadata_info_destroy(self->manifest_info);
    self->manifest_info = NULL;
  }
  if (self->property_info) {
    ten_metadata_info_destroy(self->property_info);
    self->property_info = NULL;
  }

  ten_list_clear(&self->pending_msgs);

  ten_path_table_check_empty(self->path_table);
  ten_path_table_destroy(self->path_table);

  ten_string_deinit(&self->base_dir);

  TEN_ASSERT(ten_list_is_empty(&self->path_timers),
             "The path timers should all be closed before the destroy.");

  if (self->addon_host) {
    // Since the extension has already been destroyed, there is no need to
    // release its resources through the corresponding addon anymore. Therefore,
    // decrement the reference count of the corresponding addon.
    ten_ref_dec_ref(&self->addon_host->ref);
    self->addon_host = NULL;
  }

  TEN_FREE(self);
}

static bool ten_extension_check_if_msg_dests_have_msg_names(
    ten_extension_t *self, ten_list_t *msg_dests, ten_list_t *msg_names) {
  TEN_ASSERT(msg_dests && msg_names, "Invalid argument.");

  ten_list_foreach (msg_dests, iter) {
    ten_shared_ptr_t *shared_msg_dest = ten_smart_ptr_listnode_get(iter.node);
    ten_msg_dest_info_t *msg_dest = ten_shared_ptr_get_data(shared_msg_dest);
    TEN_ASSERT(msg_dest && ten_msg_dest_info_check_integrity(msg_dest),
               "Invalid argument.");

    ten_listnode_t *node = ten_list_find_ptr_custom(msg_names, &msg_dest->name,
                                                    ten_string_is_equal);
    if (node) {
      TEN_ASSERT(0, "Extension (%s) has duplicated msg name (%s) in dest info.",
                 ten_extension_get_name(self),
                 ten_string_get_raw_str(&msg_dest->name));
      return true;
    }
  }

  return false;
}

static bool ten_extension_merge_interface_dest_to_msg(
    ten_extension_t *self, ten_extension_context_t *extension_context,
    ten_list_iterator_t iter, TEN_MSG_TYPE msg_type, ten_list_t *msg_dests) {
  TEN_ASSERT(
      self && ten_extension_check_integrity(self, true) && extension_context,
      "Should not happen.");

  ten_msg_dest_info_t *interface_dest =
      ten_shared_ptr_get_data(ten_smart_ptr_listnode_get(iter.node));
  TEN_ASSERT(interface_dest, "Should not happen.");

  ten_string_t *interface_name = &interface_dest->name;
  TEN_ASSERT(!ten_string_is_empty(interface_name), "Should not happen.");

  ten_list_t all_msg_names_in_interface_out = TEN_LIST_INIT_VAL;
  bool interface_found = ten_schema_store_get_all_msg_names_in_interface_out(
      &self->schema_store, msg_type, ten_string_get_raw_str(interface_name),
      &all_msg_names_in_interface_out);
  if (!interface_found) {
    TEN_ASSERT(0, "Extension uses an undefined output interface (%s).",
               ten_string_get_raw_str(interface_name));
    return false;
  }

  if (ten_list_is_empty(&all_msg_names_in_interface_out)) {
    // The interface does not define any message of this type, it's legal.
    return true;
  }

  if (ten_extension_check_if_msg_dests_have_msg_names(
          self, msg_dests, &all_msg_names_in_interface_out)) {
    TEN_ASSERT(0, "Should not happen.");
    return false;
  }

  ten_list_foreach (&all_msg_names_in_interface_out, iter) {
    ten_string_t *msg_name = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(msg_name, "Should not happen.");

    ten_msg_dest_info_t *msg_dest =
        ten_msg_dest_info_create(ten_string_get_raw_str(msg_name));

    ten_list_foreach (&interface_dest->dest, iter_dest) {
      ten_weak_ptr_t *shared_dest_extension_info =
          ten_smart_ptr_listnode_get(iter_dest.node);
      ten_list_push_smart_ptr_back(&msg_dest->dest, shared_dest_extension_info);
    }

    ten_shared_ptr_t *shared_msg_dest =
        ten_shared_ptr_create(msg_dest, ten_msg_dest_info_destroy);
    ten_list_push_smart_ptr_back(msg_dests, shared_msg_dest);
    ten_shared_ptr_destroy(shared_msg_dest);
  }

  ten_list_clear(&all_msg_names_in_interface_out);

  return true;
}

bool ten_extension_determine_and_merge_all_interface_dest_extension(
    ten_extension_t *self) {
  TEN_ASSERT(self && ten_extension_check_integrity(self, true),
             "Invalid argument.");
  TEN_ASSERT(self->state == TEN_EXTENSION_STATE_ON_CONFIGURE_DONE,
             "Extension should be on_configure_done.");

  if (!self->extension_info) {
    return true;
  }

  ten_list_foreach (&self->extension_info->msg_dest_info.interface, iter) {
    if (!ten_extension_merge_interface_dest_to_msg(
            self, self->extension_context, iter, TEN_MSG_TYPE_CMD,
            &self->extension_info->msg_dest_info.cmd)) {
      return false;
    }

    if (!ten_extension_merge_interface_dest_to_msg(
            self, self->extension_context, iter, TEN_MSG_TYPE_DATA,
            &self->extension_info->msg_dest_info.data)) {
      return false;
    }

    if (!ten_extension_merge_interface_dest_to_msg(
            self, self->extension_context, iter, TEN_MSG_TYPE_VIDEO_FRAME,
            &self->extension_info->msg_dest_info.video_frame)) {
      return false;
    }

    if (!ten_extension_merge_interface_dest_to_msg(
            self, self->extension_context, iter, TEN_MSG_TYPE_AUDIO_FRAME,
            &self->extension_info->msg_dest_info.audio_frame)) {
      return false;
    }
  }

  return true;
}

static ten_list_t *ten_extension_get_msg_dests_from_graph_internal(
    ten_list_t *dest_info_list, ten_shared_ptr_t *msg) {
  TEN_ASSERT(dest_info_list && msg, "Should not happen.");

  const char *msg_name = ten_msg_get_name(msg);

  // TODO(Wei): Use hash table to speed up the findings.
  ten_listnode_t *msg_dest_info_node = ten_list_find_shared_ptr_custom(
      dest_info_list, msg_name, ten_msg_dest_info_qualified);
  if (msg_dest_info_node) {
    ten_msg_dest_info_t *msg_dest =
        ten_shared_ptr_get_data(ten_smart_ptr_listnode_get(msg_dest_info_node));

    return &msg_dest->dest;
  }

  return NULL;
}

static ten_list_t *ten_extension_get_msg_dests_from_graph(
    ten_extension_t *self, ten_shared_ptr_t *msg) {
  TEN_ASSERT(self && ten_extension_check_integrity(self, true) && msg,
             "Should not happen.");

  if (ten_msg_is_cmd_base(msg)) {
    return ten_extension_get_msg_dests_from_graph_internal(
        &self->extension_info->msg_dest_info.cmd, msg);
  } else {
    switch (ten_msg_get_type(msg)) {
      case TEN_MSG_TYPE_DATA:
        return ten_extension_get_msg_dests_from_graph_internal(
            &self->extension_info->msg_dest_info.data, msg);
      case TEN_MSG_TYPE_VIDEO_FRAME:
        return ten_extension_get_msg_dests_from_graph_internal(
            &self->extension_info->msg_dest_info.video_frame, msg);
      case TEN_MSG_TYPE_AUDIO_FRAME:
        return ten_extension_get_msg_dests_from_graph_internal(
            &self->extension_info->msg_dest_info.audio_frame, msg);
      default:
        TEN_ASSERT(0, "Should not happen.");
        return NULL;
    }
  }
}

static bool need_to_clone_msg_when_sending(ten_shared_ptr_t *msg,
                                           size_t dest_index) {
  TEN_ASSERT(msg && ten_msg_check_integrity(msg), "Should not happen.");

  // Because when a message is sent to other extensions, these extensions
  // might be located in different extension groups. Therefore, after the
  // message is sent, if it is addressed to more than two extensions, the TEN
  // runtime needs to clone a copy of the message for extensions beyond the
  // first one, to avoid creating a thread safety issue. This principle applies
  // to all message types.
  //
  // A potential future optimization is that for the second and subsequent
  // extensions, if they are in the same extension group as the current
  // extension, then the cloning process can be omitted.

  return dest_index > 0;
}

static void ten_extension_determine_out_msg_dest_from_msg(
    ten_extension_t *self, ten_shared_ptr_t *msg, ten_list_t *result_msgs) {
  TEN_ASSERT(self && ten_extension_check_integrity(self, true),
             "Invalid argument.");
  TEN_ASSERT(msg && ten_msg_check_integrity(msg) && ten_msg_get_dest_cnt(msg),
             "Invalid argument.");
  TEN_ASSERT(result_msgs && ten_list_size(result_msgs) == 0,
             "Should not happen.");

  ten_list_t dests = TEN_LIST_INIT_VAL;

  ten_list_swap(&dests, ten_msg_get_dest(msg));

  ten_list_foreach (&dests, iter) {
    ten_loc_t *dest_loc = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(dest_loc && ten_loc_check_integrity(dest_loc),
               "Should not happen.");

    bool need_to_clone_msg = need_to_clone_msg_when_sending(msg, iter.index);

    ten_shared_ptr_t *curr_msg = NULL;
    if (need_to_clone_msg) {
      curr_msg = ten_msg_clone(msg, NULL);
    } else {
      curr_msg = msg;
    }

    ten_msg_clear_and_set_dest_to_loc(curr_msg, dest_loc);

    ten_list_push_smart_ptr_back(result_msgs, curr_msg);

    if (need_to_clone_msg) {
      ten_shared_ptr_destroy(curr_msg);
    }
  }

  ten_list_clear(&dests);
}

/**
 * @return true if a destination is specified within the graph, false otherwise.
 */
static bool ten_extension_determine_out_msg_dest_from_graph(
    ten_extension_t *self, ten_shared_ptr_t *msg, ten_list_t *result_msgs,
    ten_error_t *err) {
  TEN_ASSERT(self && ten_extension_check_integrity(self, true),
             "Should not happen.");
  TEN_ASSERT(
      msg && ten_msg_check_integrity(msg) && ten_msg_get_dest_cnt(msg) == 0,
      "Should not happen.");
  TEN_ASSERT(result_msgs && ten_list_size(result_msgs) == 0,
             "Should not happen.");

  TEN_UNUSED ten_extension_thread_t *extension_thread = self->extension_thread;
  TEN_ASSERT(extension_thread, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(extension_thread, true),
             "Invalid use of extension_thread %p.", extension_thread);

  // Fetch the destinations from the graph.
  ten_list_t *dests = ten_extension_get_msg_dests_from_graph(self, msg);

  if (dests && ten_list_size(dests) > 0) {
    ten_list_foreach (dests, iter) {
      bool need_to_clone_msg = need_to_clone_msg_when_sending(msg, iter.index);

      ten_shared_ptr_t *curr_msg = NULL;
      if (need_to_clone_msg) {
        curr_msg = ten_msg_clone(msg, NULL);
      } else {
        curr_msg = msg;
      }

      ten_extension_info_t *dest_extension_info =
          ten_smart_ptr_get_data(ten_smart_ptr_listnode_get(iter.node));
      TEN_ASSERT(dest_extension_info, "Should not happen.");

      // TEN_NOLINTNEXTLINE(thread-check)
      // thread-check: The graph-related information of the extension remains
      // unchanged during the lifecycle of engine/graph, allowing safe
      // cross-thread access.
      TEN_ASSERT(ten_extension_info_check_integrity(dest_extension_info, false),
                 "Invalid use of extension_info %p.", dest_extension_info);

      ten_msg_clear_and_set_dest_from_extension_info(curr_msg,
                                                     dest_extension_info);

      ten_list_push_smart_ptr_back(result_msgs, curr_msg);

      if (need_to_clone_msg) {
        ten_shared_ptr_destroy(curr_msg);
      }
    }

    return true;
  } else {
    // Graph doesn't specify how to route the messages.

    const char *msg_name = ten_msg_get_name(msg);

    if (err) {
      ten_error_set(err, TEN_ERRNO_INVALID_GRAPH,
                    "Failed to find destination of a message (%s) from graph.",
                    msg_name);
    } else {
      if (ten_msg_is_cmd_base(msg)) {
        TEN_LOGE("Failed to find destination of a command (%s) from graph.",
                 msg_name);
      } else {
        // The amount of the data-like messages might be huge, so we don't dump
        // error logs here to prevent log flooding.
      }
    }

    return false;
  }
}

typedef enum TEN_EXTENSION_DETERMINE_OUT_MSGS_RESULT {
  TEN_EXTENSION_DETERMINE_OUT_MSGS_SUCCESS,
  TEN_EXTENSION_DETERMINE_OUT_MSGS_NOT_FOUND_IN_GRAPH,
  TEN_EXTENSION_DETERMINE_OUT_MSGS_DROPPING,
  TEN_EXTENSION_DETERMINE_OUT_MSGS_CACHING_IN_PATH_IN_GROUP,
} TEN_EXTENSION_DETERMINE_OUT_MSGS_RESULT;

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
static TEN_EXTENSION_DETERMINE_OUT_MSGS_RESULT ten_extension_determine_out_msgs(
    ten_extension_t *self, ten_shared_ptr_t *msg, ten_list_t *result_msgs,
    ten_path_t *in_path, ten_error_t *err) {
  TEN_ASSERT(self && ten_extension_check_integrity(self, true),
             "Should not happen.");
  TEN_ASSERT(msg && ten_msg_check_integrity(msg), "Should not happen.");
  TEN_ASSERT(result_msgs, "Should not happen.");

  if (ten_msg_get_dest_cnt(msg) > 0) {
    // Because the messages has already had destinations, no matter it is a
    // backward path or a forward path, dispatch the message according to the
    // destinations specified in the message.

    ten_extension_determine_out_msg_dest_from_msg(self, msg, result_msgs);

    return TEN_EXTENSION_DETERMINE_OUT_MSGS_SUCCESS;
  } else {
    // Need to find the destinations from 2 databases:
    // - graph: all messages without the cmd results.
    // - IN path table: cmd results only.
    if (ten_msg_is_cmd_base(msg)) {
      if (ten_msg_get_type(msg) == TEN_MSG_TYPE_CMD_RESULT) {
        // Find the destinations of a cmd result from the path table.
        if (!in_path) {
          TEN_LOGD("[%s] IN path is missing, discard cmd result.",
                   ten_extension_get_name(self));

          return TEN_EXTENSION_DETERMINE_OUT_MSGS_DROPPING;
        }

        TEN_ASSERT(ten_path_check_integrity(in_path, true),
                   "Should not happen.");

        msg = ten_path_table_determine_actual_cmd_result(
            self->path_table, TEN_PATH_IN, in_path,
            ten_cmd_result_get_is_final(msg, NULL));
        if (msg) {
          // The cmd result is resolved to be transmitted to the previous node.

          ten_extension_determine_out_msg_dest_from_msg(self, msg, result_msgs);
          ten_shared_ptr_destroy(msg);

          return TEN_EXTENSION_DETERMINE_OUT_MSGS_SUCCESS;
        } else {
          return TEN_EXTENSION_DETERMINE_OUT_MSGS_CACHING_IN_PATH_IN_GROUP;
        }
      } else {
        // All command types except the cmd results, should find its
        // destination in the graph.
        if (ten_extension_determine_out_msg_dest_from_graph(self, msg,
                                                            result_msgs, err)) {
          return TEN_EXTENSION_DETERMINE_OUT_MSGS_SUCCESS;
        } else {
          return TEN_EXTENSION_DETERMINE_OUT_MSGS_NOT_FOUND_IN_GRAPH;
        }
      }
    } else {
      // The message is not a command, so the only source to find its
      // destination is in the graph.
      if (ten_extension_determine_out_msg_dest_from_graph(self, msg,
                                                          result_msgs, err)) {
        return TEN_EXTENSION_DETERMINE_OUT_MSGS_SUCCESS;
      } else {
        return TEN_EXTENSION_DETERMINE_OUT_MSGS_NOT_FOUND_IN_GRAPH;
      }
    }
  }
}

static void ten_extension_dispatch_msg(ten_extension_t *self,
                                       ten_shared_ptr_t *msg) {
  TEN_ASSERT(self && ten_extension_check_integrity(self, true) && msg &&
                 ten_msg_check_integrity(msg),
             "Should not happen.");

  ten_extension_thread_dispatch_msg(self->extension_thread, msg);
}

bool ten_extension_handle_out_msg(ten_extension_t *self, ten_shared_ptr_t *msg,
                                  ten_error_t *err) {
  TEN_ASSERT(self && ten_extension_check_integrity(self, true),
             "Should not happen.");
  TEN_ASSERT(msg && ten_msg_check_integrity(msg), "Should not happen.");
  TEN_ASSERT(err && ten_error_check_integrity(err), "Invalid argument.");

  // The source of the out message is the current extension.
  ten_msg_set_src_to_extension(msg, self);

  bool result = true;
  const bool msg_is_cmd = ten_msg_is_cmd_base(msg);
  bool msg_is_cmd_result = false;

  if (ten_msg_get_type(msg) == TEN_MSG_TYPE_CMD_RESULT) {
    msg_is_cmd_result = true;

    // The backward path should strictly follow the information recorded
    // in the path table, therefore, users should not 'override' the
    // destination location in this case.
    ten_msg_clear_dest(msg);
  }

  ten_msg_correct_dest(msg, self->extension_context->engine);

  // Before the schema validation, the origin cmd name is needed to retrieve the
  // schema definition if the msg is a cmd result.
  ten_path_t *in_path = NULL;
  if (ten_msg_get_type(msg) == TEN_MSG_TYPE_CMD_RESULT) {
    // We do not need to resolve in the path group even if the `in_path` is in a
    // group, as we only want the cmd name here.
    in_path = ten_path_table_set_result(self->path_table, TEN_PATH_IN, msg);
    if (!in_path) {
      TEN_LOGD("[%s] IN path is missing, discard cmd result.",
               ten_extension_get_name(self));
      return true;
    }

    TEN_ASSERT(ten_path_check_integrity(in_path, true), "Should not happen.");
    TEN_ASSERT(!ten_string_is_empty(&in_path->cmd_name), "Should not happen.");

    // We need to use the original cmd name to find the schema definition of the
    // cmd result.
    ten_cmd_result_set_original_cmd_name(
        msg, ten_string_get_raw_str(&in_path->cmd_name));
  }

  // The schema validation of the `msg` must be happened before
  // `ten_extension_determine_out_msgs()`, the reasons are as follows:
  //
  // 1. The schema of the msg sending out or returning from the extension is
  // defined by the extension itself, however there might have some conversions
  // defined by users of the extension (ex: the conversion will be defined in a
  // graph). So the schema validation should be happened before the conversions
  // as the structure of the msg might be changed after conversions.
  //
  // 2. For cmd result, its path will be removed in
  // `ten_extension_determine_out_msgs()`. But users can call `return_result()`
  // twice if the schema validation fails in the first time. In other words, the
  // path can not be removed if the schema validation fails.
  if (!ten_extension_validate_msg_schema(self, msg, true, err)) {
    return false;
  }

  ten_list_t result_msgs = TEN_LIST_INIT_VAL;  // ten_shared_ptr_t*

  switch (
      ten_extension_determine_out_msgs(self, msg, &result_msgs, in_path, err)) {
    case TEN_EXTENSION_DETERMINE_OUT_MSGS_NOT_FOUND_IN_GRAPH:
      result = false;
    case TEN_EXTENSION_DETERMINE_OUT_MSGS_CACHING_IN_PATH_IN_GROUP:
    case TEN_EXTENSION_DETERMINE_OUT_MSGS_DROPPING:
      goto done;

    case TEN_EXTENSION_DETERMINE_OUT_MSGS_SUCCESS:
      break;

    default:
      TEN_ASSERT(0, "Should not happen.");
      break;
  }

  if (msg_is_cmd && !msg_is_cmd_result) {
    // Need to add all out path entries before _dispatching_ messages,
    // otherwise, the checking of receiving all results would be misjudgment.

    ten_list_t result_out_paths = TEN_LIST_INIT_VAL;

    ten_list_foreach (&result_msgs, iter) {
      ten_shared_ptr_t *result_msg = ten_smart_ptr_listnode_get(iter.node);
      TEN_ASSERT(result_msg && ten_msg_check_integrity(result_msg),
                 "Invalid argument.");

      ten_path_t *path = (ten_path_t *)ten_path_table_add_out_path(
          self->path_table, result_msg);
      TEN_ASSERT(path && ten_path_check_integrity(path, true),
                 "Should not happen.");

      ten_list_push_ptr_back(&result_out_paths, path, NULL);
    }

    if (ten_list_size(&result_out_paths) > 1) {
      // Create a path group in this case.
      ten_paths_create_group(
          &result_out_paths,
          TEN_PATH_GROUP_POLICY_ONE_FAIL_RETURN_AND_ALL_OK_RETURN_LAST);
    }

    ten_list_clear(&result_out_paths);
  }

  // The handling of the OUT path table is completed, it's time to send the
  // message out of the extension.

  ten_list_foreach (&result_msgs, iter) {
    ten_shared_ptr_t *result_msg = ten_smart_ptr_listnode_get(iter.node);
    TEN_ASSERT(result_msg && ten_msg_check_integrity(result_msg),
               "Invalid argument.");

    ten_extension_dispatch_msg(self, result_msg);
  }

done:
  ten_list_clear(&result_msgs);
  return result;
}

ten_runloop_t *ten_extension_get_attached_runloop(ten_extension_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads when
  // the extension is alive.
  TEN_ASSERT(ten_extension_check_integrity(self, false),
             "Invalid use of extension %p.", self);

  return self->extension_thread->runloop;
}

static void ten_extension_on_configure(ten_env_t *ten_env) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");
  TEN_ASSERT(ten_env_get_attach_to(ten_env) == TEN_ENV_ATTACH_TO_EXTENSION,
             "Invalid argument.");

  ten_extension_t *self = ten_env_get_attached_extension(ten_env);
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  TEN_LOGD("[%s] on_configure().", ten_extension_get_name(self));

  self->manifest_info =
      ten_metadata_info_create(TEN_METADATA_ATTACH_TO_MANIFEST, self->ten_env);
  self->property_info =
      ten_metadata_info_create(TEN_METADATA_ATTACH_TO_PROPERTY, self->ten_env);

  if (self->on_configure) {
    self->on_configure(self, self->ten_env);
  } else {
    ten_extension_on_configure_done(self->ten_env);
  }
}

void ten_extension_on_init(ten_env_t *ten_env) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");
  TEN_ASSERT(ten_env_get_attach_to(ten_env) == TEN_ENV_ATTACH_TO_EXTENSION,
             "Invalid argument.");

  ten_extension_t *self = ten_env_get_attached_extension(ten_env);
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  TEN_LOGD("[%s] on_init().", ten_extension_get_name(self));

  if (self->on_init) {
    self->on_init(self, self->ten_env);
  } else {
    ten_extension_on_init_done(self->ten_env);
  }
}

static void ten_extension_flush_all_pending_msgs(ten_extension_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  // Flush the previously got messages, which are received before
  // on_init_done(), into the extension.
  ten_extension_thread_t *extension_thread = self->extension_thread;
  ten_list_foreach (&extension_thread->pending_msgs, iter) {
    ten_shared_ptr_t *msg = ten_smart_ptr_listnode_get(iter.node);
    TEN_ASSERT(msg, "Should not happen.");

    ten_loc_t *dest_loc = ten_msg_get_first_dest_loc(msg);
    TEN_ASSERT(dest_loc, "Should not happen.");

    if (ten_string_is_equal(&dest_loc->extension_name, &self->name)) {
      ten_extension_handle_in_msg(self, msg);
      ten_list_remove_node(&extension_thread->pending_msgs, iter.node);
    }
  }

  // Flush the previously got messages, which are received before
  // on_init_done(), into the extension.
  ten_list_foreach (&self->pending_msgs, iter) {
    ten_shared_ptr_t *msg = ten_smart_ptr_listnode_get(iter.node);
    TEN_ASSERT(msg, "Should not happen.");

    ten_extension_handle_in_msg(self, msg);
  }
  ten_list_clear(&self->pending_msgs);
}

void ten_extension_on_start(ten_extension_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  TEN_LOGI("[%s] on_start().", ten_extension_get_name(self));

  self->state = TEN_EXTENSION_STATE_ON_START;

  if (self->on_start) {
    self->on_start(self, self->ten_env);

    // The developer expects that on_start() will execute before all on_cmd()
    // events. Therefore, after on_start() has been executed, there is no need
    // to wait for on_start_done() before sending all previously buffered
    // messages into the extension.

    ten_extension_flush_all_pending_msgs(self);
  } else {
    ten_extension_flush_all_pending_msgs(self);

    ten_extension_on_start_done(self->ten_env);
  }
}

void ten_extension_on_stop(ten_extension_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  TEN_LOGI("[%s] on_stop().", ten_extension_get_name(self));

  if (self->on_stop) {
    self->on_stop(self, self->ten_env);
  } else {
    ten_extension_on_stop_done(self->ten_env);
  }
}

void ten_extension_on_deinit(ten_extension_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  TEN_LOGD("[%s] on_deinit().", ten_extension_get_name(self));

  self->state = TEN_EXTENSION_STATE_ON_DEINIT;

  if (self->on_deinit) {
    self->on_deinit(self, self->ten_env);
  } else {
    ten_extension_on_deinit_done(self->ten_env);
  }
}

void ten_extension_on_cmd(ten_extension_t *self, ten_shared_ptr_t *msg) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  TEN_LOGV("[%s] on_cmd(%s).", ten_extension_get_name(self),
           ten_msg_get_name(msg));

  if (self->on_cmd) {
    self->on_cmd(self, self->ten_env, msg);
  } else {
    // The default behavior of 'on_cmd' is to _not_ forward this command out,
    // and return an 'OK' result to the previous stage.
    ten_shared_ptr_t *cmd_result =
        ten_cmd_result_create_from_cmd(TEN_STATUS_CODE_OK, msg);
    ten_env_return_result(self->ten_env, cmd_result, msg, NULL);
    ten_shared_ptr_destroy(cmd_result);
  }
}

void ten_extension_on_data(ten_extension_t *self, ten_shared_ptr_t *msg) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  TEN_LOGV("[%s] on_data(%s).", ten_extension_get_name(self),
           ten_msg_get_name(msg));

  if (self->on_data) {
    self->on_data(self, self->ten_env, msg);
  } else {
    // Bypass the data.
    ten_env_send_data(self->ten_env, msg, NULL);
  }
}

void ten_extension_on_video_frame(ten_extension_t *self,
                                  ten_shared_ptr_t *msg) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  TEN_LOGV("[%s] on_video_frame(%s).", ten_extension_get_name(self),
           ten_msg_get_name(msg));

  if (self->on_video_frame) {
    self->on_video_frame(self, self->ten_env, msg);
  } else {
    // Bypass the video frame.
    ten_env_send_video_frame(self->ten_env, msg, NULL);
  }
}

void ten_extension_on_audio_frame(ten_extension_t *self,
                                  ten_shared_ptr_t *msg) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  TEN_LOGV("[%s] on_audio_frame(%s).", ten_extension_get_name(self),
           ten_msg_get_name(msg));

  if (self->on_audio_frame) {
    self->on_audio_frame(self, self->ten_env, msg);
  } else {
    // Bypass the audio frame.
    ten_env_send_audio_frame(self->ten_env, msg, NULL);
  }
}

void ten_extension_load_metadata(ten_extension_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  TEN_LOGD("[%s] Load metadata.", ten_extension_get_name(self));

  // This function is safe to be called from the extension main threads, because
  // all the resources it accesses are not be modified after the app
  // initialization phase.
  TEN_UNUSED ten_extension_thread_t *extension_thread = self->extension_thread;
  TEN_ASSERT(extension_thread, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(extension_thread, true),
             "Invalid use of extension_thread %p.", extension_thread);

  if (self->addon_host) {
    // If the extension is created by an addon, then the base directory of the
    // extension can be set to `<app>/ten_packages/extension/<addon-name>`. And
    // the `base_dir` must be set before `on_init()`, as the `property.json` and
    // `manifest.json` under the `base_dir` might be loaded in the default
    // behavior of `on_init()`, or users might want to know the value of
    // `base_dir` in `on_init()`.
    ten_addon_host_set_base_dir(self->addon_host,
                                self->extension_context->engine->app,
                                &self->base_dir);
  }

  ten_metadata_load(ten_extension_on_configure, self->ten_env);
}

void ten_extension_set_me_in_target_lang(ten_extension_t *self,
                                         void *me_in_target_lang) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  self->binding_handle.me_in_target_lang = me_in_target_lang;
}

ten_env_t *ten_extension_get_ten(ten_extension_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: Need to pay attention to the thread safety of the using side
  // of this function.
  TEN_ASSERT(ten_extension_check_integrity(self, false),
             "Invalid use of extension %p.", self);

  return self->ten_env;
}

ten_addon_host_t *ten_extension_get_addon(ten_extension_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  return self->addon_host;
}

const char *ten_extension_get_name(ten_extension_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  return ten_string_get_raw_str(&self->name);
}

void ten_extension_set_addon(ten_extension_t *self,
                             ten_addon_host_t *addon_host) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  TEN_ASSERT(addon_host, "Should not happen.");
  TEN_ASSERT(ten_addon_host_check_integrity(addon_host), "Should not happen.");

  // Since the extension requires the corresponding addon to release
  // its resources, therefore, hold on to a reference count of the corresponding
  // addon.
  TEN_ASSERT(!self->addon_host, "Should not happen.");
  self->addon_host = addon_host;
  ten_ref_inc_ref(&addon_host->ref);
}

ten_path_in_t *ten_extension_get_cmd_return_path_from_itself(
    ten_extension_t *self, const char *cmd_id) {
  TEN_ASSERT(self && ten_extension_check_integrity(self, true) && cmd_id,
             "Should not happen.");

  ten_listnode_t *returned_node = ten_path_table_find_path_from_cmd_id(
      self->path_table, TEN_PATH_IN, cmd_id);

  if (!returned_node) {
    return NULL;
  }

  return ten_ptr_listnode_get(returned_node);
}

ten_string_t *ten_extension_get_base_dir(ten_extension_t *self) {
  TEN_ASSERT(self && ten_extension_check_integrity(self, true),
             "Invalid argument.");
  return &self->base_dir;
}

bool ten_extension_validate_msg_schema(ten_extension_t *self,
                                       ten_shared_ptr_t *msg, bool is_msg_out,
                                       ten_error_t *err) {
  TEN_ASSERT(self && ten_extension_check_integrity(self, true),
             "Invalid argument.");
  TEN_ASSERT(msg && ten_msg_check_integrity(msg), "Invalid argument.");

  bool validated =
      ten_msg_validate_schema(msg, &self->schema_store, is_msg_out, err);
  if (!validated) {
    TEN_LOGW("[%s] See %s %s::%s with invalid schema: %s.",
             ten_extension_get_name(self), is_msg_out ? "out" : "in",
             ten_msg_get_type_string(msg), ten_msg_get_name(msg),
             ten_error_errmsg(err));

    if (!is_msg_out) {
      // Only when a schema checking error occurs before a message is sent to
      // the extension should an automatic generated error cmd result. This is
      // because, at this time, the extension has not had the opportunity to do
      // anything. Conversely, if a schema checking error occurs after a message
      // is sent out from the extension, the extension will be notified of the
      // error, giving it a chance to handle the error. Otherwise, if an
      // automatic error cmd result is also sent at this time, the act of
      // notifying the extension of a message error would be meaningless.

      switch (ten_msg_get_type(msg)) {
        case TEN_MSG_TYPE_CMD_TIMER:
        case TEN_MSG_TYPE_CMD_TIMEOUT:
        case TEN_MSG_TYPE_CMD_STOP_GRAPH:
        case TEN_MSG_TYPE_CMD_CLOSE_APP:
        case TEN_MSG_TYPE_CMD_START_GRAPH:
        case TEN_MSG_TYPE_CMD: {
          ten_shared_ptr_t *cmd_result =
              ten_cmd_result_create_from_cmd(TEN_STATUS_CODE_ERROR, msg);
          ten_msg_set_property(cmd_result, "detail",
                               ten_value_create_string(ten_error_errmsg(err)),
                               NULL);
          ten_env_return_result(self->ten_env, cmd_result, msg, NULL);
          ten_shared_ptr_destroy(cmd_result);
          break;
        }

        case TEN_MSG_TYPE_CMD_RESULT:
          // TODO(Liu): The detail or property in the cmd result might be
          // invalid, we should adjust the value according to the schema
          // definition. Ex: set the value to default after the schema system
          // supports `default` keyword.
          //
          // Set the status_code of the cmd result to an error code to notify to
          // the target extension that something wrong.
          //
          // TODO(Wei): Do we really need to set the status_code to error?
          ten_cmd_result_set_status_code(msg, TEN_STATUS_CODE_ERROR);

          // No matter what happens, the flow of the cmd result should
          // continue. Otherwise, the sender will not know what is happening,
          // and the entire command flow will be blocked.
          validated = true;
          break;

        case TEN_MSG_TYPE_DATA:
        case TEN_MSG_TYPE_VIDEO_FRAME:
        case TEN_MSG_TYPE_AUDIO_FRAME:
          // TODO(Liu): Provide a better way to let users know about this error
          // as there is no ack for msgs except cmd. We might consider dropping
          // this type of message at this point, and sending an event into the
          // extension.
          break;

        default:
          TEN_ASSERT(0, "Should not happen.");
          break;
      }
    }
  }

  return validated;
}

ten_extension_t *ten_extension_from_smart_ptr(
    ten_smart_ptr_t *extension_smart_ptr) {
  TEN_ASSERT(extension_smart_ptr, "Invalid argument.");
  return ten_smart_ptr_get_data(extension_smart_ptr);
}
