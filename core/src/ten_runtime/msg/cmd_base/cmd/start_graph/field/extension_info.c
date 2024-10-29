//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/extension/extension_info/extension_info.h"

#include <complex.h>
#include <string.h>

#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/common/loc.h"
#include "include_internal/ten_runtime/extension/extension_info/json.h"
#include "include_internal/ten_runtime/extension/extension_info/value.h"
#include "include_internal/ten_runtime/extension_group/extension_group_info/extension_group_info.h"
#include "include_internal/ten_runtime/extension_group/extension_group_info/json.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd/cmd.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd/start_graph/cmd.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_utils/log/log.h"
#include "ten_runtime/common/errno.h"
#include "ten_runtime/msg/cmd/start_graph/cmd.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_str.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/log/log.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"
#include "ten_utils/value/value.h"
#include "ten_utils/value/value_is.h"

ten_value_t *ten_cmd_start_graph_extensions_info_to_value(ten_msg_t *self,
                                                          ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");

  ten_cmd_start_graph_t *cmd = (ten_cmd_start_graph_t *)self;
  ten_list_t *extensions_info =
      ten_raw_cmd_start_graph_get_extensions_info(cmd);

  // Convert the core part of the start_graph command into ten_value_t, which is
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
  ten_list_t value_object_kv_list = TEN_LIST_INIT_VAL;

  ten_list_t nodes_list = TEN_LIST_INIT_VAL;
  ten_list_t connections_list = TEN_LIST_INIT_VAL;

  ten_list_t unique_extension_list = TEN_LIST_INIT_VAL;
  ten_list_foreach (extensions_info, iter) {
    ten_extension_info_t *extension_info =
        ten_shared_ptr_get_data(ten_smart_ptr_listnode_get(iter.node));
    TEN_ASSERT(extension_info, "Should not happen.");

    ten_string_t loc_str;
    ten_string_init(&loc_str);
    ten_loc_to_string(&extension_info->loc, &loc_str);

    if (ten_list_find_string(&unique_extension_list,
                             ten_string_get_raw_str(&loc_str)) != NULL) {
      TEN_LOGE("Extension %s is duplicated.", ten_string_get_raw_str(&loc_str));

      if (err) {
        ten_error_set(err, TEN_ERRNO_GENERIC, "Extension %s is duplicated.",
                      ten_string_get_raw_str(&loc_str));
      }

      ten_string_deinit(&loc_str);
      ten_list_clear(&unique_extension_list);
      return NULL;
    }

    ten_list_push_str_back(&unique_extension_list,
                           ten_string_get_raw_str(&loc_str));
    ten_string_deinit(&loc_str);

    ten_value_t *extension_info_value =
        ten_extension_info_node_to_value(extension_info, err);
    if (!extension_info_value) {
      ten_list_clear(&unique_extension_list);
      return NULL;
    }

    ten_list_push_ptr_back(&nodes_list, extension_info_value,
                           (ten_ptr_listnode_destroy_func_t)ten_value_destroy);
  }

  ten_list_clear(&unique_extension_list);

  ten_list_push_ptr_back(
      &value_object_kv_list,
      ten_value_kv_create(TEN_STR_NODES,
                          ten_value_create_array_with_move(&nodes_list)),
      (ten_ptr_listnode_destroy_func_t)ten_value_kv_destroy);

  ten_list_foreach (extensions_info, iter) {
    ten_extension_info_t *extension_info =
        ten_shared_ptr_get_data(ten_smart_ptr_listnode_get(iter.node));
    TEN_ASSERT(extension_info, "Should not happen.");

    ten_value_t *extension_info_connections_value =
        ten_extension_info_connection_to_value(extension_info, err);
    if (extension_info_connections_value) {
      ten_list_push_ptr_back(
          &connections_list, extension_info_connections_value,
          (ten_ptr_listnode_destroy_func_t)ten_value_destroy);
    }
  }

  ten_list_push_ptr_back(
      &value_object_kv_list,
      ten_value_kv_create(TEN_STR_CONNECTIONS,
                          ten_value_create_array_with_move(&connections_list)),
      (ten_ptr_listnode_destroy_func_t)ten_value_kv_destroy);

  ten_value_t *value = ten_value_create_object_with_move(&value_object_kv_list);

  return value;
}

