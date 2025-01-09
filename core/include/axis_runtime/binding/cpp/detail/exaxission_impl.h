//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <utility>

#include "aptima_runtime/binding/cpp/detail/extension.h"
#include "aptima_runtime/binding/cpp/detail/msg/cmd/close_app.h"
#include "aptima_runtime/binding/cpp/detail/msg/cmd/start_graph.h"
#include "aptima_runtime/msg/msg.h"

namespace ten {

inline void extension_t::proxy_on_cmd_internal(
    aptima_extension_t *extension, ::aptima_env_t *aptima_env, aptima_shared_ptr_t *cmd,
    cpp_extension_on_cmd_func_t on_cmd_func) {
  aptima_ASSERT(extension && aptima_env && cmd, "Should not happen.");

  auto *cpp_extension =
      static_cast<extension_t *>(aptima_binding_handle_get_me_in_target_lang(
          reinterpret_cast<aptima_binding_handle_t *>(extension)));
  auto *cpp_aptima_env =
      static_cast<aptima_env_t *>(aptima_binding_handle_get_me_in_target_lang(
          reinterpret_cast<aptima_binding_handle_t *>(aptima_env)));

  // Clone a C shared_ptr to be owned by the C++ instance.
  cmd = aptima_shared_ptr_clone(cmd);

  cmd_t *cpp_cmd_ptr = nullptr;
  switch (aptima_msg_get_type(cmd)) {
    case aptima_MSG_TYPE_CMD_START_GRAPH:
      cpp_cmd_ptr = new cmd_start_graph_t(cmd);
      break;

    case aptima_MSG_TYPE_CMD_STOP_GRAPH:
      cpp_cmd_ptr = new cmd_stop_graph_t(cmd);
      break;

    case aptima_MSG_TYPE_CMD_CLOSE_APP:
      cpp_cmd_ptr = new cmd_close_app_t(cmd);
      break;

    case aptima_MSG_TYPE_CMD:
      cpp_cmd_ptr = new cmd_t(cmd);
      break;

    default:
      aptima_ASSERT(0, "Should not happen.");
      break;
  }

  auto cpp_cmd_unique_ptr = std::unique_ptr<cmd_t>(cpp_cmd_ptr);

  cpp_extension->invoke_cpp_extension_on_cmd(
      *cpp_aptima_env, std::move(cpp_cmd_unique_ptr), on_cmd_func);
}

}  // namespace ten
