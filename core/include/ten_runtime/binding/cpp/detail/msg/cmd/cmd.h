//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <memory>

#include "ten_runtime/binding/cpp/detail/msg/msg.h"
#include "ten_runtime/msg/cmd/cmd.h"
#include "ten_runtime/msg/msg.h"
#include "ten_utils/lib/smart_ptr.h"

namespace ten {

class extension_t;
class extension_tester_t;

class cmd_t : public msg_t {
 private:
  // Passkey Idiom.
  struct ctor_passkey_t {
   private:
    friend cmd_t;

    explicit ctor_passkey_t() = default;
  };

 public:
  static std::unique_ptr<cmd_t> create(const char *name,
                                       error_t *err = nullptr) {
    ten_shared_ptr_t *c_cmd =
        ten_cmd_create(name, err != nullptr ? err->get_c_error() : nullptr);

    return std::make_unique<cmd_t>(c_cmd, ctor_passkey_t());
  }

  explicit cmd_t(ten_shared_ptr_t *cmd, ctor_passkey_t /*unused*/)
      : msg_t(cmd) {}

  cmd_t() = default;
  ~cmd_t() override = default;

  // @{
  cmd_t(const cmd_t &other) noexcept = delete;
  cmd_t(cmd_t &&other) noexcept = delete;
  cmd_t &operator=(const cmd_t &cmd) noexcept = delete;
  cmd_t &operator=(cmd_t &&cmd) noexcept = delete;
  // @}

 protected:
  // @{
  // Used by the constructor of the real command class to create a base command
  // first.
  explicit cmd_t(ten_shared_ptr_t *cmd) : msg_t(cmd) {}
  // @}

 private:
  friend extension_t;
  friend extension_tester_t;

  void clone_internal(const cmd_t &cmd) noexcept {
    if (cmd.c_msg != nullptr) {
      c_msg = ten_msg_clone(cmd.c_msg, nullptr);
    } else {
      c_msg = nullptr;
    }
  }
};

}  // namespace ten
