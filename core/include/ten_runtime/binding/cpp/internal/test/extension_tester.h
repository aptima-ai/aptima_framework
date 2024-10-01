//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <memory>

#include "ten_runtime/binding/common.h"
#include "ten_runtime/binding/cpp/internal/msg/cmd/cmd.h"
#include "ten_runtime/binding/cpp/internal/test/ten_env_tester.h"
#include "ten_runtime/test/extension_tester.h"
#include "ten_utils/macro/check.h"

namespace ten {

class extension_tester_t {
 public:
  virtual ~extension_tester_t() {
    TEN_ASSERT(c_extension_tester, "Should not happen.");
    ten_extension_tester_destroy(c_extension_tester);

    TEN_ASSERT(cpp_ten_env_tester, "Should not happen.");
    delete cpp_ten_env_tester;
  }

  // @{
  extension_tester_t(const extension_tester_t &) = delete;
  extension_tester_t(extension_tester_t &&) = delete;
  extension_tester_t &operator=(const extension_tester_t &) = delete;
  extension_tester_t &operator=(const extension_tester_t &&) = delete;
  // @}

  void add_addon(const char *addon_name) {
    TEN_ASSERT(addon_name, "Invalid argument.");
    ten_extension_tester_add_addon(c_extension_tester, addon_name);
  }

  void run(error_t *err = nullptr) {
    if (c_extension_tester == nullptr) {
      return;
    }

    ten_extension_tester_run(c_extension_tester);
  }

 protected:
  explicit extension_tester_t()
      : c_extension_tester(::ten_extension_tester_create(
            reinterpret_cast<ten_extension_tester_on_start_func_t>(
                &proxy_on_start),
            reinterpret_cast<ten_extension_tester_on_cmd_func_t>(
                &proxy_on_cmd))) {
    TEN_ASSERT(c_extension_tester, "Should not happen.");

    ten_binding_handle_set_me_in_target_lang(
        reinterpret_cast<ten_binding_handle_t *>(c_extension_tester),
        static_cast<void *>(this));

    cpp_ten_env_tester = new ten_env_tester_t(
        ten_extension_tester_get_ten_env_tester(c_extension_tester));
    TEN_ASSERT(cpp_ten_env_tester, "Should not happen.");
  }

  virtual void on_start(ten_env_tester_t &ten_env_tester) {
    ten_env_tester.on_start_done();
  }

  virtual void on_cmd(ten_env_tester_t &ten_env_tester,
                      std::unique_ptr<cmd_t> cmd) {}

 private:
  void invoke_cpp_extension_tester_on_start(
      ten_env_tester_t &cpp_ten_env_tester) {
    on_start(cpp_ten_env_tester);
  }

  static void proxy_on_start(ten_extension_tester_t *tester,
                             ::ten_env_tester_t *c_ten_env_tester) {
    TEN_ASSERT(tester && c_ten_env_tester, "Should not happen.");

    auto *cpp_extension_tester = static_cast<extension_tester_t *>(
        ten_binding_handle_get_me_in_target_lang(
            reinterpret_cast<ten_binding_handle_t *>(tester)));
    auto *cpp_ten_env_tester = static_cast<ten_env_tester_t *>(
        ten_binding_handle_get_me_in_target_lang(
            reinterpret_cast<ten_binding_handle_t *>(c_ten_env_tester)));

    cpp_extension_tester->invoke_cpp_extension_tester_on_start(
        *cpp_ten_env_tester);
  }

  void invoke_cpp_extension_on_cmd(ten_env_tester_t &cpp_ten_env_tester,
                                   std::unique_ptr<cmd_t> cmd) {
    on_cmd(cpp_ten_env_tester, std::move(cmd));
  }

  static void proxy_on_cmd(ten_extension_tester_t *extension_tester,
                           ::ten_env_tester_t *c_ten_env_tester,
                           ten_shared_ptr_t *cmd) {
    TEN_ASSERT(extension_tester && c_ten_env_tester && cmd,
               "Should not happen.");

    auto *cpp_extension_tester = static_cast<extension_tester_t *>(
        ten_binding_handle_get_me_in_target_lang(
            reinterpret_cast<ten_binding_handle_t *>(extension_tester)));
    auto *cpp_ten_env_tester = static_cast<ten_env_tester_t *>(
        ten_binding_handle_get_me_in_target_lang(
            reinterpret_cast<ten_binding_handle_t *>(c_ten_env_tester)));

    // Clone a C shared_ptr to be owned by the C++ instance.
    cmd = ten_shared_ptr_clone(cmd);

    auto *cpp_cmd_ptr = new cmd_t(cmd);
    auto cpp_cmd_unique_ptr = std::unique_ptr<cmd_t>(cpp_cmd_ptr);

    cpp_extension_tester->invoke_cpp_extension_on_cmd(
        *cpp_ten_env_tester, std::move(cpp_cmd_unique_ptr));
  }

  ::ten_extension_tester_t *c_extension_tester;
  ten_env_tester_t *cpp_ten_env_tester;
};

}  // namespace ten
