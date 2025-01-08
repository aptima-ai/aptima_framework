//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension_context/extension_context.h"

#include <stddef.h>
#include <stdlib.h>

#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/addon/addon_host.h"
#include "include_internal/axis_runtime/addon/extension_group/extension_group.h"
#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/common/loc.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/engine/internal/thread.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension/extension_info/extension_info.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/extension_group/extension_group_info/extension_group_info.h"
#include "include_internal/axis_runtime/extension_group/on_xxx.h"
#include "include_internal/axis_runtime/extension_thread/extension_thread.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/start_graph/cmd.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/app/app.h"
#include "axis_runtime/common/errno.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/atomic.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"

bool axis_extension_context_check_integrity(axis_extension_context_t *self,
                                           bool check_thread) {
  axis_ASSERT(self, "Invalid argument.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_EXTENSION_CONTEXT_SIGNATURE) {
    return false;
  }

  if (check_thread &&
      !axis_sanitizer_thread_check_do_check(&self->thread_check)) {
    return false;
  }

  return true;
}

axis_extension_context_t *axis_extension_context_create(axis_engine_t *engine) {
  axis_ASSERT(engine && axis_engine_check_integrity(engine, true),
             "Should not happen.");

  axis_LOGD("[%s] Create Extension context.", axis_engine_get_id(engine, true));

  axis_extension_context_t *self =
      (axis_extension_context_t *)axis_MALLOC(sizeof(axis_extension_context_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_signature_set(&self->signature,
                    (axis_signature_t)axis_EXTENSION_CONTEXT_SIGNATURE);
  axis_sanitizer_thread_check_init_with_current_thread(&self->thread_check);

  axis_atomic_store(&self->is_closing, 0);
  self->on_closed = NULL;
  self->on_closed_data = NULL;

  self->engine = engine;

  axis_list_init(&self->extension_groups_info_from_graph);
  axis_list_init(&self->extensions_info_from_graph);

  axis_list_init(&self->extension_groups);
  axis_list_init(&self->extension_threads);

  self->extension_threads_cnt_of_ready = 0;
  self->extension_threads_cnt_of_closed = 0;

  self->extension_groups_cnt_of_being_destroyed = 0;

  self->state_requester_cmd = NULL;

  return self;
}

static void axis_extension_context_destroy(axis_extension_context_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_context_check_integrity(self, true),
             "Invalid use of extension_context %p.", self);

  axis_ASSERT((axis_list_size(&self->extension_threads) == 0),
             "Should not happen.");
  axis_ASSERT(axis_list_size(&self->extension_groups) == 0, "Should not happen.");

  axis_list_clear(&self->extension_groups_info_from_graph);
  axis_list_clear(&self->extensions_info_from_graph);

  if (self->state_requester_cmd) {
    axis_shared_ptr_destroy(self->state_requester_cmd);
  }

  axis_signature_set(&self->signature, 0);
  axis_sanitizer_thread_check_deinit(&self->thread_check);

  axis_FREE(self);
}

static void axis_extension_context_start(axis_extension_context_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_context_check_integrity(self, true),
             "Invalid use of extension_context %p.", self);

  axis_list_foreach (&self->extension_threads, iter) {
    axis_extension_thread_start(axis_ptr_listnode_get(iter.node));
  }
}

static void
axis_extension_context_do_close_after_all_extension_groups_are_closed(
    axis_extension_context_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_context_check_integrity(self, true),
             "Invalid use of extension_context %p.", self);

  axis_engine_t *engine = self->engine;
  axis_ASSERT(engine && axis_engine_check_integrity(engine, true),
             "Should not happen.");
  axis_env_close(engine->axis_env);

  if (self->on_closed) {
    self->on_closed(self, self->on_closed_data);
  }

  axis_extension_context_destroy(self);
}

