//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/app/predefined_graph.h"

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/app/engine_interface.h"
#include "include_internal/axis_runtime/app/graph.h"
#include "include_internal/axis_runtime/app/metadata.h"
#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/engine/msg_interface/common.h"
#include "include_internal/axis_runtime/extension/extension_info/extension_info.h"
#include "include_internal/axis_runtime/extension/extension_info/json.h"
#include "include_internal/axis_runtime/extension/extension_info/value.h"
#include "include_internal/axis_runtime/extension_group/extension_group_info/extension_group_info.h"
#include "include_internal/axis_runtime/extension_group/extension_group_info/json.h"
#include "include_internal/axis_runtime/extension_group/extension_group_info/value.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/start_graph/cmd.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_runtime/msg/cmd/start_graph/cmd.h"
#include "axis_runtime/msg/msg.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value_get.h"

axis_predefined_graph_info_t *axis_predefined_graph_info_create(void) {
  axis_predefined_graph_info_t *self =
      axis_MALLOC(sizeof(axis_predefined_graph_info_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_string_init(&self->name);
  axis_list_init(&self->extensions_info);
  axis_list_init(&self->extension_groups_info);

  self->auto_start = false;
  self->singleton = false;
  self->engine = NULL;

  return self;
}

void axis_predefined_graph_info_destroy(axis_predefined_graph_info_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_string_deinit(&self->name);
  axis_list_clear(&self->extensions_info);
  axis_list_clear(&self->extension_groups_info);

  axis_FREE(self);
}

static axis_shared_ptr_t *
axis_app_build_start_graph_cmd_to_start_predefined_graph(
    axis_app_t *self, axis_predefined_graph_info_t *predefined_graph_info,
    axis_error_t *err) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Invalid argument.");
  axis_ASSERT(predefined_graph_info, "Invalid argument.");

  const char *app_uri = axis_app_get_uri(self);

  axis_shared_ptr_t *start_graph_cmd = axis_cmd_start_graph_create();
  axis_ASSERT(start_graph_cmd, "Should not happen.");

  axis_msg_clear_and_set_dest(start_graph_cmd, app_uri, NULL, NULL, NULL, err);

  axis_json_t *start_graph_cmd_json = axis_json_create_object();
  axis_ASSERT(start_graph_cmd_json, "Should not happen.");

  axis_json_t *axis_json = axis_json_object_peek_object_forcibly(
      start_graph_cmd_json, axis_STR_UNDERLINE_TEN);

  axis_json_t *nodes_json = axis_json_create_array();
  axis_json_object_set_new(axis_json, axis_STR_NODES, nodes_json);

  axis_list_foreach (&predefined_graph_info->extensions_info, iter) {
    axis_extension_info_t *extension_info =
        axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(iter.node));

    axis_json_t *extension_info_json =
        axis_extension_info_node_to_json(extension_info);
    axis_ASSERT(
        extension_info_json && axis_json_check_integrity(extension_info_json),
        "Invalid argument.");
    if (!extension_info_json) {
      goto error;
    }

    axis_json_array_append_new(nodes_json, extension_info_json);
  }

  axis_list_foreach (&predefined_graph_info->extension_groups_info, iter) {
    axis_extension_group_info_t *extension_group_info =
        axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(iter.node));

    axis_json_t *extension_group_info_json =
        axis_extension_group_info_to_json(extension_group_info);
    axis_ASSERT(extension_group_info_json &&
                   axis_json_check_integrity(extension_group_info_json),
               "Invalid argument.");
    if (!extension_group_info_json) {
      goto error;
    }

    axis_json_array_append_new(nodes_json, extension_group_info_json);
  }

  axis_json_t *connections_json = axis_json_create_array();
  axis_json_object_set_new(axis_json, axis_STR_CONNECTIONS, connections_json);

  axis_list_foreach (&predefined_graph_info->extensions_info, iter) {
    axis_extension_info_t *extension_info =
        axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(iter.node));

    axis_json_t *extension_info_json = NULL;
    bool rc = axis_extension_info_connections_to_json(extension_info,
                                                     &extension_info_json, err);
    if (!rc) {
      goto error;
    }

    if (extension_info_json) {
      axis_ASSERT(axis_json_check_integrity(extension_info_json),
                 "Invalid argument.");
      axis_json_array_append_new(connections_json, extension_info_json);
    }
  }

  axis_raw_cmd_start_graph_init_from_json(
      (axis_cmd_start_graph_t *)axis_msg_get_raw_msg(start_graph_cmd),
      start_graph_cmd_json, err);

  goto done;

