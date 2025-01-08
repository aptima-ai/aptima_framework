
//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/app/msg_interface/start_graph.h"

#include <time.h>

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/app/close.h"
#include "include_internal/axis_runtime/app/engine_interface.h"
#include "include_internal/axis_runtime/app/graph.h"
#include "include_internal/axis_runtime/app/metadata.h"
#include "include_internal/axis_runtime/app/msg_interface/common.h"
#include "include_internal/axis_runtime/app/predefined_graph.h"
#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/connection/connection.h"
#include "include_internal/axis_runtime/connection/migration.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/engine/internal/migration.h"
#include "include_internal/axis_runtime/engine/msg_interface/common.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/start_graph/cmd.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/protocol/protocol.h"
#include "axis_runtime/app/app.h"
#include "axis_runtime/common/status_code.h"
#include "axis_runtime/msg/cmd_result/cmd_result.h"
#include "axis_runtime/msg/msg.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"

static bool axis_app_fill_start_graph_cmd_extensions_info_from_predefined_graph(
    axis_app_t *self, axis_shared_ptr_t *cmd, axis_error_t *err) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");
  axis_ASSERT(cmd && axis_cmd_base_check_integrity(cmd), "Should not happen.");

  axis_string_t *predefined_graph_name =
      axis_cmd_start_graph_get_predefined_graph_name(cmd);
  if (axis_string_is_empty(predefined_graph_name)) {
    return true;
  }

  axis_list_t *extensions_info = axis_cmd_start_graph_get_extensions_info(cmd);
  axis_list_t *extension_groups_info =
      axis_cmd_start_graph_get_extension_groups_info(cmd);

  bool res = axis_app_get_predefined_graph_extensions_and_groups_info_by_name(
      self, axis_string_get_raw_str(predefined_graph_name), extensions_info,
      extension_groups_info, err);
  axis_ASSERT(res, "should not happen.");
  if (!res) {
    return false;
  }

  return true;
}

static bool axis_app_check_start_graph_cmd_from_connection(
    axis_app_t *self, axis_connection_t *connection, axis_shared_ptr_t *cmd,
    axis_error_t *err) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Invalid argument.");
  axis_ASSERT(cmd && axis_cmd_base_check_integrity(cmd), "Invalid argument.");
  axis_ASSERT(axis_msg_get_type(cmd) == axis_MSG_TYPE_CMD_START_GRAPH,
             "Invalid argument.");
  axis_ASSERT(err && axis_error_check_integrity(err), "Invalid argument.");

  bool rc = axis_app_check_start_graph_cmd(self, cmd, err);
  if (!rc && connection) {
    axis_shared_ptr_t *ret_cmd =
        axis_cmd_result_create_from_cmd(axis_STATUS_CODE_ERROR, cmd);
    axis_msg_set_property(ret_cmd, axis_STR_DETAIL,
                         axis_value_create_string(axis_error_errmsg(err)), NULL);
    axis_msg_clear_and_set_dest_from_msg_src(ret_cmd, cmd);
    axis_connection_send_msg(connection, ret_cmd);
    axis_shared_ptr_destroy(ret_cmd);
  }

  return rc;
}

bool axis_app_handle_start_graph_cmd(axis_app_t *self,
                                    axis_connection_t *connection,
                                    axis_shared_ptr_t *cmd, axis_error_t *err) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Invalid argument.");
  axis_ASSERT(cmd && axis_cmd_base_check_integrity(cmd), "Invalid argument.");
  axis_ASSERT(axis_msg_get_type(cmd) == axis_MSG_TYPE_CMD_START_GRAPH,
             "Invalid argument.");
  axis_ASSERT(axis_msg_get_dest_cnt(cmd) == 1, "Invalid argument.");
  axis_ASSERT(
      connection ? axis_app_has_orphan_connection(self, connection) : true,
      "Invalid argument.");

  // If the start_graph command is aimed at initting from a predefined graph, we
  // should append the extension info list of the predefined graph to the cmd.
  if (!axis_app_fill_start_graph_cmd_extensions_info_from_predefined_graph(
          self, cmd, err)) {
    return false;
  }

  axis_engine_t *engine =
      axis_app_get_engine_based_on_dest_graph_id_from_msg(self, cmd);
  if (engine == NULL) {
    // The graph should be only checked once.
    if (!axis_app_check_start_graph_cmd_from_connection(self, connection, cmd,
                                                       err)) {
      axis_LOGE("[%s] Failed to check start_graph cmd, %s",
               axis_app_get_uri(self), axis_error_errmsg(err));
      return false;
    }

    // The engine does not exist, create one, and send 'cmd' to the newly
    // created engine.
    engine = axis_app_create_engine(self, cmd);
  } else {
    // The engine of the graph has already been created, this condition would be
    // hit in polygon graph.
  }

  // No matter the situation, it is up to the engine to handle the connect
  // command and return the corresponding cmd result.
  axis_app_do_connection_migration_or_push_to_engine_queue(connection, engine,
                                                          cmd);

  return true;
}
