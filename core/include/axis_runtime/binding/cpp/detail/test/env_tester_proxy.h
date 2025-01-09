
//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <functional>

#include "aptima_runtime/binding/cpp/detail/test/env_tester.h"
#include "aptima_runtime/test/env_tester_proxy.h"

using aptima_env_tester_t = struct aptima_env_tester_t;
using aptima_env_tester_proxy_t = struct aptima_env_tester_proxy_t;

namespace ten {

namespace {

using tester_notify_std_func_t = std::function<void(aptima_env_tester_t &)>;

struct tester_proxy_notify_info_t {
  explicit tester_proxy_notify_info_t(tester_notify_std_func_t &&func)
      : notify_std_func(std::move(func)) {}

  ~tester_proxy_notify_info_t() = default;

  // @{
  tester_proxy_notify_info_t(const tester_proxy_notify_info_t &) = delete;
  tester_proxy_notify_info_t &operator=(const tester_proxy_notify_info_t &) =
      delete;
  tester_proxy_notify_info_t(tester_proxy_notify_info_t &&) = delete;
  tester_proxy_notify_info_t &operator=(tester_proxy_notify_info_t &&) = delete;
  // @}

  tester_notify_std_func_t notify_std_func;
};

inline void proxy_notify(::aptima_env_tester_t *aptima_env, void *data = nullptr) {
  aptima_ASSERT(data, "Invalid argument.");

  auto *info = static_cast<tester_proxy_notify_info_t *>(data);
  auto *cpp_aptima_env =
      static_cast<aptima_env_tester_t *>(aptima_binding_handle_get_me_in_target_lang(
          reinterpret_cast<aptima_binding_handle_t *>(aptima_env)));

  if (info->notify_std_func != nullptr) {
    auto func = info->notify_std_func;
    func(*cpp_aptima_env);
  }

  delete info;
}

}  // namespace

class aptima_env_tester_proxy_t {
 private:
  // Passkey Idiom.
  struct ctor_passkey_t {
   private:
    friend aptima_env_tester_proxy_t;

    explicit ctor_passkey_t() = default;
  };

 public:
  // @{
  aptima_env_tester_proxy_t(const aptima_env_tester_proxy_t &) = delete;
  aptima_env_tester_proxy_t(aptima_env_tester_proxy_t &&) = delete;
  aptima_env_tester_proxy_t &operator=(const aptima_env_tester_proxy_t &) = delete;
  aptima_env_tester_proxy_t &operator=(const aptima_env_tester_proxy_t &&) = delete;
  // @{

  static aptima_env_tester_proxy_t *create(aptima_env_tester_t &aptima_env_tester,
                                        error_t *err = nullptr) {
    return new aptima_env_tester_proxy_t(aptima_env_tester, ctor_passkey_t());
  }

  explicit aptima_env_tester_proxy_t(aptima_env_tester_t &aptima_env_tester,
                                  ctor_passkey_t /*unused*/)
      : c_aptima_env_tester_proxy(aptima_env_tester_proxy_create(
            aptima_env_tester.c_aptima_env_tester, nullptr)) {}

  ~aptima_env_tester_proxy_t() {
    if (c_aptima_env_tester_proxy == nullptr) {
      aptima_ASSERT(0, "Invalid argument.");
    }

    bool rc = aptima_env_tester_proxy_release(c_aptima_env_tester_proxy, nullptr);
    aptima_ASSERT(rc, "Should not happen.");

    c_aptima_env_tester_proxy = nullptr;
  };

  bool notify(tester_notify_std_func_t &&notify_func, error_t *err = nullptr) {
    if (c_aptima_env_tester_proxy == nullptr) {
      aptima_ASSERT(0, "Invalid argument.");
      return false;
    }

    auto *info = new tester_proxy_notify_info_t(std::move(notify_func));

    auto rc = aptima_env_tester_proxy_notify(
        c_aptima_env_tester_proxy, proxy_notify, info,
        err != nullptr ? err->get_c_error() : nullptr);
    if (!rc) {
      delete info;
    }

    return rc;
  }

 private:
  ::aptima_env_tester_proxy_t *c_aptima_env_tester_proxy;
};
}  // namespace ten