error:
  axis_shared_ptr_destroy(start_graph_cmd);
  start_graph_cmd = NULL;

done:
  axis_json_destroy(start_graph_cmd_json);
  return start_graph_cmd;
}

bool axis_app_start_predefined_graph(
    axis_app_t *self, axis_predefined_graph_info_t *predefined_graph_info,
    axis_error_t *err) {
  axis_ASSERT(
      self && axis_app_check_integrity(self, true) && predefined_graph_info,
      "Should not happen.");

  axis_shared_ptr_t *start_graph_cmd =
      axis_app_build_start_graph_cmd_to_start_predefined_graph(
          self, predefined_graph_info, err);
  if (!start_graph_cmd) {
    return false;
  }

  if (!axis_app_check_start_graph_cmd(self, start_graph_cmd, err)) {
    // TODO(Wei): The graph check does not support message conversion now, so we
    // can not return false here. WIP: issues#160.
    axis_LOGW("[%s] The predefined graph is invalid, %s", axis_app_get_uri(self),
             axis_error_errmsg(err));
  }

  axis_msg_set_src_to_app(start_graph_cmd, self);

  predefined_graph_info->engine = axis_app_create_engine(self, start_graph_cmd);

  // There is no 'connection' when creating predefined graph, so it's always no
  // migration in this stage. Send the 'start_graph_cmd' into the newly created
  // engine directly.
  axis_engine_append_to_in_msgs_queue(predefined_graph_info->engine,
                                     start_graph_cmd);

  axis_shared_ptr_destroy(start_graph_cmd);

  return true;
}

bool axis_app_start_auto_start_predefined_graph(axis_app_t *self,
                                               axis_error_t *err) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");

  axis_list_foreach (&self->predefined_graph_infos, iter) {
    axis_predefined_graph_info_t *predefined_graph_info =
        (axis_predefined_graph_info_t *)axis_ptr_listnode_get(iter.node);

    if (!predefined_graph_info->auto_start) {
      continue;
    }

    if (!axis_app_start_predefined_graph(self, predefined_graph_info, err)) {
      return false;
    }
  }

  return true;
}

static axis_predefined_graph_info_t *axis_predefined_graph_infos_get_by_name(
    axis_list_t *predefined_graph_infos, const char *name) {
  axis_ASSERT(predefined_graph_infos && name, "Invalid argument.");

  axis_list_foreach (predefined_graph_infos, iter) {
    axis_predefined_graph_info_t *predefined_graph_info =
        (axis_predefined_graph_info_t *)axis_ptr_listnode_get(iter.node);

    if (axis_string_is_equal_c_str(&predefined_graph_info->name, name)) {
      return predefined_graph_info;
    }
  }

  return NULL;
}

static axis_predefined_graph_info_t *axis_app_get_predefined_graph_info_by_name(
    axis_app_t *self, const char *name) {
  axis_ASSERT(self && axis_app_check_integrity(self, true) && name,
             "Should not happen.");

  return axis_predefined_graph_infos_get_by_name(&self->predefined_graph_infos,
                                                name);
}

axis_predefined_graph_info_t *axis_predefined_graph_infos_get_singleton_by_name(
    axis_list_t *predefined_graph_infos, const char *name) {
  axis_ASSERT(predefined_graph_infos && name, "Invalid argument.");

  axis_predefined_graph_info_t *result =
      axis_predefined_graph_infos_get_by_name(predefined_graph_infos, name);

  if (result && result->singleton) {
    return result;
  }

  return NULL;
}

axis_predefined_graph_info_t *
axis_app_get_singleton_predefined_graph_info_by_name(axis_app_t *self,
                                                    const char *name) {
  axis_ASSERT(self && axis_app_check_integrity(self, true) && name,
             "Should not happen.");

  return axis_predefined_graph_infos_get_singleton_by_name(
      &self->predefined_graph_infos, name);
}

