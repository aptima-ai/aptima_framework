//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/app/predefined_graph.h"

#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/app/engine_interface.h"
#include "include_internal/ten_runtime/app/graph.h"
#include "include_internal/ten_runtime/app/metadata.h"
#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/engine/engine.h"
#include "include_internal/ten_runtime/engine/msg_interface/common.h"
#include "include_internal/ten_runtime/extension/extension_info/extension_info.h"
#include "include_internal/ten_runtime/extension/extension_info/json.h"
#include "include_internal/ten_runtime/extension/extension_info/value.h"
#include "include_internal/ten_runtime/extension_group/extension_group_info/extension_group_info.h"
#include "include_internal/ten_runtime/extension_group/extension_group_info/json.h"
#include "include_internal/ten_runtime/extension_group/extension_group_info/value.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_node.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/log/log.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/value/value_get.h"

ten_predefined_graph_info_t *ten_predefined_graph_info_create(void) {
  ten_predefined_graph_info_t *self =
      TEN_MALLOC(sizeof(ten_predefined_graph_info_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  ten_string_init(&self->name);
  ten_list_init(&self->extensions_info);
  ten_list_init(&self->extension_groups_info);

  self->auto_start = false;
  self->singleton = false;
  self->engine = NULL;

  return self;
}

void ten_predefined_graph_info_destroy(ten_predefined_graph_info_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  ten_string_deinit(&self->name);
  ten_list_clear(&self->extensions_info);
  ten_list_clear(&self->extension_groups_info);

  TEN_FREE(self);
}

static ten_json_t *ten_app_build_start_graph_cmd_to_start_predefined_graph(
    ten_app_t *self, ten_predefined_graph_info_t *predefined_graph_info,
    ten_error_t *err) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Invalid argument.");
  TEN_ASSERT(predefined_graph_info, "Invalid argument.");

  const char *app_uri = ten_app_get_uri(self);

  ten_json_t *start_graph_cmd_json = ten_json_create_object();
  TEN_ASSERT(start_graph_cmd_json, "Should not happen.");

  ten_json_t *ten_json = ten_json_object_peek_object_forcibly(
      start_graph_cmd_json, TEN_STR_UNDERLINE_TEN);

  ten_json_t *type_json = ten_json_create_string(TEN_STR_START_GRAPH);
  ten_json_object_set_new(ten_json, TEN_STR_TYPE, type_json);

  ten_json_t *dests_json = ten_json_create_array();
  ten_json_object_set_new(ten_json, TEN_STR_DEST, dests_json);

  ten_json_t *dest_json = ten_json_create_object();
  ten_json_object_set_new(dest_json, TEN_STR_APP,
                          ten_json_create_string(app_uri));
  ten_json_array_append_new(dests_json, dest_json);

  // Convert to a json first, then dump to a string from the created json.

  ten_json_t *nodes_json = ten_json_create_array();
  ten_json_object_set_new(ten_json, TEN_STR_NODES, nodes_json);

  ten_list_foreach (&predefined_graph_info->extensions_info, iter) {
    ten_extension_info_t *extension_info =
        ten_shared_ptr_get_data(ten_smart_ptr_listnode_get(iter.node));

    ten_json_t *extension_info_json =
        ten_extension_info_node_to_json(extension_info);
    TEN_ASSERT(
        extension_info_json && ten_json_check_integrity(extension_info_json),
        "Invalid argument.");

    ten_json_array_append_new(nodes_json, extension_info_json);
  }

  ten_list_foreach (&predefined_graph_info->extension_groups_info, iter) {
    ten_extension_group_info_t *extension_group_info =
        ten_shared_ptr_get_data(ten_smart_ptr_listnode_get(iter.node));

    ten_json_t *extension_group_info_json =
        ten_extension_group_info_to_json(extension_group_info);
    TEN_ASSERT(extension_group_info_json &&
                   ten_json_check_integrity(extension_group_info_json),
               "Invalid argument.");

    ten_json_array_append_new(nodes_json, extension_group_info_json);
  }

  ten_json_t *connections_json = ten_json_create_array();
  ten_json_object_set_new(ten_json, TEN_STR_CONNECTIONS, connections_json);

  ten_list_foreach (&predefined_graph_info->extensions_info, iter) {
    ten_extension_info_t *extension_info =
        ten_shared_ptr_get_data(ten_smart_ptr_listnode_get(iter.node));

    ten_json_t *extension_info_json = NULL;
    bool rc = ten_extension_info_connections_to_json(extension_info,
                                                     &extension_info_json, err);
    if (!rc) {
      ten_json_destroy(start_graph_cmd_json);
      return NULL;
    }

    if (extension_info_json) {
      TEN_ASSERT(ten_json_check_integrity(extension_info_json),
                 "Invalid argument.");
      ten_json_array_append_new(connections_json, extension_info_json);
    }
  }

  return start_graph_cmd_json;
}

bool ten_app_start_predefined_graph(
    ten_app_t *self, ten_predefined_graph_info_t *predefined_graph_info,
    ten_error_t *err) {
  TEN_ASSERT(
      self && ten_app_check_integrity(self, true) && predefined_graph_info,
      "Should not happen.");

  ten_json_t *start_graph_cmd_json =
      ten_app_build_start_graph_cmd_to_start_predefined_graph(
          self, predefined_graph_info, err);
  if (!start_graph_cmd_json) {
    return false;
  }

  if (!ten_app_check_start_graph_cmd_json(self, start_graph_cmd_json, err)) {
    // TODO(Wei): The graph check does not support message conversion now, so we
    // can not return false here. WIP: issues#160.
    TEN_LOGW("[%s] The predefined graph is invalid, %s", ten_app_get_uri(self),
             ten_error_errmsg(err));
  }

  ten_shared_ptr_t *start_graph_cmd =
      ten_msg_create_from_json(start_graph_cmd_json, NULL);

  ten_json_destroy(start_graph_cmd_json);

  ten_msg_set_src_to_app(start_graph_cmd, self);

  predefined_graph_info->engine = ten_app_create_engine(self, start_graph_cmd);

  // There is no 'connection' when creating predefined graph, so it's always no
  // migration in this stage. Send the 'start_graph_cmd' into the newly created
  // engine directly.
  ten_engine_append_to_in_msgs_queue(predefined_graph_info->engine,
                                     start_graph_cmd);

  ten_shared_ptr_destroy(start_graph_cmd);

  return true;
}

bool ten_app_start_auto_start_predefined_graph(ten_app_t *self,
                                               ten_error_t *err) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");

  ten_list_foreach (&self->predefined_graph_infos, iter) {
    ten_predefined_graph_info_t *predefined_graph_info =
        (ten_predefined_graph_info_t *)ten_ptr_listnode_get(iter.node);

    if (!predefined_graph_info->auto_start) {
      continue;
    }

    if (!ten_app_start_predefined_graph(self, predefined_graph_info, err)) {
      return false;
    }
  }

  return true;
}

