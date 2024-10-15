//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_runtime/msg/cmd/start_graph/cmd.h"

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/common/loc.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension/extension_addon_and_instance_name_pair.h"
#include "include_internal/ten_runtime/extension/extension_info/extension_info.h"
#include "include_internal/ten_runtime/extension/msg_dest_info/msg_dest_info.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/extension_group/extension_group_info/extension_group_info.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd/cmd.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd/start_graph/cmd.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd/start_graph/field/field_info.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "ten_runtime/app/app.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_node.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

static ten_cmd_start_graph_t *get_raw_cmd(ten_shared_ptr_t *self) {
  TEN_ASSERT(self && ten_cmd_base_check_integrity(self), "Should not happen.");
  return (ten_cmd_start_graph_t *)ten_shared_ptr_get_data(self);
}

static void ten_raw_cmd_start_graph_destroy(ten_cmd_start_graph_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  ten_raw_cmd_deinit(&self->cmd_hdr);

  ten_list_clear(&self->extension_groups_info);
  ten_list_clear(&self->extensions_info);

  ten_string_deinit(&self->predefined_graph_name);

  TEN_FREE(self);
}

void ten_raw_cmd_start_graph_as_msg_destroy(ten_msg_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  ten_raw_cmd_start_graph_destroy((ten_cmd_start_graph_t *)self);
}

ten_cmd_start_graph_t *ten_raw_cmd_start_graph_create(void) {
  ten_cmd_start_graph_t *self =
      (ten_cmd_start_graph_t *)TEN_MALLOC(sizeof(ten_cmd_start_graph_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  ten_raw_cmd_init((ten_cmd_t *)self, TEN_MSG_TYPE_CMD_START_GRAPH);

  ten_list_init(&self->extension_groups_info);
  ten_list_init(&self->extensions_info);

  self->long_running_mode = false;
  ten_string_init(&self->predefined_graph_name);

  return self;
}

ten_shared_ptr_t *ten_cmd_start_graph_create(void) {
  return ten_shared_ptr_create(ten_raw_cmd_start_graph_create(),
                               ten_raw_cmd_start_graph_destroy);
}

static bool ten_raw_cmd_start_graph_init_from_json(ten_cmd_start_graph_t *self,
                                                   ten_json_t *json,
                                                   ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_cmd_check_integrity((ten_cmd_t *)self),
             "Should not happen.");
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");

  for (size_t i = 0; i < ten_cmd_start_graph_fields_info_size; ++i) {
    ten_msg_get_field_from_json_func_t get_field_from_json =
        ten_cmd_start_graph_fields_info[i].get_field_from_json;
    if (get_field_from_json) {
      if (!get_field_from_json((ten_msg_t *)self, json, err)) {
        return false;
      }
    }
  }

  return true;
}

bool ten_raw_cmd_start_graph_as_msg_init_from_json(ten_msg_t *self,
                                                   ten_json_t *json,
                                                   ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_cmd_check_integrity((ten_cmd_t *)self),
             "Should not happen.");
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");

  return ten_raw_cmd_start_graph_init_from_json((ten_cmd_start_graph_t *)self,
                                                json, err);
}

static ten_cmd_start_graph_t *ten_raw_cmd_start_graph_create_from_json(
    ten_json_t *json, ten_error_t *err) {
  TEN_ASSERT(json, "Should not happen.");

  ten_cmd_start_graph_t *cmd = ten_raw_cmd_start_graph_create();
  TEN_ASSERT(cmd && ten_raw_cmd_check_integrity((ten_cmd_t *)cmd),
             "Should not happen.");

  if (!ten_raw_cmd_start_graph_init_from_json(cmd, json, err)) {
    ten_raw_cmd_start_graph_destroy(cmd);
    return NULL;
  }

  return cmd;
}

