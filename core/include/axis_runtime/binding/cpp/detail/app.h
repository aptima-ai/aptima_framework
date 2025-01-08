//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <cstddef>
#include <string>

#include "axis_runtime/app/app.h"
#include "axis_runtime/binding/common.h"
#include "axis_runtime/binding/cpp/detail/common.h"
#include "axis_runtime/binding/cpp/detail/axis_env.h"
#include "axis_runtime/ten.h"
#include "axis_utils/macro/check.h"

using axis_app_t = struct axis_app_t;

namespace ten {

class app_t {
 public:
  app_t()
      : c_app(axis_app_create(cpp_app_on_configure_cb_wrapper,
                             cpp_app_on_init_cb_wrapper, nullptr, nullptr)),
        cpp_axis_env(new axis_env_t(axis_app_get_axis_env(c_app))) {
    axis_ASSERT(cpp_axis_env, "Should not happen.");
    axis_binding_handle_set_me_in_target_lang(
        reinterpret_cast<axis_binding_handle_t *>(c_app),
        static_cast<void *>(this));
  }

  virtual ~app_t() {
    axis_app_destroy(c_app);
    c_app = nullptr;

    axis_ASSERT(cpp_axis_env, "Should not happen.");
    delete cpp_axis_env;
  }

  // @{
  app_t(const app_t &) = delete;
  app_t(app_t &&) = delete;
  app_t &operator=(const app_t &) = delete;
  app_t &operator=(app_t &&) = delete;
  // @}

  bool run(bool run_in_background = false, error_t *err = nullptr) {
    if (c_app == nullptr) {
      return false;
    }

    return axis_app_run(c_app, run_in_background,
                       err != nullptr ? err->get_c_error() : nullptr);
  }

  bool close(error_t *err = nullptr) {
    return axis_app_close(c_app, err != nullptr ? err->get_c_error() : nullptr);
  }

  bool wait(error_t *err = nullptr) {
    return axis_app_wait(c_app, err != nullptr ? err->get_c_error() : nullptr);
  }

 protected:
  virtual void on_configure(axis_env_t &axis_env) { axis_env.on_configure_done(); }

  virtual void on_init(axis_env_t &axis_env) { axis_env.on_init_done(); }

  virtual void on_deinit(axis_env_t &axis_env) { axis_env.on_deinit_done(); }

 private:
  static void cpp_app_on_configure_cb_wrapper(axis_app_t *app,
                                              ::axis_env_t *axis_env) {
    axis_ASSERT(app && axis_app_check_integrity(app, true), "Should not happen.");
    axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
               "Should not happen.");
    axis_ASSERT(axis_app_get_axis_env(app) == axis_env, "Should not happen.");

    auto *cpp_app =
        static_cast<app_t *>(axis_binding_handle_get_me_in_target_lang(
            reinterpret_cast<axis_binding_handle_t *>(app)));
    auto *cpp_axis_env =
        static_cast<axis_env_t *>(axis_binding_handle_get_me_in_target_lang(
            reinterpret_cast<axis_binding_handle_t *>(axis_env)));

    cpp_app->on_configure_helper_for_cpp(*cpp_axis_env);
  }

  void on_configure_helper_for_cpp(axis_env_t &axis_env) {
    // The TEN runtime does not use C++ exceptions. The use of try/catch here is
    // merely to intercept any exceptions that might be thrown by the user's app
    // code. If exceptions are disabled during the compilation of the TEN
    // runtime (i.e., with -fno-exceptions), it implies that the extensions used
    // will also not employ exceptions (otherwise it would be unreasonable). In
    // this case, the try/catch blocks become no-ops. Conversely, if exceptions
    // are enabled during compilation, then the try/catch here can intercept all
    // exceptions thrown by user code that are not already caught, serving as a
    // kind of fallback.
    try {
      on_configure(axis_env);
    } catch (...) {
      axis_LOGW("Caught a exception of type '%s' in App on_configure().",
               curr_exception_type_name().c_str());
    }
  }

  static void cpp_app_on_init_cb_wrapper(axis_app_t *app, ::axis_env_t *axis_env) {
    axis_ASSERT(app && axis_app_check_integrity(app, true), "Should not happen.");
    axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
               "Should not happen.");
    axis_ASSERT(axis_app_get_axis_env(app) == axis_env, "Should not happen.");

    auto *cpp_app =
        static_cast<app_t *>(axis_binding_handle_get_me_in_target_lang(
            reinterpret_cast<axis_binding_handle_t *>(app)));
    auto *cpp_axis_env =
        static_cast<axis_env_t *>(axis_binding_handle_get_me_in_target_lang(
            reinterpret_cast<axis_binding_handle_t *>(axis_env)));

    cpp_app->on_init_helper_for_cpp(*cpp_axis_env);
  }

  void on_init_helper_for_cpp(axis_env_t &axis_env) {
    // The TEN runtime does not use C++ exceptions. The use of try/catch here is
    // merely to intercept any exceptions that might be thrown by the user's app
    // code. If exceptions are disabled during the compilation of the TEN
    // runtime (i.e., with -fno-exceptions), it implies that the extensions used
    // will also not employ exceptions (otherwise it would be unreasonable). In
    // this case, the try/catch blocks become no-ops. Conversely, if exceptions
    // are enabled during compilation, then the try/catch here can intercept all
    // exceptions thrown by user code that are not already caught, serving as a
    // kind of fallback.
    try {
      on_init(axis_env);
    } catch (...) {
      axis_LOGW("Caught a exception of type '%s' in App on_init().",
               curr_exception_type_name().c_str());
    }
  }

  static void cpp_app_on_close_cb_wrapper(axis_app_t *app,
                                          ::axis_env_t *axis_env) {
    axis_ASSERT(app && axis_app_check_integrity(app, true), "Should not happen.");
    axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
               "Should not happen.");
    axis_ASSERT(axis_app_get_axis_env(app) == axis_env, "Should not happen.");

    auto *cpp_app =
        static_cast<app_t *>(axis_binding_handle_get_me_in_target_lang(
            reinterpret_cast<axis_binding_handle_t *>(app)));
    auto *cpp_axis_env =
        static_cast<axis_env_t *>(axis_binding_handle_get_me_in_target_lang(
            reinterpret_cast<axis_binding_handle_t *>(axis_env)));

    cpp_app->on_deinit_helper_for_cpp(*cpp_axis_env);
  }

  void on_deinit_helper_for_cpp(axis_env_t &axis_env) {
    // The TEN runtime does not use C++ exceptions. The use of try/catch here is
    // merely to intercept any exceptions that might be thrown by the user's app
    // code. If exceptions are disabled during the compilation of the TEN
    // runtime (i.e., with -fno-exceptions), it implies that the extensions used
    // will also not employ exceptions (otherwise it would be unreasonable). In
    // this case, the try/catch blocks become no-ops. Conversely, if exceptions
    // are enabled during compilation, then the try/catch here can intercept all
    // exceptions thrown by user code that are not already caught, serving as a
    // kind of fallback.
    try {
      on_deinit(axis_env);
    } catch (...) {
      axis_LOGW("Caught a exception of type '%s' in App on_close().",
               curr_exception_type_name().c_str());
    }
  }

  ::axis_app_t *c_app = nullptr;
  axis_env_t *cpp_axis_env;
};

}  // namespace ten
