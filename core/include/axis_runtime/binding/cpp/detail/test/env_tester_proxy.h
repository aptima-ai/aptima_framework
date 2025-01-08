
//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <functional>

#include "axis_runtime/binding/cpp/detail/test/env_tester.h"
#include "axis_runtime/test/env_tester_proxy.h"

using axis_env_tester_t = struct axis_env_tester_t;
using axis_env_tester_proxy_t = struct axis_env_tester_proxy_t;

namespace ten {

namespace {

using tester_notify_std_func_t = std::function<void(axis_env_tester_t &)>;

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

inline void proxy_notify(::axis_env_tester_t *axis_env, void *data = nullptr) {
  axis_ASSERT(data, "Invalid argument.");

  auto *info = static_cast<tester_proxy_notify_info_t *>(data);
  auto *cpp_axis_env =
      static_cast<axis_env_tester_t *>(axis_binding_handle_get_me_in_target_lang(
          reinterpret_cast<axis_binding_handle_t *>(axis_env)));

  if (info->notify_std_func != nullptr) {
    auto func = info->notify_std_func;
    func(*cpp_axis_env);
  }

  delete info;
}

}  // namespace

class axis_env_tester_proxy_t {
 private:
  // Passkey Idiom.
  struct ctor_passkey_t {
   private:
    friend axis_env_tester_proxy_t;

    explicit ctor_passkey_t() = default;
  };

 public:
  // @{
  axis_env_tester_proxy_t(const axis_env_tester_proxy_t &) = delete;
  axis_env_tester_proxy_t(axis_env_tester_proxy_t &&) = delete;
  axis_env_tester_proxy_t &operator=(const axis_env_tester_proxy_t &) = delete;
  axis_env_tester_proxy_t &operator=(const axis_env_tester_proxy_t &&) = delete;
  // @{

  static axis_env_tester_proxy_t *create(axis_env_tester_t &axis_env_tester,
                                        error_t *err = nullptr) {
    return new axis_env_tester_proxy_t(axis_env_tester, ctor_passkey_t());
  }

  explicit axis_env_tester_proxy_t(axis_env_tester_t &axis_env_tester,
                                  ctor_passkey_t /*unused*/)
      : c_axis_env_tester_proxy(axis_env_tester_proxy_create(
            axis_env_tester.c_axis_env_tester, nullptr)) {}

  ~axis_env_tester_proxy_t() {
    if (c_axis_env_tester_proxy == nullptr) {
      axis_ASSERT(0, "Invalid argument.");
    }

    bool rc = axis_env_tester_proxy_release(c_axis_env_tester_proxy, nullptr);
    axis_ASSERT(rc, "Should not happen.");

    c_axis_env_tester_proxy = nullptr;
  };

  bool notify(tester_notify_std_func_t &&notify_func, error_t *err = nullptr) {
    if (c_axis_env_tester_proxy == nullptr) {
      axis_ASSERT(0, "Invalid argument.");
      return false;
    }

    auto *info = new tester_proxy_notify_info_t(std::move(notify_func));

    auto rc = axis_env_tester_proxy_notify(
        c_axis_env_tester_proxy, proxy_notify, info,
        err != nullptr ? err->get_c_error() : nullptr);
    if (!rc) {
      delete info;
    }

    return rc;
  }

 private:
  ::axis_env_tester_proxy_t *c_axis_env_tester_proxy;
};
}  // namespace ten