void axis_extension_context_close(axis_extension_context_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_context_check_integrity(self, true),
             "Invalid use of extension_context %p.", self);

  axis_ASSERT(axis_engine_check_integrity(self->engine, true),
             "Should not happen.");

  if (!axis_atomic_bool_compare_swap(&self->is_closing, 0, 1)) {
    axis_LOGW("[%s] Extension context has already been signaled to close.",
             axis_engine_get_id(self->engine, true));
    return;
  }

  axis_LOGD("[%s] Try to close extension context.",
           axis_engine_get_id(self->engine, true));

  if (axis_list_size(&self->extension_threads)) {
    axis_list_foreach (&self->extension_threads, iter) {
      axis_extension_thread_t *extension_thread =
          axis_ptr_listnode_get(iter.node);
      axis_ASSERT(extension_thread && axis_extension_thread_check_integrity(
                                         extension_thread, false),
                 "Should not happen.");

      axis_extension_thread_close(extension_thread);
    }
  } else {
    // No extension threads need to be closed, so we can proceed directly to the
    // closing process of the extension context itself.
    axis_extension_context_do_close_after_all_extension_groups_are_closed(self);
  }
}

static bool axis_extension_context_could_be_close(
    axis_extension_context_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_context_check_integrity(self, true),
             "Invalid use of extension_context %p.", self);

  // Extension context could _only_ be closed when all extension threads have
  // been stopped.

  return self->extension_threads_cnt_of_closed ==
                 axis_list_size(&self->extension_threads)
             ? true
             : false;
}

static void axis_extension_context_on_extension_group_destroyed(
    axis_env_t *axis_env, void *cb_data) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");
  axis_ASSERT(axis_env->attach_to == axis_ENV_ATTACH_TO_ENGINE,
             "Should not happen.");

  axis_engine_t *engine = axis_env_get_attached_engine(axis_env);
  axis_ASSERT(engine && axis_engine_check_integrity(engine, true),
             "Should not happen.");

  axis_extension_context_t *extension_context = engine->extension_context;
  axis_ASSERT(extension_context, "Invalid argument.");
  axis_ASSERT(axis_extension_context_check_integrity(extension_context, true),
             "Invalid use of extension_context %p.", extension_context);

  if (--extension_context->extension_groups_cnt_of_being_destroyed == 0) {
    axis_extension_context_do_close_after_all_extension_groups_are_closed(
        extension_context);
  }
}

static void axis_extension_context_do_close(axis_extension_context_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_context_check_integrity(self, true),
             "Invalid use of extension_context %p.", self);

  axis_list_clear(&self->extension_threads);

  if (axis_list_size(&self->extension_groups) == 0) {
    axis_extension_context_do_close_after_all_extension_groups_are_closed(self);
    return;
  }

  self->extension_groups_cnt_of_being_destroyed =
      axis_list_size(&self->extension_groups);

  axis_list_clear(&self->extension_groups);
}

void axis_extension_context_on_close(axis_extension_context_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_context_check_integrity(self, true),
             "Invalid use of extension_context %p.", self);

  if (!axis_extension_context_could_be_close(self)) {
    axis_LOGD("[%s] Could not close alive extension context.",
             axis_engine_get_id(self->engine, true));
    return;
  }
  axis_LOGD("[%s] Close extension context.",
           axis_engine_get_id(self->engine, true));

  axis_extension_context_do_close(self);
}

void axis_extension_context_set_on_closed(
    axis_extension_context_t *self,
    axis_extension_context_on_closed_func_t on_closed, void *on_closed_data) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_context_check_integrity(self, true),
             "Invalid use of extension_context %p.", self);

  self->on_closed = on_closed;
  self->on_closed_data = on_closed_data;
}