ten_msg_t *ten_raw_cmd_start_graph_as_msg_create_from_json(ten_json_t *json,
                                                           ten_error_t *err) {
  TEN_ASSERT(json, "Should not happen.");

  return (ten_msg_t *)ten_raw_cmd_start_graph_create_from_json(json, err);
}

ten_json_t *ten_raw_cmd_start_graph_to_json(ten_msg_t *self, ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_cmd_check_integrity((ten_cmd_t *)self) &&
                 ten_raw_msg_get_type(self) == TEN_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  ten_json_t *json = ten_json_create_object();
  TEN_ASSERT(json, "Should not happen.");

  for (size_t i = 0; i < ten_cmd_start_graph_fields_info_size; ++i) {
    ten_msg_put_field_to_json_func_t put_field_to_json =
        ten_cmd_start_graph_fields_info[i].put_field_to_json;
    if (put_field_to_json) {
      if (!put_field_to_json(
              &(((ten_cmd_start_graph_t *)self)->cmd_hdr.cmd_base_hdr.msg_hdr),
              json, err)) {
        ten_json_destroy(json);
        return NULL;
      }
    }
  }

  return json;
}

ten_msg_t *ten_raw_cmd_start_graph_as_msg_clone(
    ten_msg_t *self, TEN_UNUSED ten_list_t *excluded_field_ids) {
  TEN_ASSERT(self && ten_raw_cmd_base_check_integrity((ten_cmd_base_t *)self),
             "Should not happen.");

  ten_cmd_start_graph_t *cloned_cmd = ten_raw_cmd_start_graph_create();

  for (size_t i = 0; i < ten_cmd_start_graph_fields_info_size; ++i) {
    ten_msg_copy_field_func_t copy_field =
        ten_cmd_start_graph_fields_info[i].copy_field;
    if (copy_field) {
      copy_field((ten_msg_t *)cloned_cmd, self, NULL);
    }
  }

  return (ten_msg_t *)cloned_cmd;
}

