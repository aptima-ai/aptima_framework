//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/engine/msg_interface/start_graph.h"

#include "include_internal/ten_runtime/connection/connection.h"
#include "include_internal/ten_runtime/engine/engine.h"
#include "include_internal/ten_runtime/engine/internal/close.h"
#include "include_internal/ten_runtime/engine/internal/extension_interface.h"
#include "include_internal/ten_runtime/engine/internal/remote_interface.h"
#include "include_internal/ten_runtime/engine/msg_interface/common.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd/start_graph/cmd.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_runtime/path/path.h"
#include "include_internal/ten_runtime/path/path_group.h"
#include "include_internal/ten_runtime/remote/remote.h"
#include "include_internal/ten_utils/log/log.h"
#include "include_internal/ten_utils/value/value.h"
#include "ten_runtime/msg/cmd_result/cmd_result.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_ptr.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/macro/check.h"

void ten_engine_handle_cmd_start_graph(ten_engine_t *self,
                                       ten_shared_ptr_t *cmd,
                                       ten_error_t *err) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(cmd, "Invalid argument.");
  TEN_ASSERT(ten_engine_check_integrity(self, true),
             "The usage of the engine is incorrect.");
  TEN_ASSERT(
      ten_msg_get_type(cmd) == TEN_MSG_TYPE_CMD_START_GRAPH,
      "The command this function handles should be a 'start_graph' command.");
  TEN_ASSERT(ten_msg_get_src_app_uri(cmd),
             "The 'start_graph' command should have a src_uri information.");

  ten_cmd_start_graph_add_missing_extension_group_node(cmd);

  ten_list_t next = TEN_LIST_INIT_VAL;
  ten_cmd_start_graph_get_next_list(cmd, self->app, &next);

  if (ten_list_is_empty(&next)) {
    TEN_LOGD(
        "No more extensions need to be connected in the graph, enable the "
        "extension system now.");

    ten_engine_enable_extension_system(self, cmd, err);
  } else {
    // There are more apps need to be connected, so connect them now.
    ten_list_t new_works = TEN_LIST_INIT_VAL;
    bool error_occurred = false;

    ten_list_foreach (&next, iter) {
      ten_string_t *dest_uri = ten_str_listnode_get(iter.node);
      TEN_ASSERT(dest_uri, "Invalid argument.");

      const char *dest_uri_c_str = ten_string_get_raw_str(dest_uri);
      TEN_ASSERT(dest_uri_c_str, "Invalid argument.");

      TEN_LOGD("Check if we have connected to %s.", dest_uri_c_str);

      // Check to see if we have connected to this URI or not.
      ten_remote_t *remote =
          ten_engine_check_remote_is_existed(self, dest_uri_c_str);
      if (!remote) {
        TEN_LOGD("%s is unconnected, connect now.", dest_uri_c_str);

        ten_shared_ptr_t *child_cmd = ten_msg_clone(cmd, NULL);
        TEN_ASSERT(child_cmd, "Should not happen.");

        // Use the uri of the local app to fill/override the value of 'from'
        // field (even if there is any old value in the 'from' field), so that
        // the remote could know who connects to them.
        ten_msg_set_src_to_engine(child_cmd, self);

        // Correct the destination information of the 'start_graph' command.
        ten_msg_clear_and_set_dest(child_cmd, dest_uri_c_str,
                                   ten_string_get_raw_str(&self->graph_id),
                                   NULL, NULL, err);

        ten_path_t *out_path = (ten_path_t *)ten_path_table_add_out_path(
            self->path_table, child_cmd);
        TEN_ASSERT(out_path && ten_path_check_integrity(out_path, true),
                   "Should not happen.");

        ten_list_push_ptr_back(&new_works, out_path, NULL);

        bool rc = ten_engine_connect_to_graph_remote(
            self, ten_string_get_raw_str(dest_uri), child_cmd);
        if (!rc) {
          TEN_LOGE("Failed to connect to %s.", dest_uri_c_str);
          error_occurred = true;
          ten_shared_ptr_destroy(child_cmd);
          ten_engine_return_error_for_cmd_start_graph(
              self, cmd, "Failed to connect to %s.", dest_uri_c_str);
          break;
        }
      } else {
        TEN_LOGD("%s is connected, there is nothing to do.", dest_uri_c_str);
      }
    }

    if (error_occurred) {
      // An error occurred, so we should not continue to connect to the
      // remaining apps (remotes).
      ten_list_clear(&new_works);
    } else {
      if (!ten_list_is_empty(&new_works)) {
        // This means that we can _not_ start the engine now. We must wait for
        // these newly submitted 'start_graph' commands to be completed in order
        // to start the engine, so we must save the current received
        // 'start_graph' command (to prevent it from being destroyed) in order
        // to return a correct cmd result according to it.
        TEN_ASSERT(!self->original_start_graph_cmd_of_enabling_engine,
                   "Should not happen.");
        self->original_start_graph_cmd_of_enabling_engine =
            ten_shared_ptr_clone(cmd);

        if (ten_list_size(&new_works) > 1) {
          // Create path group for these newly submitted 'start_graph' commands.
          ten_paths_create_group(
              &new_works, TEN_RESULT_RETURN_POLICY_FIRST_ERROR_OR_LAST_OK);
        }
        ten_list_clear(&new_works);

        TEN_LOGD(
            "Create a IN path for the receiving 'start_graph' command: %s.",
            ten_cmd_base_get_cmd_id(cmd));
        ten_path_table_add_in_path(self->path_table, cmd, NULL);
      } else {
        TEN_LOGD(
            "No more new connections should be made, enable the extension "
            "system now.");

        ten_engine_enable_extension_system(self, cmd, err);
      }
    }
  }

  ten_list_clear(&next);
}

