//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <cassert>
#include <cstddef>

#include "aptima_runtime/addon/addon.h"
#include "aptima_runtime/addon/extension/extension.h"
#include "aptima_runtime/binding/common.h"
#include "aptima_runtime/binding/cpp/detail/binding_handle.h"
#include "aptima_runtime/binding/cpp/detail/common.h"
#include "aptima_runtime/binding/cpp/detail/aptima_env.h"
#include "aptima_runtime/aptima_env/aptima_env.h"

namespace aptima {

class addon_t : public binding_handle_t {
 public:
  addon_t()
      : binding_handle_t(aptima_addon_create(
            proxy_on_init, proxy_on_deinit, proxy_on_create_instance,
            proxy_on_destroy_instance, proxy_on_destroy)) {
    aptima_binding_handle_set_me_in_target_lang(
        reinterpret_cast<aptima_binding_handle_t *>(get_c_instance()), this);
  }

  ~addon_t() override {
    aptima_addon_destroy(static_cast<aptima_addon_t *>(get_c_instance()));

    aptima_ASSERT(cpp_aptima_env, "Should not happen.");
    delete cpp_aptima_env;
  };

  // @{
  addon_t(const addon_t &) = delete;
  addon_t(addon_t &&) = delete;
  addon_t &operator=(const addon_t &) = delete;
  addon_t &operator=(const addon_t &&) = delete;
  // @}

 protected:
  virtual void on_init(aptima_env_t &aptima_env) { aptima_env.on_init_done(); }

  virtual void on_deinit(aptima_env_t &aptima_env) { aptima_env.on_deinit_done(); }

  virtual void on_create_instance(aptima_env_t &aptima_env, const char *name,
                                  void *context) {
    (void)aptima_env;
    (void)name;
    (void)context;

    // If a subclass requires the functionality of this function, it needs to
    // override this function.
    aptima_ASSERT(0, "Should not happen.");
  };

  virtual void on_destroy_instance(aptima_env_t &aptima_env, void *instance,
                                   void *context) {
    (void)aptima_env;
    (void)instance;
    (void)context;

    // If a subclass requires the functionality of this function, it needs to
    // override this function.
    aptima_ASSERT(0, "Should not happen.");
  };

 private:
  aptima_env_t *cpp_aptima_env{};

  void invoke_cpp_addon_on_init(aptima_env_t &aptima_env) {
    try {
      on_init(aptima_env);
    } catch (...) {
      aptima_LOGW("Caught a exception of type '%s' in addon on_init().",
               curr_exception_type_name().c_str());
    }
  }

  void invoke_cpp_addon_on_deinit(aptima_env_t &aptima_env) {
    try {
      on_deinit(aptima_env);
    } catch (...) {
      aptima_LOGW("Caught a exception '%s' in addon on_deinit().",
               curr_exception_type_name().c_str());
    }
  }

  void invoke_cpp_addon_on_create_instance(aptima_env_t &aptima_env, const char *name,
                                           void *context) {
    try {
      on_create_instance(aptima_env, name, context);
    } catch (...) {
      aptima_LOGW("Caught a exception '%s' in addon on_create_instance(%s).",
               curr_exception_type_name().c_str(), name);
    }
  }

  void invoke_cpp_addon_on_destroy_instance(aptima_env_t &aptima_env, void *instance,
                                            void *context) {
    try {
      on_destroy_instance(aptima_env, instance, context);
    } catch (...) {
      aptima_LOGW("Caught a exception '%s' in addon on_destroy_instance().",
               curr_exception_type_name().c_str());
    }
  }

  static void proxy_on_init(aptima_addon_t *addon, ::aptima_env_t *aptima_env) {
    aptima_ASSERT(addon && aptima_env, "Invalid argument.");

    auto *cpp_addon =
        static_cast<addon_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(addon)));
    aptima_ASSERT(!aptima_binding_handle_get_me_in_target_lang(
                   reinterpret_cast<aptima_binding_handle_t *>(aptima_env)),
               "Should not happen.");

    auto *cpp_aptima_env = new aptima_env_t(aptima_env);
    aptima_ASSERT(cpp_addon && cpp_aptima_env, "Should not happen.");

    // Remember it so that we can destroy it when C++ addon is destroyed.
    cpp_addon->cpp_aptima_env = cpp_aptima_env;

    cpp_addon->invoke_cpp_addon_on_init(*cpp_aptima_env);
  }

  static void proxy_on_deinit(aptima_addon_t *addon, ::aptima_env_t *aptima_env) {
    aptima_ASSERT(addon && aptima_env, "Should not happen.");

    auto *cpp_addon =
        static_cast<addon_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(addon)));
    auto *cpp_aptima_env =
        static_cast<aptima_env_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(aptima_env)));
    aptima_ASSERT(cpp_addon && cpp_aptima_env, "Should not happen.");

    cpp_addon->invoke_cpp_addon_on_deinit(*cpp_aptima_env);
  }

  static void proxy_on_create_instance(aptima_addon_t *addon, ::aptima_env_t *aptima_env,
                                       const char *name, void *context) {
    aptima_ASSERT(addon && aptima_env && name && strlen(name), "Invalid argument.");

    auto *cpp_addon =
        static_cast<addon_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(addon)));
    aptima_ASSERT(cpp_addon, "Should not happen.");

    auto *cpp_aptima_env =
        static_cast<aptima_env_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(aptima_env)));
    aptima_ASSERT(cpp_aptima_env, "Should not happen.");

    cpp_addon->invoke_cpp_addon_on_create_instance(*cpp_aptima_env, name, context);
  }

  static void proxy_on_destroy_instance(aptima_addon_t *addon,
                                        ::aptima_env_t *aptima_env, void *instance,
                                        void *context) {
    aptima_ASSERT(addon && aptima_env && instance, "Invalid argument.");

    auto *cpp_addon =
        static_cast<addon_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(addon)));

    auto *cpp_aptima_env =
        static_cast<aptima_env_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(aptima_env)));
    aptima_ASSERT(cpp_aptima_env, "Should not happen.");

    auto *cpp_instance = aptima_binding_handle_get_me_in_target_lang(
        static_cast<aptima_binding_handle_t *>(instance));
    aptima_ASSERT(cpp_instance, "Should not happen.");

    cpp_addon->invoke_cpp_addon_on_destroy_instance(*cpp_aptima_env, cpp_instance,
                                                    context);
  }

  static void proxy_on_destroy(aptima_addon_t *addon) {
    aptima_ASSERT(addon, "Invalid argument.");

    auto *cpp_addon =
        static_cast<addon_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(addon)));

    delete cpp_addon;
  }
};

}  // namespace aptima
