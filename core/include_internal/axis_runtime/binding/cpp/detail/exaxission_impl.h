//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <cassert>
#include <cstddef>
#include <cstdlib>

#include "include_internal/axis_runtime/binding/cpp/detail/msg/cmd/timeout.h"
#include "include_internal/axis_runtime/binding/cpp/detail/msg/cmd/timer.h"
#include "axis_runtime/binding/cpp/detail/extension.h"
#include "axis_runtime/binding/cpp/detail/msg/cmd/close_app.h"
#include "axis_runtime/binding/cpp/detail/msg/cmd/start_graph.h"
#include "axis_runtime/binding/cpp/detail/msg/cmd/stop_graph.h"

namespace aptima {

inline void extension_t::proxy_on_cmd_internal(
    axis_extension_t *extension, ::axis_env_t *axis_env, axis_shared_ptr_t *cmd,
    cpp_extension_on_cmd_func_t on_cmd_func) {
  axis_ASSERT(extension && axis_env, "Should not happen.");

  auto *cpp_extension =
      static_cast<extension_t *>(axis_binding_handle_get_me_in_target_lang(
          reinterpret_cast<axis_binding_handle_t *>(extension)));
  auto *cpp_axis_env =
      static_cast<axis_env_t *>(axis_binding_handle_get_me_in_target_lang(
          reinterpret_cast<axis_binding_handle_t *>(axis_env)));

  // Clone a C shared_ptr to be owned by the C++ instance.
  cmd = axis_shared_ptr_clone(cmd);

  cmd_t *cpp_cmd_ptr = nullptr;
  switch (axis_msg_get_type(cmd)) {
    case axis_MSG_TYPE_CMD_START_GRAPH:
      cpp_cmd_ptr = new cmd_start_graph_t(cmd);
      break;

    case axis_MSG_TYPE_CMD_TIMER:
      cpp_cmd_ptr = new cmd_timer_t(cmd);
      break;

    case axis_MSG_TYPE_CMD_TIMEOUT:
      cpp_cmd_ptr = new cmd_timeout_t(cmd);
      break;

    case axis_MSG_TYPE_CMD_STOP_GRAPH:
      cpp_cmd_ptr = new cmd_stop_graph_t(cmd);
      break;

    case axis_MSG_TYPE_CMD_CLOSE_APP:
      cpp_cmd_ptr = new cmd_close_app_t(cmd);
      break;

    case axis_MSG_TYPE_CMD:
      cpp_cmd_ptr = new cmd_t(cmd);
      break;

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  auto cpp_cmd_unique_ptr = std::unique_ptr<cmd_t>(cpp_cmd_ptr);

  cpp_extension->invoke_cpp_extension_on_cmd(
      *cpp_axis_env, std::move(cpp_cmd_unique_ptr), on_cmd_func);
}

}  // namespace aptima