bool ten_cmd_start_graph_put_extension_info_to_json(ten_msg_t *self,
                                                    ten_json_t *json,
                                                    ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) && json &&
                 ten_raw_msg_get_type(self) == TEN_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  ten_json_t *ten_json =
      ten_json_object_peek_object_forcibly(json, TEN_STR_UNDERLINE_TEN);
  TEN_ASSERT(ten_json, "Should not happen.");

  ten_cmd_start_graph_t *cmd = (ten_cmd_start_graph_t *)self;

  ten_json_t *nodes_json =
      ten_json_object_peek_array_forcibly(ten_json, TEN_STR_NODES);
  TEN_ASSERT(nodes_json, "Should not happen.");

  ten_list_t unique_extension_list;
  ten_list_init(&unique_extension_list);

  ten_list_foreach (ten_raw_cmd_start_graph_get_extensions_info(cmd), iter) {
    ten_extension_info_t *extension_info =
        ten_shared_ptr_get_data(ten_smart_ptr_listnode_get(iter.node));
    TEN_ASSERT(extension_info, "Should not happen.");

    ten_string_t loc_str;
    ten_string_init(&loc_str);
    ten_loc_to_string(&extension_info->loc, &loc_str);

    if (ten_list_find_string(&unique_extension_list,
                             ten_string_get_raw_str(&loc_str)) != NULL) {
      TEN_LOGE("Extension %s is duplicated.", ten_string_get_raw_str(&loc_str));

      ten_string_deinit(&loc_str);
      ten_list_clear(&unique_extension_list);
      return false;
    }

    ten_list_push_str_back(&unique_extension_list,
                           ten_string_get_raw_str(&loc_str));
    ten_string_deinit(&loc_str);

    ten_json_t *extension_info_node_json =
        ten_extension_info_node_to_json(extension_info);
    TEN_ASSERT(extension_info_node_json, "Should not happen.");

    ten_json_array_append_new(nodes_json, extension_info_node_json);
  }

  ten_list_clear(&unique_extension_list);

  ten_list_foreach (ten_raw_cmd_start_graph_get_extension_groups_info(cmd),
                    iter) {
    ten_extension_group_info_t *extension_group_info =
        ten_shared_ptr_get_data(ten_smart_ptr_listnode_get(iter.node));
    TEN_ASSERT(extension_group_info, "Should not happen.");

    ten_json_t *extension_group_info_json =
        ten_extension_group_info_to_json(extension_group_info);
    TEN_ASSERT(extension_group_info_json, "Should not happen.");

    ten_json_array_append_new(nodes_json, extension_group_info_json);
  }

  ten_json_t *connections_json =
      ten_json_object_peek_array_forcibly(ten_json, TEN_STR_CONNECTIONS);
  TEN_ASSERT(connections_json, "Should not happen.");

  ten_list_foreach (ten_raw_cmd_start_graph_get_extensions_info(cmd), iter) {
    ten_extension_info_t *extension_info =
        ten_shared_ptr_get_data(ten_smart_ptr_listnode_get(iter.node));
    TEN_ASSERT(extension_info, "Should not happen.");

    ten_json_t *extension_info_connections_json = NULL;
    bool rc = ten_extension_info_connections_to_json(
        extension_info, &extension_info_connections_json, err);
    if (!rc) {
      return false;
    }

    if (extension_info_connections_json) {
      ten_json_array_append_new(connections_json,
                                extension_info_connections_json);
    }
  }

  return true;
}