bool axis_app_get_predefined_graph_extensions_and_groups_info_by_name(
    axis_app_t *self, const char *name, axis_list_t *extensions_info,
    axis_list_t *extension_groups_info, axis_error_t *err) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");
  axis_ASSERT(name, "Invalid argument.");
  axis_ASSERT(extensions_info, "Should not happen.");

  axis_predefined_graph_info_t *predefined_graph_info =
      axis_app_get_predefined_graph_info_by_name(self, name);
  axis_ASSERT(predefined_graph_info, "Should not happen.");
  if (!predefined_graph_info) {
    return false;
  }

  if (!axis_extensions_info_clone(&predefined_graph_info->extensions_info,
                                 extensions_info, err)) {
    return false;
  }

  axis_list_foreach (&predefined_graph_info->extension_groups_info, iter) {
    axis_extension_group_info_t *extension_group_info =
        axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(iter.node));
    axis_extension_group_info_clone(extension_group_info, extension_groups_info);
  }

  return true;
}

axis_engine_t *axis_app_get_singleton_predefined_graph_engine_by_name(
    axis_app_t *self, const char *name) {
  axis_ASSERT(self && axis_app_check_integrity(self, true) && name,
             "Should not happen.");

  axis_predefined_graph_info_t *predefined_graph_info =
      axis_app_get_singleton_predefined_graph_info_by_name(self, name);

  if (predefined_graph_info) {
    return predefined_graph_info->engine;
  }
  return NULL;
}

