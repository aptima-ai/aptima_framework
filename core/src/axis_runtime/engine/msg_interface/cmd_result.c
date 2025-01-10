//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/msg/cmd_result/cmd_result.h"

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/connection/connection.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/engine/internal/close.h"
#include "include_internal/axis_runtime/engine/internal/extension_interface.h"
#include "include_internal/axis_runtime/engine/internal/remote_interface.h"
#include "include_internal/axis_runtime/engine/msg_interface/common.h"
#include "include_internal/axis_runtime/engine/msg_interface/start_graph.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd_result/cmd.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/path/path_table.h"
#include "include_internal/axis_runtime/remote/remote.h"
#include "include_internal/axis_utils/log/log.h"
#include "axis_runtime/common/status_code.h"
#include "axis_runtime/msg/msg.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value_get.h"
#include "axis_utils/value/value_is.h"

static bool axis_engine_close_duplicated_remote_or_upgrade_it_to_normal(
    axis_engine_t *self, axis_shared_ptr_t *cmd_result, axis_error_t *err) {
  axis_ASSERT(self && axis_engine_check_integrity(self, true) && cmd_result &&
                 axis_cmd_base_check_integrity(cmd_result),
             "Should not happen.");

  axis_connection_t *connection =
      axis_cmd_base_get_original_connection(cmd_result);
  axis_ASSERT(connection && axis_connection_check_integrity(connection, true) &&
                 axis_connection_attach_to(connection) ==
                     axis_CONNECTION_ATTACH_TO_REMOTE,
             "Should not happen.");

  axis_remote_t *remote = connection->attached_target.remote;
  axis_ASSERT(remote, "Invalid argument.");
  axis_ASSERT(axis_remote_check_integrity(remote, true),
             "Invalid use of remote %p.", remote);

  axis_ASSERT(axis_engine_check_remote_is_weak(self, remote),
             "%p should be a weak remote.", remote);

  axis_string_t detail_str;
  axis_string_init(&detail_str);

  axis_value_t *detail_value = axis_msg_peek_property(cmd_result, "detail", NULL);
  if (!detail_value || !axis_value_is_string(detail_value)) {
    axis_ASSERT(0, "Should not happen.");
  }

  bool rc = axis_value_to_string(detail_value, &detail_str, err);
  axis_ASSERT(rc, "Should not happen.");

  if (axis_string_is_equal_c_str(&detail_str, axis_STR_DUPLICATE)) {
    axis_LOGW("Receives a 'duplicate' result from %s",
             axis_string_get_raw_str(&remote->uri));

    // This is a duplicated channel, closing it now.
    axis_connection_t *connection = remote->connection;
    axis_ASSERT(connection && axis_connection_check_integrity(connection, true),
               "Should not happen.");

    connection->duplicate = true;

    axis_connection_close(connection);
  } else {
    // The 'start_graph' is done, change this remote from weak-type to
    // normal-type.
    axis_engine_upgrade_weak_remote_to_normal_remote(self, remote);
  }

  axis_string_deinit(&detail_str);

  return true;
}

