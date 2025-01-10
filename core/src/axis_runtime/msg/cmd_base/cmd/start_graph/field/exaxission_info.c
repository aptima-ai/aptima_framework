//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension/extension_info/extension_info.h"

#include <complex.h>
#include <string.h>

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/common/loc.h"
#include "include_internal/axis_runtime/extension/extension_info/json.h"
#include "include_internal/axis_runtime/extension/extension_info/value.h"
#include "include_internal/axis_runtime/extension_group/extension_group_info/extension_group_info.h"
#include "include_internal/axis_runtime/extension_group/extension_group_info/json.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/cmd.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/start_graph/cmd.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_utils/log/log.h"
#include "axis_runtime/common/errno.h"
#include "axis_runtime/msg/cmd/start_graph/cmd.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_str.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_is.h"

static axis_value_t *axis_cmd_start_graph_extensions_info_to_value(
    axis_msg_t *self, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  axis_cmd_start_graph_t *cmd = (axis_cmd_start_graph_t *)self;
  axis_list_t *extensions_info =
      axis_raw_cmd_start_graph_get_extensions_info(cmd);

  // Convert the core part of the start_graph command into axis_value_t, which is
  // an object-type value containing two key-value pairs: "nodes" and
  // "connections". The snippet is as follows:
  //
  // ------------------------
  // {
  //   "nodes": [
  //     ...
  //   ],
  //   "connections": [
  //     ...
  //   ]
  // }
  // ------------------------
  axis_list_t value_object_kv_list = axis_LIST_INIT_VAL;

  axis_list_t nodes_list = axis_LIST_INIT_VAL;
  axis_list_t connections_list = axis_LIST_INIT_VAL;

  axis_list_t unique_extension_list = axis_LIST_INIT_VAL;
  axis_list_foreach (extensions_info, iter) {
    axis_extension_info_t *extension_info =
        axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(iter.node));
    axis_ASSERT(extension_info, "Should not happen.");

    axis_string_t loc_str;
    axis_string_init(&loc_str);
    axis_loc_to_string(&extension_info->loc, &loc_str);

    if (axis_list_find_string(&unique_extension_list,
                             axis_string_get_raw_str(&loc_str)) != NULL) {
      axis_LOGE("Extension %s is duplicated.", axis_string_get_raw_str(&loc_str));

      if (err) {
        axis_error_set(err, axis_ERRNO_GENERIC, "Extension %s is duplicated.",
                      axis_string_get_raw_str(&loc_str));
      }

      axis_string_deinit(&loc_str);
      axis_list_clear(&unique_extension_list);
      return NULL;
    }

    axis_list_push_str_back(&unique_extension_list,
                           axis_string_get_raw_str(&loc_str));
    axis_string_deinit(&loc_str);

    axis_value_t *extension_info_value =
        axis_extension_info_node_to_value(extension_info, err);
    if (!extension_info_value) {
      axis_list_clear(&unique_extension_list);
      return NULL;
    }

    axis_list_push_ptr_back(&nodes_list, extension_info_value,
                           (axis_ptr_listnode_destroy_func_t)axis_value_destroy);
  }

  axis_list_clear(&unique_extension_list);

  axis_list_push_ptr_back(
      &value_object_kv_list,
      axis_value_kv_create(axis_STR_NODES,
                          axis_value_create_array_with_move(&nodes_list)),
      (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);

  axis_list_foreach (extensions_info, iter) {
    axis_extension_info_t *extension_info =
        axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(iter.node));
    axis_ASSERT(extension_info, "Should not happen.");

    axis_value_t *extension_info_connections_value =
        axis_extension_info_connection_to_value(extension_info, err);
    if (extension_info_connections_value) {
      axis_list_push_ptr_back(
          &connections_list, extension_info_connections_value,
          (axis_ptr_listnode_destroy_func_t)axis_value_destroy);
    }
  }

  axis_list_push_ptr_back(
      &value_object_kv_list,
      axis_value_kv_create(axis_STR_CONNECTIONS,
                          axis_value_create_array_with_move(&connections_list)),
      (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);

  axis_value_t *value = axis_value_create_object_with_move(&value_object_kv_list);

  return value;
}

