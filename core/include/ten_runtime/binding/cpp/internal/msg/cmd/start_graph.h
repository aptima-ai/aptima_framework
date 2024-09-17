//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <memory>

#include "ten_runtime/binding/cpp/internal/msg/cmd/cmd.h"
#include "ten_runtime/msg/cmd/start_graph/cmd.h"

namespace ten {

class extension_t;

class cmd_start_graph_t : public cmd_t {
 private:
  friend extension_t;

  // Passkey Idiom.
  struct ctor_passkey_t {
   private:
    friend cmd_start_graph_t;

    explicit ctor_passkey_t() = default;
  };

  explicit cmd_start_graph_t(ten_shared_ptr_t *cmd) : cmd_t(cmd) {}

 public:
  static std::unique_ptr<cmd_start_graph_t> create(error_t *err = nullptr) {
    return std::make_unique<cmd_start_graph_t>(ctor_passkey_t());
  }

  explicit cmd_start_graph_t(ctor_passkey_t /*unused*/)
      : cmd_t(ten_cmd_start_graph_create()) {};
  ~cmd_start_graph_t() override = default;

  // @{
  cmd_start_graph_t(cmd_start_graph_t &other) = delete;
  cmd_start_graph_t(cmd_start_graph_t &&other) = delete;
  cmd_start_graph_t &operator=(const cmd_start_graph_t &cmd) = delete;
  cmd_start_graph_t &operator=(cmd_start_graph_t &&cmd) = delete;
  // @}
};

}  // namespace ten