axis_extension_info_t *axis_extension_context_get_extension_info_by_name(
    axis_extension_context_t *self, const char *app_uri, const char *graph_id,
    const char *extension_group_name, const char *extension_name) {
  axis_ASSERT(self, "Invalid argument.");

  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function will be called in the extension thread,
  // however, the graph would not be changed after the extension system is
  // starting, so it's safe to access the graph information in the extension
  // thead.
  //
  // However, for the strict thread safety, it's possible to modify the logic
  // here to use asynchronous operations (i.e., add a task to the
  // extension_context, and add a task to the extension_thread when the result
  // is found) here.
  axis_ASSERT(axis_extension_context_check_integrity(self, false),
             "Invalid use of extension_context %p.", self);

  axis_ASSERT(app_uri && extension_group_name && extension_name,
             "Should not happen.");

  axis_extension_info_t *result = NULL;

  axis_list_foreach (&self->extensions_info_from_graph, iter) {
    axis_extension_info_t *extension_info =
        axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(iter.node));

    if (app_uri &&
        !axis_string_is_equal_c_str(&extension_info->loc.app_uri, app_uri)) {
      continue;
    }

    if (graph_id &&
        !axis_string_is_equal_c_str(&extension_info->loc.graph_id, graph_id)) {
      continue;
    }

    if (extension_group_name &&
        !axis_string_is_equal_c_str(&extension_info->loc.extension_group_name,
                                   extension_group_name)) {
      continue;
    }

    if (extension_name &&
        !axis_string_is_equal_c_str(&extension_info->loc.extension_name,
                                   extension_name)) {
      continue;
    }

    result = extension_info;
    break;
  }

  return result;
}

static axis_extension_group_info_t *
axis_extension_context_get_extension_group_info_by_name(
    axis_extension_context_t *self, const char *app_uri,
    const char *extension_group_name) {
  axis_ASSERT(self, "Invalid argument.");

  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function will be called in the extension thread,
  // however, the graph would not be changed after the extension system is
  // starting, so it's safe to access the graph information in the extension
  // thead.
  //
  // However, for the strict thread safety, it's possible to modify the logic
  // here to use asynchronous operations (i.e., add a task to the
  // extension_context, and add a task to the extension_thread when the result
  // is found) here.
  axis_ASSERT(axis_extension_context_check_integrity(self, false),
             "Invalid use of extension_context %p.", self);

  axis_ASSERT(app_uri && extension_group_name, "Should not happen.");

  axis_extension_group_info_t *result = NULL;

  axis_list_foreach (&self->extension_groups_info_from_graph, iter) {
    axis_extension_group_info_t *extension_group_info =
        axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(iter.node));

    if (axis_string_is_equal_c_str(&extension_group_info->loc.app_uri,
                                  app_uri) &&
        axis_string_is_equal_c_str(
            &extension_group_info->loc.extension_group_name,
            extension_group_name)) {
      result = extension_group_info;
      break;
    }
  }

  return result;
}

static void axis_extension_context_add_extensions_info_from_graph(
    axis_extension_context_t *self, axis_list_t *extensions_info) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_context_check_integrity(self, true),
             "Invalid use of extension_context %p.", self);

  axis_ASSERT(extensions_info, "Should not happen.");

  axis_ASSERT(axis_list_size(&self->extensions_info_from_graph) == 0,
             "Should not happen.");

  axis_list_swap(&self->extensions_info_from_graph, extensions_info);
}

static void axis_extension_context_add_extension_groups_info_from_graph(
    axis_extension_context_t *self, axis_list_t *extension_groups_info) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_context_check_integrity(self, true),
             "Invalid use of extension_context %p.", self);

  axis_ASSERT(extension_groups_info, "Should not happen.");

  axis_ASSERT(axis_list_size(&self->extension_groups_info_from_graph) == 0,
             "Should not happen.");

  axis_list_swap(&self->extension_groups_info_from_graph, extension_groups_info);
}