static bool parse_graph_nodes(ten_json_t *nodes_json,
                              ten_list_t *extensions_info,
                              ten_list_t *extension_groups_info,
                              ten_error_t *err) {
  TEN_ASSERT(nodes_json && ten_json_check_integrity(nodes_json),
             "Should not happen.");

  size_t i = 0;
  ten_json_t *node_json = NULL;
  bool found_error = false;
  ten_json_array_foreach(nodes_json, i, node_json) {
    const char *type = ten_json_object_peek_string(node_json, TEN_STR_TYPE);
    if (strlen(type) == 0) {
      ten_error_set(err, TEN_ERRNO_INVALID_GRAPH,
                    "Invalid extension node type.");
      found_error = true;
      break;
    } else if (!strcmp(type, TEN_STR_EXTENSION)) {
      ten_shared_ptr_t *extension_info =
          ten_extension_info_nodes_from_json(node_json, extensions_info, err);
      if (!extension_info) {
        found_error = true;
        break;
      }
    } else if (!strcmp(type, TEN_STR_EXTENSION_GROUP)) {
      ten_shared_ptr_t *extension_group_info =
          ten_extension_group_info_from_json(node_json, extension_groups_info,
                                             err);
      if (!extension_group_info) {
        found_error = true;
        break;
      }
    }
  }

  if (found_error) {
    ten_list_clear(extensions_info);
    ten_list_clear(extension_groups_info);
  }

  return !found_error;
}

static bool parse_graph_connections(ten_json_t *connections_json,
                                    ten_list_t *extensions_info,
                                    ten_error_t *err) {
  TEN_ASSERT(connections_json && ten_json_check_integrity(connections_json),
             "Should not happen.");

  size_t i = 0;
  ten_json_t *connection_json = NULL;
  bool found_error = false;
  ten_json_array_foreach(connections_json, i, connection_json) {
    ten_shared_ptr_t *extension_info =
        ten_extension_info_parse_connection_src_part_from_json(
            connection_json, extensions_info, err);
    if (!extension_info) {
      found_error = true;
      break;
    }
  }

  return !found_error;
}

static void ten_raw_cmd_start_graph_set_extensions_and_groups_info(
    ten_cmd_start_graph_t *self, ten_list_t *extensions_info,
    ten_list_t *extension_groups_info) {
  TEN_ASSERT(self, "Should not happen.");
  TEN_ASSERT(extensions_info, "Should not happen.");
  TEN_ASSERT(extension_groups_info, "Should not happen.");

  ten_list_swap(&self->extensions_info, extensions_info);
  ten_list_swap(&self->extension_groups_info, extension_groups_info);
}

bool ten_cmd_start_graph_get_extension_info_from_json(ten_msg_t *self,
                                                      ten_json_t *json,
                                                      ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) &&
                 ten_raw_msg_get_type(self) == TEN_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");

  ten_json_t *ten_json =
      ten_json_object_peek_object(json, TEN_STR_UNDERLINE_TEN);
  if (!ten_json) {
    if (err) {
      ten_error_set(err, TEN_ERRNO_INVALID_GRAPH,
                    "The `ten` field is required.");
    }

    return false;
  }

  ten_json_t *nodes_json = ten_json_object_peek(ten_json, TEN_STR_NODES);
  if (nodes_json) {
    ten_list_t extensions_info = TEN_LIST_INIT_VAL;
    ten_list_t extension_groups_info = TEN_LIST_INIT_VAL;

    bool rc = parse_graph_nodes(nodes_json, &extensions_info,
                                &extension_groups_info, err);
    if (!rc) {
      ten_list_clear(&extensions_info);
      ten_list_clear(&extension_groups_info);
      return false;
    }

    ten_json_t *connections_json =
        ten_json_object_peek(ten_json, TEN_STR_CONNECTIONS);
    if (connections_json) {
      rc = parse_graph_connections(connections_json, &extensions_info, err);
      if (!rc) {
        ten_list_clear(&extensions_info);
        ten_list_clear(&extension_groups_info);
        return false;
      }
    }

    if (rc) {
      ten_raw_cmd_start_graph_set_extensions_and_groups_info(
          (ten_cmd_start_graph_t *)self, &extensions_info,
          &extension_groups_info);
    }

    return rc;
  }

  return true;
}