ten_list_t *ten_raw_cmd_start_graph_get_extensions_info(
    ten_cmd_start_graph_t *self) {
  TEN_ASSERT(self && ten_raw_cmd_check_integrity((ten_cmd_t *)self) &&
                 ten_raw_msg_get_type((ten_msg_t *)self) ==
                     TEN_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  return &self->extensions_info;
}

ten_list_t *ten_cmd_start_graph_get_extensions_info(ten_shared_ptr_t *self) {
  return ten_raw_cmd_start_graph_get_extensions_info(get_raw_cmd(self));
}

ten_list_t *ten_raw_cmd_start_graph_get_extension_groups_info(
    ten_cmd_start_graph_t *self) {
  TEN_ASSERT(self && ten_raw_cmd_check_integrity((ten_cmd_t *)self) &&
                 ten_raw_msg_get_type((ten_msg_t *)self) ==
                     TEN_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  return &self->extension_groups_info;
}

ten_list_t *ten_cmd_start_graph_get_extension_groups_info(
    ten_shared_ptr_t *self) {
  return ten_raw_cmd_start_graph_get_extension_groups_info(get_raw_cmd(self));
}

static void ten_cmd_start_graph_get_next_list_through_dests(
    ten_shared_ptr_t *self, ten_app_t *app,
    ten_extension_info_t *extension_info, ten_list_t *dests, ten_list_t *next,
    bool from_src_point_of_view) {
  TEN_ASSERT(self && ten_cmd_base_check_integrity(self) && app && next,
             "Should not happen.");

  ten_list_foreach (dests, iter_dest) {
    ten_smart_ptr_t *shared_dest_extension_info =
        ten_smart_ptr_listnode_get(iter_dest.node);
    TEN_ASSERT(shared_dest_extension_info, "Invalid argument.");

    ten_extension_info_t *dest_extension_info =
        ten_extension_info_from_smart_ptr(shared_dest_extension_info);

    const bool equal = ten_string_is_equal_c_str(
        &dest_extension_info->loc.app_uri, ten_app_get_uri(app));
    const bool expected_equality = (from_src_point_of_view ? false : true);

    if (equal == expected_equality) {
      ten_extension_info_t *target_extension_info =
          from_src_point_of_view ? dest_extension_info : extension_info;

      ten_listnode_t *found = ten_list_find_string(
          next, ten_string_get_raw_str(&target_extension_info->loc.app_uri));
      if (!found) {
        // Found a new remote app, add it to the 'next' list.
        ten_list_push_str_back(
            next, ten_string_get_raw_str(&target_extension_info->loc.app_uri));
      }
    }
  }
}

static void ten_cmd_start_graph_get_next_list_per_extension_info(
    ten_shared_ptr_t *self, ten_app_t *app,
    ten_extension_info_t *extension_info, ten_list_t *next,
    bool from_src_point_of_view) {
  TEN_ASSERT(self && ten_cmd_base_check_integrity(self) &&
                 ten_msg_get_type(self) == TEN_MSG_TYPE_CMD_START_GRAPH &&
                 app && next,
             "Should not happen.");

  ten_list_foreach (&extension_info->msg_dest_info.cmd, iter_cmd) {
    ten_msg_dest_info_t *cmd_dest =
        ten_shared_ptr_get_data(ten_smart_ptr_listnode_get(iter_cmd.node));
    ten_cmd_start_graph_get_next_list_through_dests(self, app, extension_info,
                                                    &cmd_dest->dest, next,
                                                    from_src_point_of_view);
  }

  ten_list_foreach (&extension_info->msg_dest_info.video_frame, iter_cmd) {
    ten_msg_dest_info_t *data_dest =
        ten_shared_ptr_get_data(ten_smart_ptr_listnode_get(iter_cmd.node));
    ten_cmd_start_graph_get_next_list_through_dests(self, app, extension_info,
                                                    &data_dest->dest, next,
                                                    from_src_point_of_view);
  }

  ten_list_foreach (&extension_info->msg_dest_info.audio_frame, iter_cmd) {
    ten_msg_dest_info_t *data_dest =
        ten_shared_ptr_get_data(ten_smart_ptr_listnode_get(iter_cmd.node));
    ten_cmd_start_graph_get_next_list_through_dests(self, app, extension_info,
                                                    &data_dest->dest, next,
                                                    from_src_point_of_view);
  }

  ten_list_foreach (&extension_info->msg_dest_info.data, iter_cmd) {
    ten_msg_dest_info_t *data_dest =
        ten_shared_ptr_get_data(ten_smart_ptr_listnode_get(iter_cmd.node));
    ten_cmd_start_graph_get_next_list_through_dests(self, app, extension_info,
                                                    &data_dest->dest, next,
                                                    from_src_point_of_view);
  }
}

// Get the list of the immediate remote apps of the local app.
void ten_cmd_start_graph_get_next_list(ten_shared_ptr_t *self, ten_app_t *app,
                                       ten_list_t *next) {
  TEN_ASSERT(self && ten_cmd_base_check_integrity(self) &&
                 ten_msg_get_type(self) == TEN_MSG_TYPE_CMD_START_GRAPH &&
                 app && next,
             "Should not happen.");

  ten_list_foreach (ten_cmd_start_graph_get_extensions_info(self), iter) {
    ten_extension_info_t *extension_info =
        ten_shared_ptr_get_data(ten_smart_ptr_listnode_get(iter.node));

    if (ten_string_is_equal_c_str(&extension_info->loc.app_uri,
                                  ten_app_get_uri(app))) {
      ten_cmd_start_graph_get_next_list_per_extension_info(
          self, app, extension_info, next, true);
    } else {
      ten_cmd_start_graph_get_next_list_per_extension_info(
          self, app, extension_info, next, false);
    }
  }
}

static void ten_raw_cmd_start_graph_add_missing_extension_group_node(
    ten_cmd_start_graph_t *self) {
  TEN_ASSERT(self && ten_raw_cmd_check_integrity((ten_cmd_t *)self) &&
                 ten_raw_msg_get_type((ten_msg_t *)self) ==
                     TEN_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  ten_list_t *extensions_info = &self->extensions_info;
  ten_list_t *extension_groups_info = &self->extension_groups_info;

  ten_list_foreach (extensions_info, iter_extension) {
    ten_extension_info_t *extension_info = ten_extension_info_from_smart_ptr(
        ten_smart_ptr_listnode_get(iter_extension.node));

    ten_string_t *extension_group_name =
        &extension_info->loc.extension_group_name;
    ten_string_t *app_uri = &extension_info->loc.app_uri;

    bool group_found = false;

    // Check whether the extension_group name specified by the extension has a
    // corresponding extension_group item.
    ten_list_foreach (extension_groups_info, iter_extension_group) {
      ten_extension_group_info_t *extension_group_info =
          ten_extension_group_info_from_smart_ptr(
              ten_smart_ptr_listnode_get(iter_extension_group.node));

      if (ten_string_is_equal(
              extension_group_name,
              &extension_group_info->loc.extension_group_name) &&
          ten_string_is_equal(app_uri, &extension_group_info->loc.app_uri)) {
        group_found = true;
        break;
      }
    }

    if (group_found) {
      continue;
    }

    ten_extension_group_info_t *extension_group_info =
        ten_extension_group_info_create();

    // Create an extension_group item that uses the builtin
    // default_extension_group, allowing the extension's extension_group to be
    // associated with an extension_group addon.
    ten_string_init_formatted(&extension_group_info->extension_group_addon_name,
                              TEN_STR_DEFAULT_EXTENSION_GROUP);

    ten_loc_set(
        &extension_group_info->loc,
        ten_string_get_raw_str(&extension_info->loc.app_uri), "",
        ten_string_get_raw_str(&extension_info->loc.extension_group_name), "");

    ten_shared_ptr_t *shared_group = ten_shared_ptr_create(
        extension_group_info, ten_extension_group_info_destroy);
    ten_list_push_smart_ptr_back(extension_groups_info, shared_group);
    ten_shared_ptr_destroy(shared_group);
  }
}

void ten_cmd_start_graph_add_missing_extension_group_node(
    ten_shared_ptr_t *self) {
  TEN_ASSERT(self && ten_cmd_base_check_integrity(self) &&
                 ten_msg_get_type(self) == TEN_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  ten_raw_cmd_start_graph_add_missing_extension_group_node(get_raw_cmd(self));
}

bool ten_raw_cmd_start_graph_get_long_running_mode(
    ten_cmd_start_graph_t *self) {
  TEN_ASSERT(self && ten_raw_cmd_check_integrity((ten_cmd_t *)self) &&
                 ten_raw_msg_get_type((ten_msg_t *)self) ==
                     TEN_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  return self->long_running_mode;
}

bool ten_cmd_start_graph_get_long_running_mode(ten_shared_ptr_t *self) {
  TEN_ASSERT(self && ten_cmd_base_check_integrity(self) &&
                 ten_msg_get_type(self) == TEN_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  return get_raw_cmd(self)->long_running_mode;
}

ten_string_t *ten_raw_cmd_start_graph_get_predefined_graph_name(
    ten_cmd_start_graph_t *self) {
  TEN_ASSERT(self && ten_raw_cmd_check_integrity((ten_cmd_t *)self) &&
                 ten_raw_msg_get_type((ten_msg_t *)self) ==
                     TEN_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  return &self->predefined_graph_name;
}

ten_string_t *ten_cmd_start_graph_get_predefined_graph_name(
    ten_shared_ptr_t *self) {
  TEN_ASSERT(self && ten_cmd_base_check_integrity(self) &&
                 ten_msg_get_type(self) == TEN_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  return ten_raw_cmd_start_graph_get_predefined_graph_name(get_raw_cmd(self));
}

void ten_cmd_start_graph_fill_loc_info(ten_shared_ptr_t *self,
                                       const char *app_uri,
                                       const char *graph_id) {
  TEN_ASSERT(self && ten_cmd_base_check_integrity(self) &&
                 ten_msg_get_type(self) == TEN_MSG_TYPE_CMD_START_GRAPH &&
                 graph_id && strlen(graph_id),
             "Should not happen.");

  ten_extensions_info_fill_loc_info(
      ten_cmd_start_graph_get_extensions_info(self), app_uri, graph_id);
  ten_extension_groups_info_fill_graph_id(
      ten_cmd_start_graph_get_extension_groups_info(self), graph_id);
}

ten_list_t
ten_cmd_start_graph_get_extension_addon_and_instance_name_pairs_of_specified_extension_group(
    ten_shared_ptr_t *self, const char *app_uri, const char *graph_id,
    const char *extension_group_name) {
  TEN_ASSERT(self && ten_cmd_base_check_integrity(self) &&
                 ten_msg_get_type(self) == TEN_MSG_TYPE_CMD_START_GRAPH &&
                 app_uri && graph_id && extension_group_name,
             "Should not happen.");

  ten_list_t result = TEN_LIST_INIT_VAL;

  ten_list_t *extensions_info = ten_cmd_start_graph_get_extensions_info(self);

  ten_list_foreach (extensions_info, iter) {
    ten_shared_ptr_t *shared_extension_info =
        ten_smart_ptr_listnode_get(iter.node);

    ten_extension_info_t *extension_info =
        ten_shared_ptr_get_data(shared_extension_info);
    TEN_ASSERT(extension_info, "Invalid argument.");
    // TEN_NOLINTNEXTLINE(thread-check)
    // thread-check: The graph-related information of the extension remains
    // unchanged during the lifecycle of engine/graph, allowing safe
    // cross-thread access.
    TEN_ASSERT(ten_extension_info_check_integrity(extension_info, false),
               "Invalid use of extension_info %p.", extension_info);

    if (ten_string_is_equal_c_str(&extension_info->loc.app_uri, app_uri) &&
        ten_string_is_equal_c_str(&extension_info->loc.graph_id, graph_id) &&
        ten_string_is_equal_c_str(&extension_info->loc.extension_group_name,
                                  extension_group_name)) {
      ten_extension_addon_and_instance_name_pair_t *extension_name_info =
          ten_extension_addon_and_instance_name_pair_create(
              ten_string_get_raw_str(&extension_info->extension_addon_name),
              ten_string_get_raw_str(&extension_info->loc.extension_name));

      ten_list_push_ptr_back(
          &result, extension_name_info,
          (ten_ptr_listnode_destroy_func_t)
              ten_extension_addon_and_instance_name_pair_destroy);
    }
  }

  return result;
}

ten_list_t ten_cmd_start_graph_get_requested_extension_names(
    ten_shared_ptr_t *self) {
  TEN_ASSERT(self && ten_cmd_base_check_integrity(self) &&
                 ten_msg_get_type(self) == TEN_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  ten_list_t requested_extension_names = TEN_LIST_INIT_VAL;

  ten_list_t *requested_extensions_info =
      ten_cmd_start_graph_get_extensions_info(self);

  ten_list_foreach (requested_extensions_info, iter) {
    ten_extension_info_t *requested_extension_info =
        ten_shared_ptr_get_data(ten_smart_ptr_listnode_get(iter.node));
    TEN_ASSERT(requested_extension_info && ten_extension_info_check_integrity(
                                               requested_extension_info, true),
               "Should not happen.");

    ten_string_t *requested_extension_name =
        &requested_extension_info->loc.extension_name;
    TEN_ASSERT(ten_string_len(requested_extension_name), "Should not happen.");

    ten_list_push_str_back(&requested_extension_names,
                           ten_string_get_raw_str(requested_extension_name));
  }

  return requested_extension_names;
}
