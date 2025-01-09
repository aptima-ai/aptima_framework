//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <cstddef>
#include <string>

#include "aptima_runtime/app/app.h"
#include "aptima_runtime/binding/common.h"
#include "aptima_runtime/binding/cpp/detail/common.h"
#include "aptima_runtime/binding/cpp/detail/aptima_env.h"
#include "aptima_runtime/ten.h"
#include "aptima_utils/macro/check.h"

using aptima_app_t = struct aptima_app_t;

namespace ten {

class app_t {
 public:
  app_t()
      : c_app(aptima_app_create(cpp_app_on_configure_cb_wrapper,
                             cpp_app_on_init_cb_wrapper, nullptr, nullptr)),
        cpp_aptima_env(new aptima_env_t(aptima_app_get_aptima_env(c_app))) {
    aptima_ASSERT(cpp_aptima_env, "Should not happen.");
    aptima_binding_handle_set_me_in_target_lang(
        reinterpret_cast<aptima_binding_handle_t *>(c_app),
        static_cast<void *>(this));
  }

  virtual ~app_t() {
    aptima_app_destroy(c_app);
    c_app = nullptr;

    aptima_ASSERT(cpp_aptima_env, "Should not happen.");
    delete cpp_aptima_env;
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

    return aptima_app_run(c_app, run_in_background,
                       err != nullptr ? err->get_c_error() : nullptr);
  }

  bool close(error_t *err = nullptr) {
    return aptima_app_close(c_app, err != nullptr ? err->get_c_error() : nullptr);
  }

  bool wait(error_t *err = nullptr) {
    return aptima_app_wait(c_app, err != nullptr ? err->get_c_error() : nullptr);
  }

 protected:
  virtual void on_configure(aptima_env_t &aptima_env) { aptima_env.on_configure_done(); }

  virtual void on_init(aptima_env_t &aptima_env) { aptima_env.on_init_done(); }

  virtual void on_deinit(aptima_env_t &aptima_env) { aptima_env.on_deinit_done(); }

 private:
  static void cpp_app_on_configure_cb_wrapper(aptima_app_t *app,
                                              ::aptima_env_t *aptima_env) {
    aptima_ASSERT(app && aptima_app_check_integrity(app, true), "Should not happen.");
    aptima_ASSERT(aptima_env && aptima_env_check_integrity(aptima_env, true),
               "Should not happen.");
    aptima_ASSERT(aptima_app_get_aptima_env(app) == aptima_env, "Should not happen.");

    auto *cpp_app =
        static_cast<app_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(app)));
    auto *cpp_aptima_env =
        static_cast<aptima_env_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(aptima_env)));

    cpp_app->on_configure_helper_for_cpp(*cpp_aptima_env);
  }

  void on_configure_helper_for_cpp(aptima_env_t &aptima_env) {
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
      on_configure(aptima_env);
    } catch (...) {
      aptima_LOGW("Caught a exception of type '%s' in App on_configure().",
               curr_exception_type_name().c_str());
    }
  }

  static void cpp_app_on_init_cb_wrapper(aptima_app_t *app, ::aptima_env_t *aptima_env) {
    aptima_ASSERT(app && aptima_app_check_integrity(app, true), "Should not happen.");
    aptima_ASSERT(aptima_env && aptima_env_check_integrity(aptima_env, true),
               "Should not happen.");
    aptima_ASSERT(aptima_app_get_aptima_env(app) == aptima_env, "Should not happen.");

    auto *cpp_app =
        static_cast<app_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(app)));
    auto *cpp_aptima_env =
        static_cast<aptima_env_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(aptima_env)));

    cpp_app->on_init_helper_for_cpp(*cpp_aptima_env);
  }

  void on_init_helper_for_cpp(aptima_env_t &aptima_env) {
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
      on_init(aptima_env);
    } catch (...) {
      aptima_LOGW("Caught a exception of type '%s' in App on_init().",
               curr_exception_type_name().c_str());
    }
  }

  static void cpp_app_on_close_cb_wrapper(aptima_app_t *app,
                                          ::aptima_env_t *aptima_env) {
    aptima_ASSERT(app && aptima_app_check_integrity(app, true), "Should not happen.");
    aptima_ASSERT(aptima_env && aptima_env_check_integrity(aptima_env, true),
               "Should not happen.");
    aptima_ASSERT(aptima_app_get_aptima_env(app) == aptima_env, "Should not happen.");

    auto *cpp_app =
        static_cast<app_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(app)));
    auto *cpp_aptima_env =
        static_cast<aptima_env_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(aptima_env)));

    cpp_app->on_deinit_helper_for_cpp(*cpp_aptima_env);
  }

  void on_deinit_helper_for_cpp(aptima_env_t &aptima_env) {
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
      on_deinit(aptima_env);
    } catch (...) {
      aptima_LOGW("Caught a exception of type '%s' in App on_close().",
               curr_exception_type_name().c_str());
    }
  }

  ::aptima_app_t *c_app = nullptr;
  aptima_env_t *cpp_aptima_env;
};

}  // namespace ten