static ten_predefined_graph_info_t *ten_predefined_graph_infos_get_by_name(
    ten_list_t *predefined_graph_infos, const char *name) {
  TEN_ASSERT(predefined_graph_infos && name, "Invalid argument.");

  ten_list_foreach (predefined_graph_infos, iter) {
    ten_predefined_graph_info_t *predefined_graph_info =
        (ten_predefined_graph_info_t *)ten_ptr_listnode_get(iter.node);

    if (ten_string_is_equal_c_str(&predefined_graph_info->name, name)) {
      return predefined_graph_info;
    }
  }

  return NULL;
}

static ten_predefined_graph_info_t *ten_app_get_predefined_graph_info_by_name(
    ten_app_t *self, const char *name) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true) && name,
             "Should not happen.");

  return ten_predefined_graph_infos_get_by_name(&self->predefined_graph_infos,
                                                name);
}

ten_predefined_graph_info_t *ten_predefined_graph_infos_get_singleton_by_name(
    ten_list_t *predefined_graph_infos, const char *name) {
  TEN_ASSERT(predefined_graph_infos && name, "Invalid argument.");

  ten_predefined_graph_info_t *result =
      ten_predefined_graph_infos_get_by_name(predefined_graph_infos, name);

  if (result && result->singleton) {
    return result;
  }

  return NULL;
}

ten_predefined_graph_info_t *
ten_app_get_singleton_predefined_graph_info_by_name(ten_app_t *self,
                                                    const char *name) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true) && name,
             "Should not happen.");

  return ten_predefined_graph_infos_get_singleton_by_name(
      &self->predefined_graph_infos, name);
}

