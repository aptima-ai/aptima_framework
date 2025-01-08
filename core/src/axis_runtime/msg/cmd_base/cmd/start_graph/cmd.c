//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/msg/cmd/start_graph/cmd.h"

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/common/loc.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension/extension_addon_and_instance_name_pair.h"
#include "include_internal/axis_runtime/extension/extension_info/extension_info.h"
#include "include_internal/axis_runtime/extension/msg_dest_info/msg_dest_info.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/extension_group/extension_group_info/extension_group_info.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/cmd.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/start_graph/cmd.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/start_graph/field/field_info.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_utils/value/value_set.h"
#include "axis_runtime/app/app.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_get.h"

static axis_cmd_start_graph_t *get_raw_cmd(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_cmd_base_check_integrity(self), "Should not happen.");
  return (axis_cmd_start_graph_t *)axis_shared_ptr_get_data(self);
}

static void axis_raw_cmd_start_graph_destroy(axis_cmd_start_graph_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_raw_cmd_deinit(&self->cmd_hdr);

  axis_list_clear(&self->extension_groups_info);
  axis_list_clear(&self->extensions_info);

  axis_value_deinit(&self->long_running_mode);
  axis_value_deinit(&self->predefined_graph_name);

  axis_FREE(self);
}

void axis_raw_cmd_start_graph_as_msg_destroy(axis_msg_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_raw_cmd_start_graph_destroy((axis_cmd_start_graph_t *)self);
}