bool axis_cmd_start_graph_copy_extensions_info(
    axis_msg_t *self, axis_msg_t *src, axis_UNUSED axis_list_t *excluded_field_ids,
    axis_error_t *err) {
  axis_ASSERT(self && src && axis_raw_cmd_check_integrity((axis_cmd_t *)src) &&
                 axis_raw_msg_get_type(src) == axis_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  axis_cmd_start_graph_t *src_cmd = (axis_cmd_start_graph_t *)src;
  axis_cmd_start_graph_t *self_cmd = (axis_cmd_start_graph_t *)self;

  axis_list_foreach (&src_cmd->extension_groups_info, iter) {
    axis_shared_ptr_t *extension_group_info_ =
        axis_smart_ptr_listnode_get(iter.node);
    axis_extension_group_info_t *extension_group_info =
        axis_extension_group_info_from_smart_ptr(extension_group_info_);

    axis_UNUSED bool rc = axis_extension_group_info_clone(
        extension_group_info, &self_cmd->extension_groups_info);
    axis_ASSERT(rc, "Should not happen.");
  }

  if (!axis_extensions_info_clone(&src_cmd->extensions_info,
                                 &self_cmd->extensions_info, err)) {
    return false;
  }

  return true;
}

bool axis_cmd_start_graph_process_extensions_info(
    axis_msg_t *self, axis_raw_msg_process_one_field_func_t cb, void *user_data,
    axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  axis_cmd_start_graph_t *cmd = (axis_cmd_start_graph_t *)self;

  axis_value_t *extensions_info_value =
      axis_cmd_start_graph_extensions_info_to_value(self, err);

  axis_value_t *nodes_value =
      axis_value_object_peek(extensions_info_value, axis_STR_NODES);
  axis_ASSERT(axis_value_is_array(nodes_value), "Should not happen.");

  axis_value_t *connections_value =
      axis_value_object_peek(extensions_info_value, axis_STR_CONNECTIONS);
  axis_ASSERT(axis_value_is_array(connections_value), "Should not happen.");

  axis_msg_field_process_data_t extensions_info_nodes_field;
  axis_msg_field_process_data_init(&extensions_info_nodes_field, axis_STR_NODES,
                                  nodes_value, false);

  bool rc = cb(self, &extensions_info_nodes_field, user_data, err);
  if (!rc) {
    goto error;
  }

  if (extensions_info_nodes_field.value_is_changed_after_process) {
    axis_value_array_foreach(nodes_value, iter) {
      axis_value_t *node_value = axis_ptr_listnode_get(iter.node);
      if (!axis_value_is_object(node_value)) {
        goto error;
      }

      if (axis_extension_info_node_from_value(
              node_value, axis_raw_cmd_start_graph_get_extensions_info(cmd),
              err) == NULL) {
        goto error;
      }
    }
  }

  axis_msg_field_process_data_t extensions_info_connections_field;
  axis_msg_field_process_data_init(&extensions_info_connections_field,
                                  axis_STR_CONNECTIONS, connections_value,
                                  false);

  rc = cb(self, &extensions_info_connections_field, user_data, err);
  if (!rc) {
    goto error;
  }

  if (extensions_info_connections_field.value_is_changed_after_process) {
    axis_value_array_foreach(connections_value, iter) {
      axis_value_t *item_value = axis_ptr_listnode_get(iter.node);
      if (!axis_value_is_object(item_value)) {
        goto error;
      }

      if (axis_extension_info_parse_connection_src_part_from_value(
              item_value, axis_raw_cmd_start_graph_get_extensions_info(cmd),
              err) == NULL) {
        goto error;
      }
    }
  }

  axis_value_destroy(extensions_info_value);
  return true;

error:
  axis_value_destroy(extensions_info_value);
  return false;
}