static void destroy_extension_group_by_addon(
    axis_extension_group_t *extension_group) {
  axis_ASSERT(extension_group &&
                 axis_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  axis_extension_context_t *extension_context =
      extension_group->extension_context;
  axis_ASSERT(extension_context, "Invalid argument.");
  axis_ASSERT(axis_extension_context_check_integrity(extension_context, true),
             "Invalid use of extension_context %p.", extension_context);

  axis_engine_t *engine = extension_context->engine;
  axis_ASSERT(engine && axis_engine_check_integrity(engine, true),
             "Should not happen.");

  axis_env_t *axis_env = engine->axis_env;
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_addon_destroy_extension_group(
      axis_env, extension_group,
      axis_extension_context_on_extension_group_destroyed, NULL);
}

static void axis_extension_context_create_extension_group_done(
    axis_env_t *axis_env, axis_extension_group_t *extension_group) {
  axis_ASSERT(extension_group &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: The extension thread has not been created yet,
                 // so it is thread safe.
                 axis_extension_group_check_integrity(extension_group, false),
             "Should not happen.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");
  axis_ASSERT(axis_env->attach_to == axis_ENV_ATTACH_TO_ENGINE,
             "Should not happen.");

  axis_engine_t *engine = axis_env_get_attached_engine(axis_env);
  axis_ASSERT(engine && axis_engine_check_integrity(engine, true),
             "Should not happen.");

  axis_extension_context_t *extension_context = engine->extension_context;
  axis_ASSERT(extension_context, "Invalid argument.");
  axis_ASSERT(axis_extension_context_check_integrity(extension_context, true),
             "Invalid use of extension_context %p.", extension_context);

  axis_shared_ptr_t *requester_cmd = extension_context->state_requester_cmd;
  axis_ASSERT(requester_cmd, "Should not happen.");

  axis_cmd_start_graph_t *requester_cmd_start_graph =
      axis_shared_ptr_get_data(requester_cmd);
  axis_ASSERT(requester_cmd_start_graph, "Should not happen.");

  axis_addon_host_t *addon_host = extension_group->addon_host;
  axis_ASSERT(addon_host, "Should not happen.");

  extension_group->app = engine->app;
  extension_group->extension_context = extension_context;

  if (axis_string_is_equal_c_str(&addon_host->name,
                                axis_STR_DEFAULT_EXTENSION_GROUP)) {
    // default_extension_group is a special group, it needs the 'start_graph'
    // command to fill some important information.

    axis_ASSERT(
        requester_cmd &&
            axis_msg_get_type(requester_cmd) == axis_MSG_TYPE_CMD_START_GRAPH &&
            axis_msg_get_dest_cnt(requester_cmd) == 1,
        "Should not happen.");

    axis_loc_t *dest_loc = axis_msg_get_first_dest_loc(requester_cmd);
    axis_ASSERT(dest_loc, "Should not happen.");

    // Get the information of all the extensions which this extension group
    // should create.
    axis_list_t result =
        axis_cmd_start_graph_get_extension_addon_and_instance_name_pairs_of_specified_extension_group(
            requester_cmd, axis_string_get_raw_str(&dest_loc->app_uri),
            axis_string_get_raw_str(&dest_loc->graph_id),
            axis_string_get_raw_str(&extension_group->name));

    axis_list_swap(&extension_group->extension_addon_and_instance_name_pairs,
                  &result);
  }

  // Add the newly created extension_group into the list.
  axis_list_push_ptr_back(
      &extension_context->extension_groups, extension_group,
      (axis_ptr_listnode_destroy_func_t)destroy_extension_group_by_addon);

  axis_extension_thread_t *extension_thread = axis_extension_thread_create();
  axis_extension_thread_attach_to_context_and_group(
      extension_thread, extension_context, extension_group);
  extension_group->extension_thread = extension_thread;

  axis_list_push_ptr_back(
      &extension_context->extension_threads, extension_thread,
      (axis_ptr_listnode_destroy_func_t)
          axis_extension_thread_remove_from_extension_context);

  size_t extension_groups_cnt_of_the_current_app = 0;
  axis_list_foreach (&requester_cmd_start_graph->extension_groups_info, iter) {
    axis_extension_group_info_t *extension_group_info =
        axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(iter.node));

    if (axis_string_is_equal(&extension_group_info->loc.app_uri,
                            &engine->app->uri)) {
      ++extension_groups_cnt_of_the_current_app;
    }
  }

  if (extension_groups_cnt_of_the_current_app ==
      axis_list_size(&extension_context->extension_groups)) {
    // All extension groups are created completed.

    axis_extension_context_add_extensions_info_from_graph(
        extension_context,
        axis_cmd_start_graph_get_extensions_info(requester_cmd));
    axis_extension_context_add_extension_groups_info_from_graph(
        extension_context,
        axis_cmd_start_graph_get_extension_groups_info(requester_cmd));

    extension_group->extension_group_info =
        axis_extension_context_get_extension_group_info_by_name(
            extension_context, axis_app_get_uri(extension_context->engine->app),
            axis_extension_group_get_name(extension_group, true));
    axis_ASSERT(extension_group->extension_group_info, "Should not happen.");

    axis_extension_context_start(extension_context);
  }
}