void ten_engine_return_ok_for_cmd_start_graph(
    ten_engine_t *self, ten_shared_ptr_t *cmd_start_graph) {
  TEN_ASSERT(self && ten_engine_check_integrity(self, true),
             "Invalid argument");
  TEN_ASSERT(cmd_start_graph && ten_cmd_base_check_integrity(cmd_start_graph),
             "Invalid argument.");

  ten_shared_ptr_t *ret_cmd =
      ten_cmd_result_create_from_cmd(TEN_STATUS_CODE_OK, cmd_start_graph);
  ten_msg_set_property(ret_cmd, "detail", ten_value_create_string(""), NULL);

  ten_engine_dispatch_msg(self, ret_cmd);  // Send back the cmd result.
  ten_shared_ptr_destroy(ret_cmd);         // Delete the cmd result.
}

void ten_engine_return_error_for_cmd_start_graph(
    ten_engine_t *self, ten_shared_ptr_t *cmd_start_graph, const char *fmt,
    ...) {
  TEN_ASSERT(self && ten_engine_check_integrity(self, true),
             "Invalid argument.");
  TEN_ASSERT(cmd_start_graph && ten_cmd_base_check_integrity(cmd_start_graph),
             "The engine should be started because of receiving a "
             "'start_graph' command.");

  {
    // Return an error result.
    va_list ap;
    va_start(ap, fmt);

    // Return an error to the previous graph stage.
    ten_shared_ptr_t *ret_cmd =
        ten_cmd_result_create_from_cmd(TEN_STATUS_CODE_ERROR, cmd_start_graph);
    ten_msg_set_property(ret_cmd, "detail", ten_value_create_vastring(fmt, ap),
                         NULL);

    va_end(ap);

    ten_engine_dispatch_msg(self, ret_cmd);  // Send out the returned cmd.

    ten_shared_ptr_destroy(ret_cmd);  // Delete the returned cmd.
  }

  if (self->original_start_graph_cmd_of_enabling_engine) {
    // 'original_start_graph_cmd_of_enabling_engine' is useless from now on.
    ten_shared_ptr_destroy(self->original_start_graph_cmd_of_enabling_engine);
    self->original_start_graph_cmd_of_enabling_engine = NULL;
  }

  // The graph construction is failed, so the engine have to be closed now.
  // (There could be some 'retrying' mechanism in the protocol layer to mitigate
  // some seldom network problem, and if all the retrying are failed, this
  // function would indeed be called.)
  //
  // The closing of the engine might make the above error result unable to
  // been sent it out (because the 'if (xxx_is_closing())' checks in each
  // layer). However, some new mechanism could be invented in the future to
  // ensure the error result could be sent out successfully. So for integrity,
  // an error result is still constructed and issued above.
  //
  // TODO(Wei): There should be a such mechanism to ensure the error result to
  // be sent out successfully.
  //
  // TODO(Wei): Need to have a mechanism to prevent the engine from being
  // constructed repeatedly in a scenario contains multiple TEN app.

  // Close the engine.
  ten_engine_close_async(self);
}