axis_cmd_start_graph_t *axis_raw_cmd_start_graph_create(void) {
  axis_cmd_start_graph_t *self =
      (axis_cmd_start_graph_t *)axis_MALLOC(sizeof(axis_cmd_start_graph_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_raw_cmd_init((axis_cmd_t *)self, axis_MSG_TYPE_CMD_START_GRAPH);

  axis_list_init(&self->extension_groups_info);
  axis_list_init(&self->extensions_info);

  axis_value_init_bool(&self->long_running_mode, false);
  axis_value_init_string(&self->predefined_graph_name);

  return self;
}

axis_shared_ptr_t *axis_cmd_start_graph_create(void) {
  return axis_shared_ptr_create(axis_raw_cmd_start_graph_create(),
                               axis_raw_cmd_start_graph_destroy);
}

static bool axis_raw_cmd_start_graph_as_msg_get_graph_from_json(
    axis_msg_t *self, axis_msg_field_process_data_t *field, void *user_data,
    axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(field, "Should not happen.");
  axis_ASSERT(
      field->field_value && axis_value_check_integrity(field->field_value),
      "Should not happen.");

  if (axis_c_string_is_equal(field->field_name, axis_STR_NODES) ||
      axis_c_string_is_equal(field->field_name, axis_STR_CONNECTIONS)) {
    axis_json_t *json = (axis_json_t *)user_data;
    axis_ASSERT(json, "Should not happen.");

    json = axis_json_object_peek(json, field->field_name);
    if (!json) {
      // Some fields are optional, and it is allowed for the corresponding
      // JSON block to be absent during deserialization.
      return true;
    }

    if (!axis_value_set_from_json(field->field_value, json)) {
      // If the field value cannot be set from the JSON, it means that the
      // JSON format is incorrect.
      if (err) {
        axis_error_set(err, axis_ERRNO_INVALID_JSON,
                      "Invalid JSON format for field %s.", field->field_name);
      }

      return false;
    }
  }

  // During JSON deserialization, the field value may be modified, so we set the
  // value_is_changed_after_process flag.
  field->value_is_changed_after_process = true;

  return true;
}

bool axis_raw_cmd_start_graph_init_from_json(axis_cmd_start_graph_t *self,
                                            axis_json_t *json,
                                            axis_error_t *err) {
  axis_ASSERT(self && axis_raw_cmd_check_integrity((axis_cmd_t *)self),
             "Should not happen.");
  axis_ASSERT(json && axis_json_check_integrity(json), "Should not happen.");

  return axis_raw_cmd_start_graph_loop_all_fields(
      (axis_msg_t *)self,
      axis_raw_msg_get_one_field_from_json_include_internal_field, json, err);
}

bool axis_raw_cmd_start_graph_set_graph_from_json(axis_cmd_start_graph_t *self,
                                                 axis_json_t *json,
                                                 axis_error_t *err) {
  axis_ASSERT(self && axis_raw_cmd_check_integrity((axis_cmd_t *)self),
             "Should not happen.");
  axis_ASSERT(json && axis_json_check_integrity(json), "Should not happen.");

  return axis_raw_cmd_start_graph_loop_all_fields(
      (axis_msg_t *)self, axis_raw_cmd_start_graph_as_msg_get_graph_from_json,
      json, err);
}

static bool axis_raw_cmd_start_graph_set_graph_from_json_str(
    axis_msg_t *self, const char *json_str, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_cmd_check_integrity((axis_cmd_t *)self),
             "Invalid argument.");
  axis_ASSERT(json_str, "Invalid argument.");

  axis_json_t *json = axis_json_from_string(json_str, err);
  if (!json) {
    return false;
  }

  bool rc = axis_raw_cmd_start_graph_set_graph_from_json(
      (axis_cmd_start_graph_t *)self, json, err);

  axis_json_destroy(json);

  return rc;
}

bool axis_cmd_start_graph_set_graph_from_json_str(axis_shared_ptr_t *self,
                                                 const char *json_str,
                                                 axis_error_t *err) {
  axis_ASSERT(self && axis_cmd_check_integrity(self), "Invalid argument.");
  axis_ASSERT(json_str, "Invalid argument.");

  return axis_raw_cmd_start_graph_set_graph_from_json_str(
      axis_msg_get_raw_msg(self), json_str, err);
}

axis_json_t *axis_raw_cmd_start_graph_to_json(axis_msg_t *self, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_cmd_check_integrity((axis_cmd_t *)self) &&
                 axis_raw_msg_get_type(self) == axis_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  axis_json_t *json = axis_json_create_object();
  axis_ASSERT(json, "Should not happen.");

  if (!axis_raw_cmd_start_graph_loop_all_fields(
          self, axis_raw_msg_put_one_field_to_json, json, err)) {
    axis_json_destroy(json);
    return NULL;
  }

  return json;
}

axis_msg_t *axis_raw_cmd_start_graph_as_msg_clone(
    axis_msg_t *self, axis_UNUSED axis_list_t *excluded_field_ids) {
  axis_ASSERT(self && axis_raw_cmd_base_check_integrity((axis_cmd_base_t *)self),
             "Should not happen.");

  axis_cmd_start_graph_t *cloned_cmd = axis_raw_cmd_start_graph_create();

  for (size_t i = 0; i < axis_cmd_start_graph_fields_info_size; ++i) {
    axis_msg_copy_field_func_t copy_field =
        axis_cmd_start_graph_fields_info[i].copy_field;
    if (copy_field) {
      copy_field((axis_msg_t *)cloned_cmd, self, NULL);
    }
  }

  return (axis_msg_t *)cloned_cmd;
}

axis_list_t *axis_raw_cmd_start_graph_get_extensions_info(
    axis_cmd_start_graph_t *self) {
  axis_ASSERT(self && axis_raw_cmd_check_integrity((axis_cmd_t *)self) &&
                 axis_raw_msg_get_type((axis_msg_t *)self) ==
                     axis_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  return &self->extensions_info;
}

axis_list_t *axis_cmd_start_graph_get_extensions_info(axis_shared_ptr_t *self) {
  return axis_raw_cmd_start_graph_get_extensions_info(get_raw_cmd(self));
}

axis_list_t *axis_raw_cmd_start_graph_get_extension_groups_info(
    axis_cmd_start_graph_t *self) {
  axis_ASSERT(self && axis_raw_cmd_check_integrity((axis_cmd_t *)self) &&
                 axis_raw_msg_get_type((axis_msg_t *)self) ==
                     axis_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  return &self->extension_groups_info;
}

axis_list_t *axis_cmd_start_graph_get_extension_groups_info(
    axis_shared_ptr_t *self) {
  return axis_raw_cmd_start_graph_get_extension_groups_info(get_raw_cmd(self));
}

static void axis_cmd_start_graph_collect_connectable_apps(
    axis_shared_ptr_t *self, axis_app_t *app,
    axis_extension_info_t *extension_info, axis_list_t *dests, axis_list_t *next,
    bool from_src_point_of_view) {
  axis_ASSERT(self && axis_cmd_base_check_integrity(self) && app && next,
             "Should not happen.");

  axis_list_foreach (dests, iter_dest) {
    axis_smart_ptr_t *shared_dest_extension_info =
        axis_smart_ptr_listnode_get(iter_dest.node);
    axis_ASSERT(shared_dest_extension_info, "Invalid argument.");

    axis_extension_info_t *dest_extension_info =
        axis_extension_info_from_smart_ptr(shared_dest_extension_info);

    const bool equal = axis_string_is_equal_c_str(
        &dest_extension_info->loc.app_uri, axis_app_get_uri(app));
    const bool expected_equality = (from_src_point_of_view ? false : true);

    if (equal == expected_equality) {
      axis_extension_info_t *target_extension_info =
          from_src_point_of_view ? dest_extension_info : extension_info;

      // If the URI of the target app represents a client URI, it means that the
      // target app cannot allow other apps to actively connect to it (e.g., it
      // does not have a listening port open). Instead, it can only initiate
      // connections to other apps. Therefore, what this app should do is avoid
      // connecting to the target app actively and instead wait for it to
      // initiate the connection.
      if (axis_string_starts_with(&target_extension_info->loc.app_uri,
                                 axis_STR_CLIENT)) {
        continue;
      }

      axis_listnode_t *found = axis_list_find_string(
          next, axis_string_get_raw_str(&target_extension_info->loc.app_uri));
      if (!found) {
        // Found a new remote app, add it to the 'next' list.
        axis_list_push_str_back(
            next, axis_string_get_raw_str(&target_extension_info->loc.app_uri));
      }
    }
  }
}

static void axis_cmd_start_graph_collect_all_connectable_apps(
    axis_shared_ptr_t *self, axis_app_t *app,
    axis_extension_info_t *extension_info, axis_list_t *next,
    bool from_src_point_of_view) {
  axis_ASSERT(self && axis_cmd_base_check_integrity(self) &&
                 axis_msg_get_type(self) == axis_MSG_TYPE_CMD_START_GRAPH &&
                 app && next,
             "Should not happen.");

  axis_list_foreach (&extension_info->msg_dest_info.cmd, iter_cmd) {
    axis_msg_dest_info_t *cmd_dest =
        axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(iter_cmd.node));
    axis_cmd_start_graph_collect_connectable_apps(self, app, extension_info,
                                                 &cmd_dest->dest, next,
                                                 from_src_point_of_view);
  }

  axis_list_foreach (&extension_info->msg_dest_info.video_frame, iter_cmd) {
    axis_msg_dest_info_t *data_dest =
        axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(iter_cmd.node));
    axis_cmd_start_graph_collect_connectable_apps(self, app, extension_info,
                                                 &data_dest->dest, next,
                                                 from_src_point_of_view);
  }

  axis_list_foreach (&extension_info->msg_dest_info.audio_frame, iter_cmd) {
    axis_msg_dest_info_t *data_dest =
        axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(iter_cmd.node));
    axis_cmd_start_graph_collect_connectable_apps(self, app, extension_info,
                                                 &data_dest->dest, next,
                                                 from_src_point_of_view);
  }

  axis_list_foreach (&extension_info->msg_dest_info.data, iter_cmd) {
    axis_msg_dest_info_t *data_dest =
        axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(iter_cmd.node));
    axis_cmd_start_graph_collect_connectable_apps(self, app, extension_info,
                                                 &data_dest->dest, next,
                                                 from_src_point_of_view);
  }
}

