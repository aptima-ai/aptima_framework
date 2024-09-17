//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <memory>

#include "ten_utils/macro/check.h"
#include "ten_runtime/binding/cpp/internal/msg/msg.h"
#include "ten_runtime/msg/cmd/cmd.h"
#include "ten_runtime/msg/msg.h"
#include "ten_utils/lib/smart_ptr.h"

namespace ten {

class extension_t;

class cmd_t : public msg_t {
 private:
  friend extension_t;

  // Passkey Idiom.
  struct ctor_passkey_t {
   private:
    friend cmd_t;

    explicit ctor_passkey_t() = default;
  };

 public:
  static std::unique_ptr<cmd_t> create(const char *cmd_name,
                                       error_t *err = nullptr) {
    ten_shared_ptr_t *c_cmd = ten_cmd_create(
        cmd_name,
        err != nullptr ? err->get_internal_representation() : nullptr);

    return std::make_unique<cmd_t>(c_cmd, ctor_passkey_t());
  }

  static std::unique_ptr<cmd_t> create_from_json(const char *json_str,
                                                 error_t *err = nullptr)
      __attribute__((warning("This method may access the '_ten' field. Use "
                             "caution if '_ten' is provided."))) {
    ten_shared_ptr_t *c_cmd = ten_cmd_create_from_json_string(
        json_str,
        err != nullptr ? err->get_internal_representation() : nullptr);

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
  void clone_internal(const cmd_t &cmd) noexcept {
    if (cmd.c_msg_ != nullptr) {
      c_msg_ = ten_msg_clone(cmd.c_msg_, nullptr);
    } else {
      c_msg_ = nullptr;
    }
  }
};

}  // namespace ten