bool axis_extension_context_start_extension_group(
    axis_extension_context_t *self, axis_shared_ptr_t *requester_cmd,
    axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_context_check_integrity(self, true),
             "Invalid use of extension_context %p.", self);

  axis_ASSERT(requester_cmd && axis_msg_get_type(requester_cmd) ==
                                  axis_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  bool result = true;

  axis_cmd_start_graph_t *start_graph_cmd =
      (axis_cmd_start_graph_t *)axis_msg_get_raw_msg(requester_cmd);

  axis_list_t extension_groups_info = start_graph_cmd->extension_groups_info;

  if (axis_list_is_empty(&extension_groups_info)) {
    axis_extension_context_add_extensions_info_from_graph(
        self, axis_cmd_start_graph_get_extensions_info(requester_cmd));
    axis_extension_context_add_extension_groups_info_from_graph(
        self, axis_cmd_start_graph_get_extension_groups_info(requester_cmd));

    axis_extension_context_start(self);

    goto done;
  }

  axis_engine_t *engine = self->engine;
  axis_ASSERT(engine && axis_engine_check_integrity(engine, true),
             "Should not happen.");

  axis_env_t *axis_env = engine->axis_env;
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");
  axis_ASSERT(axis_env->attach_to == axis_ENV_ATTACH_TO_ENGINE,
             "Should not happen.");

  self->state_requester_cmd = axis_shared_ptr_clone(requester_cmd);

  axis_list_foreach (&extension_groups_info, iter) {
    axis_extension_group_info_t *extension_group_info =
        axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(iter.node));
    axis_ASSERT(extension_group_info, "Invalid argument.");
    axis_ASSERT(axis_extension_group_info_check_integrity(extension_group_info),
               "Invalid use of extension_info %p.", extension_group_info);

    // Check whether the current `extension_group` is located within the current
    // `app`.
    if (axis_string_is_equal(&extension_group_info->loc.app_uri,
                            &self->engine->app->uri)) {
      bool res = axis_addon_create_extension_group(
          axis_env,
          axis_string_get_raw_str(
              &extension_group_info->extension_group_addon_name),
          axis_string_get_raw_str(
              &extension_group_info->loc.extension_group_name),
          (axis_env_addon_create_instance_done_cb_t)
              axis_extension_context_create_extension_group_done,
          NULL);

      if (!res) {
        axis_LOGE(
            "[%s] Failed to start the extension group, because unable to find "
            "the specified extension group addon: %s",
            axis_engine_get_id(self->engine, true),
            axis_string_get_raw_str(
                &extension_group_info->extension_group_addon_name));

        if (err) {
          axis_error_set(err, axis_ERRNO_GENERIC, "Unable to find %s",
                        axis_string_get_raw_str(
                            &extension_group_info->extension_group_addon_name));
        }

        result = false;
        break;
      }
    }
  }

done:
  return result;
}