bool ten_app_get_predefined_graph_extensions_and_groups_info_by_name(
    ten_app_t *self, const char *name, ten_list_t *extensions_info,
    ten_list_t *extension_groups_info, ten_error_t *err) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");
  TEN_ASSERT(name, "Invalid argument.");
  TEN_ASSERT(extensions_info, "Should not happen.");

  ten_predefined_graph_info_t *predefined_graph_info =
      ten_app_get_predefined_graph_info_by_name(self, name);
  TEN_ASSERT(predefined_graph_info, "Should not happen.");
  if (!predefined_graph_info) {
    return false;
  }

  if (!ten_extensions_info_clone(
          extensions_info, &predefined_graph_info->extensions_info, err)) {
    return false;
  }

  ten_list_foreach (&predefined_graph_info->extension_groups_info, iter) {
    ten_extension_group_info_t *extension_group_info =
        ten_shared_ptr_get_data(ten_smart_ptr_listnode_get(iter.node));
    ten_extension_group_info_clone(extension_group_info, extension_groups_info);
  }

  return true;
}

ten_engine_t *ten_app_get_singleton_predefined_graph_engine_by_name(
    ten_app_t *self, const char *name) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true) && name,
             "Should not happen.");

  ten_predefined_graph_info_t *predefined_graph_info =
      ten_app_get_singleton_predefined_graph_info_by_name(self, name);

  if (predefined_graph_info) {
    return predefined_graph_info->engine;
  }
  return NULL;
}

