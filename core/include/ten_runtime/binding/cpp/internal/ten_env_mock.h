//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/addon/extension/extension.h"
#include "ten_runtime/binding/common.h"
#include "ten_runtime/binding/cpp/internal/extension.h"
#include "ten_runtime/binding/cpp/internal/ten_env.h"
#include "ten_runtime/ten_env/ten_env.h"

namespace ten {

class ten_env_mock_t : public ten_env_t {
 public:
  ten_env_mock_t() : ten_env_t(ten_env_mock_create()) {}
  ~ten_env_mock_t() override { ten_env_destroy(c_ten_env); }

  using ten_env_t::addon_create_extension_async;
  bool addon_create_extension_async(const char *addon_name,
                                    const char *instance_name,
                                    addon_create_extension_async_cb_t &&cb,
                                    error_t *err) override {
    auto *cb_ptr = new addon_create_extension_async_cb_t(std::move(cb));

    return ten_addon_create_extension_async_for_mock(
        c_ten_env, addon_name, instance_name,
        proxy_addon_create_extension_async_cb, cb_ptr,
        err != nullptr ? err->get_internal_representation() : nullptr);
  }

  using ten_env_t::addon_destroy_extension_async;
  bool addon_destroy_extension_async(ten::extension_t *extension,
                                     addon_destroy_extension_async_cb_t &&cb,
                                     error_t *err) override {
    auto *cb_ptr = new addon_destroy_extension_async_cb_t(std::move(cb));

    return ten_addon_destroy_extension_async_for_mock(
        c_ten_env, extension->get_c_extension(),
        proxy_addon_destroy_extension_async_cb, cb_ptr,
        err != nullptr ? err->get_internal_representation() : nullptr);
  }

  // @{
  ten_env_mock_t(const ten_env_mock_t &) = delete;
  ten_env_mock_t(ten_env_mock_t &&) = delete;
  ten_env_mock_t &operator=(const ten_env_mock_t &) = delete;
  ten_env_mock_t &operator=(const ten_env_mock_t &&) = delete;
  // @}

 private:
  static void proxy_addon_create_extension_async_cb(::ten_env_t *ten_env,
                                                    void *instance,
                                                    void *cb_data) {
    auto *addon_create_extension_async_cb =
        static_cast<addon_create_extension_async_cb_t *>(cb_data);
    auto *cpp_ten_env =
        static_cast<ten_env_t *>(ten_binding_handle_get_me_in_target_lang(
            reinterpret_cast<ten_binding_handle_t *>(ten_env)));

    (*addon_create_extension_async_cb)(
        *cpp_ten_env,
        *static_cast<ten::extension_t *>(
            ten_binding_handle_get_me_in_target_lang(
                reinterpret_cast<ten_binding_handle_t *>(instance))));
    delete addon_create_extension_async_cb;
  }

  static void proxy_addon_destroy_extension_async_cb(::ten_env_t *ten_env,
                                                     void *cb_data) {
    auto *addon_destroy_extension_async_cb =
        static_cast<addon_destroy_extension_async_cb_t *>(cb_data);
    auto *cpp_ten_env =
        static_cast<ten_env_t *>(ten_binding_handle_get_me_in_target_lang(
            reinterpret_cast<ten_binding_handle_t *>(ten_env)));

    (*addon_destroy_extension_async_cb)(*cpp_ten_env);
    delete addon_destroy_extension_async_cb;
  }
};

}  // namespace ten