bool axis_app_get_predefined_graphs_from_property(axis_app_t *self) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");

  bool result = true;

  axis_value_t *app_property = &self->property;
  axis_ASSERT(axis_value_check_integrity(app_property), "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  axis_value_t *axis_namespace_properties =
      axis_app_get_axis_namespace_properties(self);
  if (axis_namespace_properties == NULL) {
    return true;
  }

  axis_value_t *predefined_graphs = axis_value_object_peek(
      axis_namespace_properties, axis_STR_PREDEFINED_GRAPHS);
  if (!predefined_graphs || !axis_value_is_array(predefined_graphs)) {
    // There is no predefined graph in the manifest, it's OK.
    goto done;
  }

  int graph_idx = -1;
  axis_list_foreach (axis_value_peek_array(predefined_graphs),
                    predefined_graphs_iter) {
    graph_idx++;

    axis_value_t *predefined_graph_info_value =
        axis_ptr_listnode_get(predefined_graphs_iter.node);
    axis_ASSERT(predefined_graph_info_value &&
                   axis_value_check_integrity(predefined_graph_info_value),
               "Invalid argument.");
    if (!predefined_graph_info_value ||
        !axis_value_is_object(predefined_graph_info_value)) {
      result = false;
      goto done;
    }

    axis_predefined_graph_info_t *predefined_graph_info =
        axis_predefined_graph_info_create();

    axis_value_t *predefined_graph_info_name_value =
        axis_value_object_peek(predefined_graph_info_value, axis_STR_NAME);
    if (!predefined_graph_info_name_value ||
        !axis_value_is_string(predefined_graph_info_name_value)) {
      axis_predefined_graph_info_destroy(predefined_graph_info);
      result = false;
      goto done;
    }
    axis_string_set_from_c_str(
        &predefined_graph_info->name,
        axis_value_peek_raw_str(predefined_graph_info_name_value, &err),
        strlen(axis_value_peek_raw_str(predefined_graph_info_name_value, &err)));

    axis_value_t *predefined_graph_info_auto_start_value =
        axis_value_object_peek(predefined_graph_info_value, axis_STR_AUTO_START);
    if (predefined_graph_info_auto_start_value &&
        axis_value_is_bool(predefined_graph_info_auto_start_value)) {
      predefined_graph_info->auto_start =
          axis_value_get_bool(predefined_graph_info_auto_start_value, &err);
    }

    axis_value_t *predefined_graph_info_singleton_value =
        axis_value_object_peek(predefined_graph_info_value, axis_STR_SINGLETON);
    if (predefined_graph_info_singleton_value &&
        axis_value_is_bool(predefined_graph_info_singleton_value)) {
      predefined_graph_info->singleton =
          axis_value_get_bool(predefined_graph_info_singleton_value, &err);
    }

    // Parse 'nodes'.
    axis_value_t *predefined_graph_info_nodes_value =
        axis_value_object_peek(predefined_graph_info_value, axis_STR_NODES);
    if (predefined_graph_info_nodes_value &&
        axis_value_is_array(predefined_graph_info_nodes_value)) {
      axis_value_array_foreach(predefined_graph_info_nodes_value,
                              predefined_graph_info_node_iter) {
        axis_value_t *predefined_graph_info_node_item_value =
            axis_ptr_listnode_get(predefined_graph_info_node_iter.node);
        axis_ASSERT(predefined_graph_info_node_item_value &&
                       axis_value_check_integrity(
                           predefined_graph_info_node_item_value),
                   "Invalid argument.");

        if (!predefined_graph_info_node_item_value ||
            !axis_value_is_object(predefined_graph_info_node_item_value)) {
          axis_predefined_graph_info_destroy(predefined_graph_info);
          result = false;
          goto done;
        }

        axis_value_t *type_value = axis_value_object_peek(
            predefined_graph_info_node_item_value, axis_STR_TYPE);
        if (!type_value || !axis_value_is_string(type_value)) {
          axis_predefined_graph_info_destroy(predefined_graph_info);
          result = false;
          goto done;
        }

        const char *type = axis_value_peek_raw_str(type_value, &err);

        // Only the extension node is preferred.
        result = axis_c_string_is_equal(type, axis_STR_EXTENSION);
        if (result) {
          axis_shared_ptr_t *extension_info = axis_extension_info_node_from_value(
              predefined_graph_info_node_item_value,
              &predefined_graph_info->extensions_info, &err);
          if (!extension_info) {
            result = false;
          }
        }

        if (!result) {
          axis_predefined_graph_info_destroy(predefined_graph_info);
          goto done;
        }
      }
    }

    // Parse 'connections'.
    axis_value_t *predefined_graph_info_connections_value =
        axis_value_object_peek(predefined_graph_info_value, axis_STR_CONNECTIONS);
    if (predefined_graph_info_connections_value &&
        axis_value_is_array(predefined_graph_info_connections_value)) {
      axis_value_array_foreach(predefined_graph_info_connections_value,
                              predefined_graph_info_connection_iter) {
        axis_value_t *predefined_graph_info_connection_item_value =
            axis_ptr_listnode_get(predefined_graph_info_connection_iter.node);
        axis_ASSERT(predefined_graph_info_connection_item_value &&
                       axis_value_check_integrity(
                           predefined_graph_info_connection_item_value),
                   "Invalid argument.");

        result =
            predefined_graph_info_connection_item_value &&
            axis_value_is_object(predefined_graph_info_connection_item_value);
        if (result) {
          axis_shared_ptr_t *src_extension_in_connection =
              axis_extension_info_parse_connection_src_part_from_value(
                  predefined_graph_info_connection_item_value,
                  &predefined_graph_info->extensions_info, &err);
          if (!src_extension_in_connection) {
            result = false;
          }
        }

        if (!result) {
          axis_predefined_graph_info_destroy(predefined_graph_info);
          goto done;
        }
      }
    }

    axis_list_push_ptr_back(
        &self->predefined_graph_infos, predefined_graph_info,
        (axis_ptr_listnode_destroy_func_t)axis_predefined_graph_info_destroy);
  }

  // Update the URI of each extension_info to the one of the current app, if
  // not specified originally.
  axis_list_foreach (&self->predefined_graph_infos, iter) {
    axis_predefined_graph_info_t *predefined_graph_info =
        axis_ptr_listnode_get(iter.node);

    axis_extensions_info_fill_app_uri(&predefined_graph_info->extensions_info,
                                     axis_string_get_raw_str(&self->uri));
    axis_extension_groups_info_fill_app_uri(
        &predefined_graph_info->extension_groups_info,
        axis_string_get_raw_str(&self->uri));
  }

done:
  if (result == false) {
    axis_list_clear(&self->predefined_graph_infos);
    axis_LOGE("[%s] Failed to parse predefined_graphs[%d], %s",
             axis_app_get_uri(self), graph_idx, axis_error_errmsg(&err));
  }

  axis_error_deinit(&err);

  return result;
}