bool ten_app_get_predefined_graphs_from_property(ten_app_t *self) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");

  bool result = true;

  ten_value_t *app_property = &self->property;
  TEN_ASSERT(ten_value_check_integrity(app_property), "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  ten_value_t *ten_namespace_properties =
      ten_app_get_ten_namespace_properties(self);
  if (ten_namespace_properties == NULL) {
    return true;
  }

  ten_value_t *predefined_graphs = ten_value_object_peek(
      ten_namespace_properties, TEN_STR_PREDEFINED_GRAPHS);
  if (!predefined_graphs || !ten_value_is_array(predefined_graphs)) {
    // There is no predefined graph in the manifest, it's OK.
    goto done;
  }

  int graph_idx = -1;
  ten_list_foreach (ten_value_peek_array(predefined_graphs),
                    predefined_graphs_iter) {
    graph_idx++;

    ten_value_t *predefined_graph_info_value =
        ten_ptr_listnode_get(predefined_graphs_iter.node);
    TEN_ASSERT(predefined_graph_info_value &&
                   ten_value_check_integrity(predefined_graph_info_value),
               "Invalid argument.");
    if (!predefined_graph_info_value ||
        !ten_value_is_object(predefined_graph_info_value)) {
      result = false;
      goto done;
    }

    ten_predefined_graph_info_t *predefined_graph_info =
        ten_predefined_graph_info_create();

    ten_value_t *predefined_graph_info_name_value =
        ten_value_object_peek(predefined_graph_info_value, TEN_STR_NAME);
    if (!predefined_graph_info_name_value ||
        !ten_value_is_string(predefined_graph_info_name_value)) {
      ten_predefined_graph_info_destroy(predefined_graph_info);
      result = false;
      goto done;
    }
    ten_string_set_from_c_str(
        &predefined_graph_info->name,
        ten_value_peek_raw_str(predefined_graph_info_name_value),
        strlen(ten_value_peek_raw_str(predefined_graph_info_name_value)));

    ten_value_t *predefined_graph_info_auto_start_value =
        ten_value_object_peek(predefined_graph_info_value, TEN_STR_AUTO_START);
    if (predefined_graph_info_auto_start_value &&
        ten_value_is_bool(predefined_graph_info_auto_start_value)) {
      predefined_graph_info->auto_start =
          ten_value_get_bool(predefined_graph_info_auto_start_value, &err);
    }

    ten_value_t *predefined_graph_info_singleton_value =
        ten_value_object_peek(predefined_graph_info_value, TEN_STR_SINGLETON);
    if (predefined_graph_info_singleton_value &&
        ten_value_is_bool(predefined_graph_info_singleton_value)) {
      predefined_graph_info->singleton =
          ten_value_get_bool(predefined_graph_info_singleton_value, &err);
    }

    // Parse 'nodes'.
    ten_value_t *predefined_graph_info_nodes_value =
        ten_value_object_peek(predefined_graph_info_value, TEN_STR_NODES);
    if (predefined_graph_info_nodes_value &&
        ten_value_is_array(predefined_graph_info_nodes_value)) {
      ten_value_array_foreach(predefined_graph_info_nodes_value,
                              predefined_graph_info_node_iter) {
        ten_value_t *predefined_graph_info_node_item_value =
            ten_ptr_listnode_get(predefined_graph_info_node_iter.node);
        TEN_ASSERT(predefined_graph_info_node_item_value &&
                       ten_value_check_integrity(
                           predefined_graph_info_node_item_value),
                   "Invalid argument.");

        if (!predefined_graph_info_node_item_value ||
            !ten_value_is_object(predefined_graph_info_node_item_value)) {
          ten_predefined_graph_info_destroy(predefined_graph_info);
          result = false;
          goto done;
        }

        ten_value_t *type_value = ten_value_object_peek(
            predefined_graph_info_node_item_value, TEN_STR_TYPE);
        if (!type_value || !ten_value_is_string(type_value)) {
          ten_predefined_graph_info_destroy(predefined_graph_info);
          result = false;
          goto done;
        }

        const char *type = ten_value_peek_raw_str(type_value);

        // Only the extension node is preferred.
        result = ten_c_string_is_equal(type, TEN_STR_EXTENSION);
        if (result) {
          ten_shared_ptr_t *extension_info = ten_extension_info_node_from_value(
              predefined_graph_info_node_item_value,
              &predefined_graph_info->extensions_info, &err);
          if (!extension_info) {
            result = false;
          }
        }

        if (!result) {
          ten_predefined_graph_info_destroy(predefined_graph_info);
          goto done;
        }
      }
    }

    // Parse 'connections'.
    ten_value_t *predefined_graph_info_connections_value =
        ten_value_object_peek(predefined_graph_info_value, TEN_STR_CONNECTIONS);
    if (predefined_graph_info_connections_value &&
        ten_value_is_array(predefined_graph_info_connections_value)) {
      ten_value_array_foreach(predefined_graph_info_connections_value,
                              predefined_graph_info_connection_iter) {
        ten_value_t *predefined_graph_info_connection_item_value =
            ten_ptr_listnode_get(predefined_graph_info_connection_iter.node);
        TEN_ASSERT(predefined_graph_info_connection_item_value &&
                       ten_value_check_integrity(
                           predefined_graph_info_connection_item_value),
                   "Invalid argument.");

        result =
            predefined_graph_info_connection_item_value &&
            ten_value_is_object(predefined_graph_info_connection_item_value);
        if (result) {
          ten_shared_ptr_t *src_extension_in_connection =
              ten_extension_info_parse_connection_src_part_from_value(
                  predefined_graph_info_connection_item_value,
                  &predefined_graph_info->extensions_info, &err);
          if (!src_extension_in_connection) {
            result = false;
          }
        }

        if (!result) {
          ten_predefined_graph_info_destroy(predefined_graph_info);
          goto done;
        }
      }
    }

    ten_list_push_ptr_back(
        &self->predefined_graph_infos, predefined_graph_info,
        (ten_ptr_listnode_destroy_func_t)ten_predefined_graph_info_destroy);
  }

  // Update the URI of each extension_info to the one of the current app, if
  // not specified originally.
  ten_list_foreach (&self->predefined_graph_infos, iter) {
    ten_predefined_graph_info_t *predefined_graph_info =
        ten_ptr_listnode_get(iter.node);

    ten_extensions_info_fill_app_uri(&predefined_graph_info->extensions_info,
                                     ten_string_get_raw_str(&self->uri));
    ten_extension_groups_info_fill_app_uri(
        &predefined_graph_info->extension_groups_info,
        ten_string_get_raw_str(&self->uri));
  }

done:
  if (result == false) {
    ten_list_clear(&self->predefined_graph_infos);
    TEN_LOGE("[%s] Failed to parse predefined_graphs[%d], %s",
             ten_app_get_uri(self), graph_idx, ten_error_errmsg(&err));
  }

  ten_error_deinit(&err);

  return result;
}
