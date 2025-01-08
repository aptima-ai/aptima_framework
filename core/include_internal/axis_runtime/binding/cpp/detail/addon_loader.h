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

#include "include_internal/axis_runtime/addon_loader/addon_loader.h"
#include "axis_runtime/binding/common.h"
#include "axis_runtime/binding/cpp/detail/binding_handle.h"
#include "axis_utils/macro/check.h"

using axis_addon_loader_t = struct axis_addon_loader_t;

namespace ten {

class addon_loader_t : public binding_handle_t {
 public:
  ~addon_loader_t() override {
    axis_ASSERT(get_c_instance(), "Should not happen.");
    axis_addon_loader_destroy(
        static_cast<axis_addon_loader_t *>(get_c_instance()));
  }

  // @{
  addon_loader_t(const addon_loader_t &) = delete;
  addon_loader_t(addon_loader_t &&) = delete;
  addon_loader_t &operator=(const addon_loader_t &) = delete;
  addon_loader_t &operator=(const addon_loader_t &&) = delete;
  // @}

 protected:
  explicit addon_loader_t()
      : binding_handle_t(::axis_addon_loader_create(
            reinterpret_cast<axis_addon_loader_on_init_func_t>(&proxy_on_init),
            reinterpret_cast<axis_addon_loader_on_deinit_func_t>(
                &proxy_on_deinit),
            reinterpret_cast<axis_addon_loader_on_load_addon_func_t>(
                &proxy_on_load_addon))) {
    axis_ASSERT(get_c_instance(), "Should not happen.");

    axis_binding_handle_set_me_in_target_lang(
        reinterpret_cast<axis_binding_handle_t *>(get_c_instance()),
        static_cast<void *>(this));
  }

  virtual void on_init() = 0;

  virtual void on_deinit() = 0;

  // **Note:** This function, used to dynamically load other addons, may be
  // called from multiple threads. Therefore, it must be thread-safe.
  virtual void on_load_addon(axis_ADDON_TYPE addon_type,
                             const char *addon_name) = 0;

 private:
  static void proxy_on_init(axis_addon_loader_t *addon_loader) {
    axis_ASSERT(addon_loader, "Should not happen.");

    auto *cpp_addon_loader =
        static_cast<addon_loader_t *>(axis_binding_handle_get_me_in_target_lang(
            reinterpret_cast<axis_binding_handle_t *>(addon_loader)));

    cpp_addon_loader->invoke_cpp_addon_loader_on_init();
  }

  static void proxy_on_deinit(axis_addon_loader_t *addon_loader) {
    axis_ASSERT(addon_loader, "Should not happen.");

    auto *cpp_addon_loader =
        static_cast<addon_loader_t *>(axis_binding_handle_get_me_in_target_lang(
            reinterpret_cast<axis_binding_handle_t *>(addon_loader)));

    cpp_addon_loader->invoke_cpp_addon_loader_on_deinit();
  }

  static void proxy_on_load_addon(axis_addon_loader_t *addon_loader,
                                  axis_ADDON_TYPE addon_type,
                                  const char *addon_name) {
    axis_ASSERT(addon_loader, "Should not happen.");

    auto *cpp_addon_loader =
        static_cast<addon_loader_t *>(axis_binding_handle_get_me_in_target_lang(
            reinterpret_cast<axis_binding_handle_t *>(addon_loader)));

    cpp_addon_loader->invoke_cpp_addon_loader_on_load_addon(addon_type,
                                                            addon_name);
  }

  void invoke_cpp_addon_loader_on_init() {
    try {
      on_init();
    } catch (...) {
      axis_ASSERT(0, "Should not happen.");
      // NOLINTNEXTLINE(concurrency-mt-unsafe)
      exit(EXIT_FAILURE);
    }
  }

  void invoke_cpp_addon_loader_on_deinit() {
    try {
      on_deinit();
    } catch (...) {
      axis_ASSERT(0, "Should not happen.");
      // NOLINTNEXTLINE(concurrency-mt-unsafe)
      exit(EXIT_FAILURE);
    }
  }

  void invoke_cpp_addon_loader_on_load_addon(axis_ADDON_TYPE addon_type,
                                             const char *addon_name) {
    try {
      on_load_addon(addon_type, addon_name);
    } catch (...) {
      axis_ASSERT(0, "Should not happen.");
      // NOLINTNEXTLINE(concurrency-mt-unsafe)
      exit(EXIT_FAILURE);
    }
  }
};

}  // namespace ten
