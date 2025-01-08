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

#include "include_internal/axis_runtime/addon/addon_loader/addon_loader.h"  // IWYU pragma: keep
#include "axis_runtime/addon/addon_manager.h"       // IWYU pragma: keep
#include "axis_runtime/binding/cpp/detail/addon.h"  // IWYU pragma: keep

#define axis_CPP_REGISTER_ADDON_AS_ADDON_LOADER(NAME, CLASS)                      \
  class NAME##_default_addon_loader_addon_t : public ten::addon_t {              \
   public:                                                                       \
    void on_create_instance(ten::axis_env_t &axis_env, const char *name,           \
                            void *context) override {                            \
      auto *instance = new CLASS(name);                                          \
      axis_env.on_create_instance_done(instance, context);                        \
    }                                                                            \
    void on_destroy_instance(ten::axis_env_t &axis_env, void *instance,            \
                             void *context) override {                           \
      delete static_cast<CLASS *>(instance);                                     \
      axis_env.on_destroy_instance_done(context);                                 \
    }                                                                            \
  };                                                                             \
  static void ____axis_addon_##NAME##_register_handler__(void *register_ctx) {    \
    auto *addon_instance = new NAME##_default_addon_loader_addon_t();            \
    axis_string_t *base_dir =                                                     \
        axis_path_get_module_path(/* NOLINTNEXTLINE */                            \
                                 (void *)                                        \
                                     ____axis_addon_##NAME##_register_handler__); \
    axis_addon_register_addon_loader(                                             \
        #NAME, axis_string_get_raw_str(base_dir),                                 \
        static_cast<axis_addon_t *>(addon_instance->get_c_instance()),            \
        register_ctx);                                                           \
    axis_string_destroy(base_dir);                                                \
  }                                                                              \
  axis_CONSTRUCTOR(____axis_addon_##NAME##_registrar____) {                        \
    /* Add addon registration function into addon manager. */                    \
    axis_addon_manager_t *manager = axis_addon_manager_get_instance();             \
    bool success = axis_addon_manager_add_addon(                                  \
        manager, "addon_loader", #NAME,                                          \
        ____axis_addon_##NAME##_register_handler__);                              \
    if (!success) {                                                              \
      axis_LOGF("Failed to register addon: %s", #NAME);                           \
      /* NOLINTNEXTLINE(concurrency-mt-unsafe) */                                \
      exit(EXIT_FAILURE);                                                        \
    }                                                                            \
  }
