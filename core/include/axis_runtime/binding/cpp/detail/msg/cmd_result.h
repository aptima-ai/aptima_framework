//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include <memory>

#include "aptima_runtime/binding/cpp/detail/msg/msg.h"
#include "aptima_runtime/common/status_code.h"
#include "aptima_runtime/msg/cmd_result/cmd_result.h"
#include "aptima_utils/lang/cpp/lib/value.h"

namespace aptima {

class extension_t;
class aptima_env_tester_t;
class cmd_result_internal_accessor_t;

class cmd_result_t : public msg_t {
 private:
  // Passkey Idiom.
  struct ctor_passkey_t {
   private:
    friend cmd_result_t;

    explicit ctor_passkey_t() = default;
  };

 public:
  static std::unique_ptr<cmd_result_t> create(aptima_STATUS_CODE status_code,
                                              error_t *err = nullptr) {
    return std::make_unique<cmd_result_t>(status_code, ctor_passkey_t());
  }

  explicit cmd_result_t(aptima_STATUS_CODE status_code, ctor_passkey_t /*unused*/)
      : msg_t(aptima_cmd_result_create(status_code)) {}
  explicit cmd_result_t(aptima_shared_ptr_t *cmd, ctor_passkey_t /*unused*/)
      : msg_t(cmd) {};

  ~cmd_result_t() override = default;

  aptima_STATUS_CODE get_status_code(error_t *err = nullptr) const {
    return aptima_cmd_result_get_status_code(c_msg);
  }

  bool is_final(error_t *err = nullptr) const {
    return aptima_cmd_result_is_final(
        c_msg, err != nullptr ? err->get_c_error() : nullptr);
  }

  bool is_completed(error_t *err = nullptr) const {
    return aptima_cmd_result_is_completed(
        c_msg, err != nullptr ? err->get_c_error() : nullptr);
  }

  bool set_final(bool final, error_t *err = nullptr) {
    return aptima_cmd_result_set_final(
        c_msg, final, err != nullptr ? err->get_c_error() : nullptr);
  }

  // @{
  cmd_result_t(cmd_result_t &other) = delete;
  cmd_result_t(cmd_result_t &&other) = delete;
  cmd_result_t &operator=(const cmd_result_t &cmd) = delete;
  cmd_result_t &operator=(cmd_result_t &&cmd) = delete;
  // @}

 private:
  friend extension_t;
  friend aptima_env_tester_t;
  friend aptima_env_t;
  friend cmd_result_internal_accessor_t;

  static std::unique_ptr<cmd_result_t> create(aptima_shared_ptr_t *cmd,
                                              error_t *err = nullptr) {
    return std::make_unique<cmd_result_t>(cmd, ctor_passkey_t());
  }
};

}  // namespace aptima
