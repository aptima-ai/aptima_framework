//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <cassert>
#include <cstddef>

#include "axis_runtime/addon/addon.h"
#include "axis_runtime/addon/extension/extension.h"
#include "axis_runtime/binding/common.h"
#include "axis_runtime/binding/cpp/detail/binding_handle.h"
#include "axis_runtime/binding/cpp/detail/common.h"
#include "axis_runtime/binding/cpp/detail/axis_env.h"
#include "axis_runtime/axis_env/axis_env.h"

namespace ten {

class addon_t : public binding_handle_t {
 public:
  addon_t()
      : binding_handle_t(axis_addon_create(
            proxy_on_init, proxy_on_deinit, proxy_on_create_instance,
            proxy_on_destroy_instance, proxy_on_destroy)) {
    axis_binding_handle_set_me_in_target_lang(
        reinterpret_cast<axis_binding_handle_t *>(get_c_instance()), this);
  }

  ~addon_t() override {
    axis_addon_destroy(static_cast<axis_addon_t *>(get_c_instance()));

    axis_ASSERT(cpp_axis_env, "Should not happen.");
    delete cpp_axis_env;
  };

  // @{
  addon_t(const addon_t &) = delete;
  addon_t(addon_t &&) = delete;
  addon_t &operator=(const addon_t &) = delete;
  addon_t &operator=(const addon_t &&) = delete;
  // @}

 protected:
  virtual void on_init(axis_env_t &axis_env) { axis_env.on_init_done(); }

  virtual void on_deinit(axis_env_t &axis_env) { axis_env.on_deinit_done(); }

  virtual void on_create_instance(axis_env_t &axis_env, const char *name,
                                  void *context) {
    (void)axis_env;
    (void)name;
    (void)context;

    // If a subclass requires the functionality of this function, it needs to
    // override this function.
    axis_ASSERT(0, "Should not happen.");
  };

  virtual void on_destroy_instance(axis_env_t &axis_env, void *instance,
                                   void *context) {
    (void)axis_env;
    (void)instance;
    (void)context;

    // If a subclass requires the functionality of this function, it needs to
    // override this function.
    axis_ASSERT(0, "Should not happen.");
  };

 private:
  axis_env_t *cpp_axis_env{};

  void invoke_cpp_addon_on_init(axis_env_t &axis_env) {
    try {
      on_init(axis_env);
    } catch (...) {
      axis_LOGW("Caught a exception of type '%s' in addon on_init().",
               curr_exception_type_name().c_str());
    }
  }

  void invoke_cpp_addon_on_deinit(axis_env_t &axis_env) {
    try {
      on_deinit(axis_env);
    } catch (...) {
      axis_LOGW("Caught a exception '%s' in addon on_deinit().",
               curr_exception_type_name().c_str());
    }
  }

  void invoke_cpp_addon_on_create_instance(axis_env_t &axis_env, const char *name,
                                           void *context) {
    try {
      on_create_instance(axis_env, name, context);
    } catch (...) {
      axis_LOGW("Caught a exception '%s' in addon on_create_instance(%s).",
               curr_exception_type_name().c_str(), name);
    }
  }

  void invoke_cpp_addon_on_destroy_instance(axis_env_t &axis_env, void *instance,
                                            void *context) {
    try {
      on_destroy_instance(axis_env, instance, context);
    } catch (...) {
      axis_LOGW("Caught a exception '%s' in addon on_destroy_instance().",
               curr_exception_type_name().c_str());
    }
  }

  static void proxy_on_init(axis_addon_t *addon, ::axis_env_t *axis_env) {
    axis_ASSERT(addon && axis_env, "Invalid argument.");

    auto *cpp_addon =
        static_cast<addon_t *>(axis_binding_handle_get_me_in_target_lang(
            reinterpret_cast<axis_binding_handle_t *>(addon)));
    axis_ASSERT(!axis_binding_handle_get_me_in_target_lang(
                   reinterpret_cast<axis_binding_handle_t *>(axis_env)),
               "Should not happen.");

    auto *cpp_axis_env = new axis_env_t(axis_env);
    axis_ASSERT(cpp_addon && cpp_axis_env, "Should not happen.");

    // Remember it so that we can destroy it when C++ addon is destroyed.
    cpp_addon->cpp_axis_env = cpp_axis_env;

    cpp_addon->invoke_cpp_addon_on_init(*cpp_axis_env);
  }

  static void proxy_on_deinit(axis_addon_t *addon, ::axis_env_t *axis_env) {
    axis_ASSERT(addon && axis_env, "Should not happen.");

    auto *cpp_addon =
        static_cast<addon_t *>(axis_binding_handle_get_me_in_target_lang(
            reinterpret_cast<axis_binding_handle_t *>(addon)));
    auto *cpp_axis_env =
        static_cast<axis_env_t *>(axis_binding_handle_get_me_in_target_lang(
            reinterpret_cast<axis_binding_handle_t *>(axis_env)));
    axis_ASSERT(cpp_addon && cpp_axis_env, "Should not happen.");

    cpp_addon->invoke_cpp_addon_on_deinit(*cpp_axis_env);
  }

  static void proxy_on_create_instance(axis_addon_t *addon, ::axis_env_t *axis_env,
                                       const char *name, void *context) {
    axis_ASSERT(addon && axis_env && name && strlen(name), "Invalid argument.");

    auto *cpp_addon =
        static_cast<addon_t *>(axis_binding_handle_get_me_in_target_lang(
            reinterpret_cast<axis_binding_handle_t *>(addon)));
    axis_ASSERT(cpp_addon, "Should not happen.");

    auto *cpp_axis_env =
        static_cast<axis_env_t *>(axis_binding_handle_get_me_in_target_lang(
            reinterpret_cast<axis_binding_handle_t *>(axis_env)));
    axis_ASSERT(cpp_axis_env, "Should not happen.");

    cpp_addon->invoke_cpp_addon_on_create_instance(*cpp_axis_env, name, context);
  }

  static void proxy_on_destroy_instance(axis_addon_t *addon,
                                        ::axis_env_t *axis_env, void *instance,
                                        void *context) {
    axis_ASSERT(addon && axis_env && instance, "Invalid argument.");

    auto *cpp_addon =
        static_cast<addon_t *>(axis_binding_handle_get_me_in_target_lang(
            reinterpret_cast<axis_binding_handle_t *>(addon)));

    auto *cpp_axis_env =
        static_cast<axis_env_t *>(axis_binding_handle_get_me_in_target_lang(
            reinterpret_cast<axis_binding_handle_t *>(axis_env)));
    axis_ASSERT(cpp_axis_env, "Should not happen.");

    auto *cpp_instance = axis_binding_handle_get_me_in_target_lang(
        static_cast<axis_binding_handle_t *>(instance));
    axis_ASSERT(cpp_instance, "Should not happen.");

    cpp_addon->invoke_cpp_addon_on_destroy_instance(*cpp_axis_env, cpp_instance,
                                                    context);
  }

  static void proxy_on_destroy(axis_addon_t *addon) {
    axis_ASSERT(addon, "Invalid argument.");

    auto *cpp_addon =
        static_cast<addon_t *>(axis_binding_handle_get_me_in_target_lang(
            reinterpret_cast<axis_binding_handle_t *>(addon)));

    delete cpp_addon;
  }
};

}  // namespace ten