static bool axis_engine_handle_cmd_result_for_cmd_start_graph(
    axis_engine_t *self, axis_shared_ptr_t *cmd_result, axis_error_t *err) {
  axis_ASSERT(self && axis_engine_check_integrity(self, true),
             "Invalid argument.");
  axis_ASSERT(cmd_result &&
                 axis_msg_get_type(cmd_result) == axis_MSG_TYPE_CMD_RESULT &&
                 axis_msg_get_dest_cnt(cmd_result) == 1,
             "Should not happen.");
  axis_ASSERT(axis_c_string_is_equal(
                 axis_app_get_uri(self->app),
                 axis_string_get_raw_str(
                     &axis_msg_get_first_dest_loc(cmd_result)->app_uri)),
             "Should not happen.");

  if (axis_cmd_result_get_status_code(cmd_result) == axis_STATUS_CODE_OK) {
    if (axis_cmd_base_get_original_connection(cmd_result)) {
      // Only if the 'start_graph' flow involves a connection, we need to handle
      // situations relevant to that connection.

      bool rc = axis_engine_close_duplicated_remote_or_upgrade_it_to_normal(
          self, cmd_result, err);
      axis_ASSERT(rc, "Should not happen.");
    }
  }

  // Find the corresponding OUT path of the cmd_result.
  axis_path_t *out_path =
      axis_path_table_set_result(self->path_table, axis_PATH_OUT, cmd_result);
  if (!out_path) {
    axis_LOGD(
        "The 'start_graph' flow was failed before, discard the cmd_result "
        "now.");
    return true;
  }

  cmd_result = axis_path_table_determine_actual_cmd_result(
      self->path_table, axis_PATH_OUT, out_path, true);
  if (!cmd_result) {
    axis_LOGD(
        "The 'start_graph' flow is not completed, skip the cmd_result now.");
    return true;
  }

  // The processing of the 'start_graph' flows are completed.

  if (axis_cmd_result_get_status_code(cmd_result) == axis_STATUS_CODE_OK) {
    // All the later connection stages are completed, enable the Extension
    // system now.

    axis_shared_ptr_t *original_start_graph_cmd =
        self->original_start_graph_cmd_of_enabling_engine;
    axis_ASSERT(
        original_start_graph_cmd &&
            axis_cmd_base_check_integrity(original_start_graph_cmd),
        "The engine should be started because of receiving a 'start_graph' "
        "command.");

    axis_error_t err;
    axis_error_init(&err);

    axis_engine_enable_extension_system(self, original_start_graph_cmd, &err);

    axis_error_deinit(&err);
  } else if (axis_cmd_result_get_status_code(cmd_result) ==
             axis_STATUS_CODE_ERROR) {
    axis_shared_ptr_t *original_start_graph_cmd =
        self->original_start_graph_cmd_of_enabling_engine;
    axis_ASSERT(
        original_start_graph_cmd &&
            axis_cmd_base_check_integrity(original_start_graph_cmd),
        "The engine should be started because of receiving a 'start_graph' "
        "command.");

    axis_value_t *err_msg_value =
        axis_msg_peek_property(cmd_result, axis_STR_DETAIL, NULL);
    if (err_msg_value) {
      axis_ASSERT(axis_value_is_string(err_msg_value), "Should not happen.");
      axis_engine_return_error_for_cmd_start_graph(
          self, original_start_graph_cmd,
          axis_value_peek_raw_str(err_msg_value, err));
    } else {
      axis_engine_return_error_for_cmd_start_graph(
          self, original_start_graph_cmd, "Failed to start engine in app [%s].",
          axis_msg_get_src_app_uri(cmd_result));
    }
  } else {
    axis_ASSERT(0, "Should not happen.");
  }

  axis_shared_ptr_destroy(cmd_result);

  return true;
}

void axis_engine_handle_cmd_result(axis_engine_t *self,
                                  axis_shared_ptr_t *cmd_result,
                                  axis_error_t *err) {
  axis_ASSERT(self && axis_engine_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(cmd_result &&
                 axis_msg_get_type(cmd_result) == axis_MSG_TYPE_CMD_RESULT &&
                 axis_msg_get_dest_cnt(cmd_result) == 1,
             "Should not happen.");

  switch (axis_cmd_result_get_original_cmd_type(cmd_result)) {
    case axis_MSG_TYPE_CMD_START_GRAPH: {
      bool rc = axis_engine_handle_cmd_result_for_cmd_start_graph(
          self, cmd_result, err);
      axis_ASSERT(rc, "Should not happen.");
      break;
    }

    case axis_MSG_TYPE_INVALID:
      axis_ASSERT(0, "Should not happen.");
      break;

    default:
      axis_ASSERT(0, "Handle more original command type.");
      break;
  }
}