// Get the list of the immediate remote apps of the local app.
void axis_cmd_start_graph_collect_all_immediate_connectable_apps(
    axis_shared_ptr_t *self, axis_app_t *app, axis_list_t *next) {
  axis_ASSERT(self && axis_cmd_base_check_integrity(self) &&
                 axis_msg_get_type(self) == axis_MSG_TYPE_CMD_START_GRAPH &&
                 app && next,
             "Should not happen.");

  axis_list_foreach (axis_cmd_start_graph_get_extensions_info(self), iter) {
    axis_extension_info_t *extension_info =
        axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(iter.node));

    if (axis_string_is_equal_c_str(&extension_info->loc.app_uri,
                                  axis_app_get_uri(app))) {
      axis_cmd_start_graph_collect_all_connectable_apps(
          self, app, extension_info, next, true);
    } else {
      axis_cmd_start_graph_collect_all_connectable_apps(
          self, app, extension_info, next, false);
    }
  }
}

static void axis_raw_cmd_start_graph_add_missing_extension_group_node(
    axis_cmd_start_graph_t *self) {
  axis_ASSERT(self && axis_raw_cmd_check_integrity((axis_cmd_t *)self) &&
                 axis_raw_msg_get_type((axis_msg_t *)self) ==
                     axis_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  axis_list_t *extensions_info = &self->extensions_info;
  axis_list_t *extension_groups_info = &self->extension_groups_info;

  axis_list_foreach (extensions_info, iter_extension) {
    axis_extension_info_t *extension_info = axis_extension_info_from_smart_ptr(
        axis_smart_ptr_listnode_get(iter_extension.node));

    axis_string_t *extension_group_name =
        &extension_info->loc.extension_group_name;
    axis_string_t *app_uri = &extension_info->loc.app_uri;

    bool group_found = false;

    // Check whether the extension_group name specified by the extension has a
    // corresponding extension_group item.
    axis_list_foreach (extension_groups_info, iter_extension_group) {
      axis_extension_group_info_t *extension_group_info =
          axis_extension_group_info_from_smart_ptr(
              axis_smart_ptr_listnode_get(iter_extension_group.node));

      if (axis_string_is_equal(
              extension_group_name,
              &extension_group_info->loc.extension_group_name) &&
          axis_string_is_equal(app_uri, &extension_group_info->loc.app_uri)) {
        group_found = true;
        break;
      }
    }

    if (group_found) {
      continue;
    }

    axis_extension_group_info_t *extension_group_info =
        axis_extension_group_info_create();

    // Create an extension_group item that uses the builtin
    // default_extension_group, allowing the extension's extension_group to be
    // associated with an extension_group addon.
    axis_string_set_formatted(&extension_group_info->extension_group_addon_name,
                             axis_STR_DEFAULT_EXTENSION_GROUP);

    axis_loc_set(
        &extension_group_info->loc,
        axis_string_get_raw_str(&extension_info->loc.app_uri), "",
        axis_string_get_raw_str(&extension_info->loc.extension_group_name), "");

    axis_shared_ptr_t *shared_group = axis_shared_ptr_create(
        extension_group_info, axis_extension_group_info_destroy);
    axis_list_push_smart_ptr_back(extension_groups_info, shared_group);
    axis_shared_ptr_destroy(shared_group);
  }
}

void axis_cmd_start_graph_add_missing_extension_group_node(
    axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_cmd_base_check_integrity(self) &&
                 axis_msg_get_type(self) == axis_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  axis_raw_cmd_start_graph_add_missing_extension_group_node(get_raw_cmd(self));
}

bool axis_raw_cmd_start_graph_get_long_running_mode(
    axis_cmd_start_graph_t *self) {
  axis_ASSERT(self && axis_raw_cmd_check_integrity((axis_cmd_t *)self) &&
                 axis_raw_msg_get_type((axis_msg_t *)self) ==
                     axis_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  return axis_value_get_bool(&self->long_running_mode, NULL);
}

bool axis_cmd_start_graph_get_long_running_mode(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_cmd_base_check_integrity(self) &&
                 axis_msg_get_type(self) == axis_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  return axis_raw_cmd_start_graph_get_long_running_mode(get_raw_cmd(self));
}

bool axis_cmd_start_graph_set_predefined_graph_name(
    axis_shared_ptr_t *self, const char *predefined_graph_name,
    axis_error_t *err) {
  axis_ASSERT(self && axis_cmd_base_check_integrity(self) &&
                 axis_msg_get_type(self) == axis_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  return axis_value_set_string(&get_raw_cmd(self)->predefined_graph_name,
                              predefined_graph_name);
}

bool axis_cmd_start_graph_set_long_running_mode(axis_shared_ptr_t *self,
                                               bool long_running_mode,
                                               axis_error_t *err) {
  axis_ASSERT(self && axis_cmd_base_check_integrity(self) &&
                 axis_msg_get_type(self) == axis_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  return axis_value_set_bool(&get_raw_cmd(self)->long_running_mode,
                            long_running_mode);
}

axis_string_t *axis_raw_cmd_start_graph_get_predefined_graph_name(
    axis_cmd_start_graph_t *self) {
  axis_ASSERT(self && axis_raw_cmd_check_integrity((axis_cmd_t *)self) &&
                 axis_raw_msg_get_type((axis_msg_t *)self) ==
                     axis_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  return axis_value_peek_string(&self->predefined_graph_name);
}

axis_string_t *axis_cmd_start_graph_get_predefined_graph_name(
    axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_cmd_base_check_integrity(self) &&
                 axis_msg_get_type(self) == axis_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  return axis_raw_cmd_start_graph_get_predefined_graph_name(get_raw_cmd(self));
}

void axis_cmd_start_graph_fill_loc_info(axis_shared_ptr_t *self,
                                       const char *app_uri,
                                       const char *graph_id) {
  axis_ASSERT(self && axis_cmd_base_check_integrity(self) &&
                 axis_msg_get_type(self) == axis_MSG_TYPE_CMD_START_GRAPH &&
                 graph_id && strlen(graph_id),
             "Should not happen.");

  axis_extensions_info_fill_loc_info(
      axis_cmd_start_graph_get_extensions_info(self), app_uri, graph_id);
  axis_extension_groups_info_fill_graph_id(
      axis_cmd_start_graph_get_extension_groups_info(self), graph_id);
}

axis_list_t
axis_cmd_start_graph_get_extension_addon_and_instance_name_pairs_of_specified_extension_group(
    axis_shared_ptr_t *self, const char *app_uri, const char *graph_id,
    const char *extension_group_name) {
  axis_ASSERT(self && axis_cmd_base_check_integrity(self) &&
                 axis_msg_get_type(self) == axis_MSG_TYPE_CMD_START_GRAPH &&
                 app_uri && graph_id && extension_group_name,
             "Should not happen.");

  axis_list_t result = axis_LIST_INIT_VAL;

  axis_list_t *extensions_info = axis_cmd_start_graph_get_extensions_info(self);

  axis_list_foreach (extensions_info, iter) {
    axis_shared_ptr_t *shared_extension_info =
        axis_smart_ptr_listnode_get(iter.node);

    axis_extension_info_t *extension_info =
        axis_shared_ptr_get_data(shared_extension_info);
    axis_ASSERT(extension_info, "Invalid argument.");
    // axis_NOLINTNEXTLINE(thread-check)
    // thread-check: The graph-related information of the extension remains
    // unchanged during the lifecycle of engine/graph, allowing safe
    // cross-thread access.
    axis_ASSERT(axis_extension_info_check_integrity(extension_info, false),
               "Invalid use of extension_info %p.", extension_info);

    if (axis_string_is_equal_c_str(&extension_info->loc.app_uri, app_uri) &&
        axis_string_is_equal_c_str(&extension_info->loc.graph_id, graph_id) &&
        axis_string_is_equal_c_str(&extension_info->loc.extension_group_name,
                                  extension_group_name)) {
      axis_extension_addon_and_instance_name_pair_t *extension_name_info =
          axis_extension_addon_and_instance_name_pair_create(
              axis_string_get_raw_str(&extension_info->extension_addon_name),
              axis_string_get_raw_str(&extension_info->loc.extension_name));

      axis_list_push_ptr_back(
          &result, extension_name_info,
          (axis_ptr_listnode_destroy_func_t)
              axis_extension_addon_and_instance_name_pair_destroy);
    }
  }

  return result;
}

axis_list_t axis_cmd_start_graph_get_requested_extension_names(
    axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_cmd_base_check_integrity(self) &&
                 axis_msg_get_type(self) == axis_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  axis_list_t requested_extension_names = axis_LIST_INIT_VAL;

  axis_list_t *requested_extensions_info =
      axis_cmd_start_graph_get_extensions_info(self);

  axis_list_foreach (requested_extensions_info, iter) {
    axis_extension_info_t *requested_extension_info =
        axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(iter.node));
    axis_ASSERT(requested_extension_info && axis_extension_info_check_integrity(
                                               requested_extension_info, true),
               "Should not happen.");

    axis_string_t *requested_extension_name =
        &requested_extension_info->loc.extension_name;
    axis_ASSERT(axis_string_len(requested_extension_name), "Should not happen.");

    axis_list_push_str_back(&requested_extension_names,
                           axis_string_get_raw_str(requested_extension_name));
  }

  return requested_extension_names;
}

bool axis_raw_cmd_start_graph_loop_all_fields(
    axis_msg_t *self, axis_raw_msg_process_one_field_func_t cb, void *user_data,
    axis_error_t *err) {
  axis_ASSERT(self && axis_raw_cmd_check_integrity((axis_cmd_t *)self) && cb,
             "Should not happen.");

  for (size_t i = 0; i < axis_cmd_start_graph_fields_info_size; ++i) {
    axis_msg_process_field_func_t process_field =
        axis_cmd_start_graph_fields_info[i].process_field;
    if (process_field) {
      if (!process_field(self, cb, user_data, err)) {
        return false;
      }
    }
  }

  return true;
}