bool ten_cmd_start_graph_copy_extensions_info(
    ten_msg_t *self, ten_msg_t *src, TEN_UNUSED ten_list_t *excluded_field_ids,
    ten_error_t *err) {
  TEN_ASSERT(self && src && ten_raw_cmd_check_integrity((ten_cmd_t *)src) &&
                 ten_raw_msg_get_type(src) == TEN_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  ten_cmd_start_graph_t *src_cmd = (ten_cmd_start_graph_t *)src;
  ten_cmd_start_graph_t *self_cmd = (ten_cmd_start_graph_t *)self;

  ten_list_foreach (&src_cmd->extension_groups_info, iter) {
    ten_shared_ptr_t *extension_group_info_ =
        ten_smart_ptr_listnode_get(iter.node);
    ten_extension_group_info_t *extension_group_info =
        ten_extension_group_info_from_smart_ptr(extension_group_info_);

    TEN_UNUSED bool rc = ten_extension_group_info_clone(
        extension_group_info, &self_cmd->extension_groups_info);
    TEN_ASSERT(rc, "Should not happen.");
  }

  ten_list_foreach (&src_cmd->extensions_info, iter) {
    ten_shared_ptr_t *extension_info_ = ten_smart_ptr_listnode_get(iter.node);
    ten_extension_info_t *extension_info =
        ten_extension_info_from_smart_ptr(extension_info_);

    bool rc = ten_extension_info_clone(extension_info,
                                       &self_cmd->extensions_info, err);
    TEN_ASSERT(rc, "Should not happen.");
    if (!rc) {
      return false;
    }
  }

  return true;
}

bool ten_cmd_start_graph_process_extensions_info(
    ten_msg_t *self, ten_raw_msg_process_one_field_func_t cb, void *user_data,
    ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");

  ten_cmd_start_graph_t *cmd = (ten_cmd_start_graph_t *)self;

  ten_value_t *extensions_info_value =
      ten_cmd_start_graph_extensions_info_to_value(self, err);

  ten_value_t *nodes_value =
      ten_value_object_peek(extensions_info_value, TEN_STR_NODES);
  TEN_ASSERT(ten_value_is_array(nodes_value), "Should not happen.");

  ten_value_t *connections_value =
      ten_value_object_peek(extensions_info_value, TEN_STR_CONNECTIONS);
  TEN_ASSERT(ten_value_is_array(connections_value), "Should not happen.");

  ten_msg_field_process_data_t extensions_info_nodes_field;
  ten_msg_field_process_data_init(&extensions_info_nodes_field, TEN_STR_NODES,
                                  nodes_value, false);

  bool rc = cb(self, &extensions_info_nodes_field, user_data, err);
  if (!rc) {
    goto error;
  }

  if (extensions_info_nodes_field.value_is_changed_after_process) {
    ten_value_array_foreach(nodes_value, iter) {
      ten_value_t *node_value = ten_ptr_listnode_get(iter.node);
      if (!ten_value_is_object(node_value)) {
        goto error;
      }

      if (ten_extension_info_node_from_value(
              node_value, ten_raw_cmd_start_graph_get_extensions_info(cmd),
              err) == NULL) {
        goto error;
      }
    }
  }

  ten_msg_field_process_data_t extensions_info_connections_field;
  ten_msg_field_process_data_init(&extensions_info_connections_field,
                                  TEN_STR_CONNECTIONS, connections_value,
                                  false);

  rc = cb(self, &extensions_info_connections_field, user_data, err);
  if (!rc) {
    goto error;
  }

  if (extensions_info_connections_field.value_is_changed_after_process) {
    ten_value_array_foreach(connections_value, iter) {
      ten_value_t *item_value = ten_ptr_listnode_get(iter.node);
      if (!ten_value_is_object(item_value)) {
        goto error;
      }

      ten_extension_info_parse_connection_src_part_from_value(
          item_value, ten_raw_cmd_start_graph_get_extensions_info(cmd), err);
    }
  }

  ten_value_destroy(extensions_info_value);
  return true;

error:
  ten_value_destroy(extensions_info_value);
  return false;
}
