//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/binding/cpp/detail/axis_env.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"

using axis_extension_t = struct axis_extension_t;
using axis_env_proxy_t = struct axis_env_proxy_t;
using axis_env_t = struct axis_env_t;

namespace ten {

class extension_t;
class extension_group_t;
class app_t;

namespace {

using notify_std_func_t = std::function<void(axis_env_t &)>;
using notify_std_with_user_data_func_t =
    std::function<void(axis_env_t &, void *user_data)>;

struct proxy_notify_info_t {
  explicit proxy_notify_info_t(notify_std_func_t &&func)
      : notify_std_func(std::move(func)) {}
  explicit proxy_notify_info_t(notify_std_with_user_data_func_t &&func,
                               void *user_data)
      : notify_std_with_user_data_func(std::move(func)), user_data(user_data) {}

  ~proxy_notify_info_t() = default;

  // @{
  proxy_notify_info_t(const proxy_notify_info_t &) = delete;
  proxy_notify_info_t &operator=(const proxy_notify_info_t &) = delete;
  proxy_notify_info_t(proxy_notify_info_t &&) = delete;
  proxy_notify_info_t &operator=(proxy_notify_info_t &&) = delete;
  // @}

  notify_std_func_t notify_std_func;

  notify_std_with_user_data_func_t notify_std_with_user_data_func;
  void *user_data{};
};

inline void proxy_notify(::axis_env_t *axis_env, void *data = nullptr) {
  axis_ASSERT(data, "Invalid argument.");

  auto *info = static_cast<proxy_notify_info_t *>(data);
  auto *cpp_axis_env =
      static_cast<axis_env_t *>(axis_binding_handle_get_me_in_target_lang(
          reinterpret_cast<axis_binding_handle_t *>(axis_env)));

  if (info->notify_std_func != nullptr) {
    auto func = info->notify_std_func;
    func(*cpp_axis_env);
  } else if (info->notify_std_with_user_data_func != nullptr) {
    auto func = info->notify_std_with_user_data_func;
    func(*cpp_axis_env, info->user_data);
  }

  delete info;
}

}  // namespace

class axis_env_proxy_t {
 private:
  // Passkey Idiom.
  struct ctor_passkey_t {
   private:
    friend axis_env_proxy_t;

    explicit ctor_passkey_t() = default;
  };

 public:
  static axis_env_proxy_t *create(axis_env_t &axis_env, error_t *err = nullptr) {
    return new axis_env_proxy_t(axis_env, ctor_passkey_t());
  }

  explicit axis_env_proxy_t(axis_env_t &axis_env, ctor_passkey_t /*unused*/)
      : c_axis_env_proxy(axis_env_proxy_create(axis_env.c_axis_env, 1, nullptr)) {}

  ~axis_env_proxy_t() {
    if (c_axis_env_proxy == nullptr) {
      axis_ASSERT(0, "Invalid argument.");
    }

    bool rc = axis_env_proxy_release(c_axis_env_proxy, nullptr);
    axis_ASSERT(rc, "Should not happen.");
  };

  // @{
  axis_env_proxy_t(axis_env_proxy_t &other) = delete;
  axis_env_proxy_t(axis_env_proxy_t &&other) = delete;
  axis_env_proxy_t &operator=(const axis_env_proxy_t &other) = delete;
  axis_env_proxy_t &operator=(axis_env_proxy_t &&other) = delete;
  // @}

  bool acquire_lock_mode(error_t *err = nullptr) {
    if (c_axis_env_proxy == nullptr) {
      axis_ASSERT(0, "Invalid argument.");
      return false;
    }

    return axis_env_proxy_acquire_lock_mode(
        c_axis_env_proxy, err != nullptr ? err->get_c_error() : nullptr);
  }

  bool release_lock_mode(error_t *err = nullptr) {
    if (c_axis_env_proxy == nullptr) {
      axis_ASSERT(0, "Invalid argument.");
      return false;
    }

    return axis_env_proxy_release_lock_mode(
        c_axis_env_proxy, err != nullptr ? err->get_c_error() : nullptr);
  }

  bool notify(notify_std_func_t &&notify_func, bool sync = false,
              error_t *err = nullptr) {
    auto *info = new proxy_notify_info_t(std::move(notify_func));

    auto rc =
        axis_env_proxy_notify(c_axis_env_proxy, proxy_notify, info, sync,
                             err != nullptr ? err->get_c_error() : nullptr);
    if (!rc) {
      delete info;
    }

    return rc;
  }

  bool notify(notify_std_with_user_data_func_t &&notify_func, void *user_data,
              bool sync = false, error_t *err = nullptr) {
    auto *info = new proxy_notify_info_t(std::move(notify_func), user_data);

    auto rc =
        axis_env_proxy_notify(c_axis_env_proxy, proxy_notify, info, sync,
                             err != nullptr ? err->get_c_error() : nullptr);
    if (!rc) {
      delete info;
    }

    return rc;
  }

 private:
  ::axis_env_proxy_t *c_axis_env_proxy;

  // We do not provide the following API. If having similar needs, it can be
  // achieved by creating a new axis_env_proxy.
  //
  // bool acquire(error_t *err = nullptr) {
  //   if (c_axis_env_proxy == nullptr) {
  //     axis_ASSERT(0, "Invalid argument.");
  //     return false;
  //   }
  //   return axis_proxy_acquire(
  //       c_axis_env_proxy,
  //       err != nullptr ? err->get_c_error() : nullptr);
  // }
  //
  // bool release(error_t *err = nullptr) {
  //   if (c_axis_env_proxy == nullptr) {
  //     axis_ASSERT(0, "Invalid argument.");
  //     return false;
  //   }
  //   bool rc = axis_proxy_release(
  //       c_axis_env_proxy,
  //       err != nullptr ? err->get_c_error() : nullptr);
  //   axis_ASSERT(rc, "Should not happen.");
  //   return rc;
  // }
};

}  // namespace ten
