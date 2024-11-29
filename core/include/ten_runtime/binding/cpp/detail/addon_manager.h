//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <cassert>
#include <cstddef>

#define TEN_CPP_REGISTER_ADDON_AS_EXTENSION(NAME, CLASS)                                 \
  class NAME##_default_extension_addon_t : public ten::extension_addon_t {               \
   public:                                                                               \
    void on_create_instance(ten::ten_env_t &ten_env, const char *name,                   \
                            void *context) override {                                    \
      auto *instance = new CLASS(name);                                                  \
      ten_env.on_create_instance_done(instance, context);                                \
    }                                                                                    \
    void on_destroy_instance(ten::ten_env_t &ten_env, void *instance,                    \
                             void *context) override {                                   \
      delete static_cast<CLASS *>(instance);                                             \
      ten_env.on_destroy_instance_done(context);                                         \
    }                                                                                    \
  };                                                                                     \
  static ten::addon_t *g_##NAME##_default_extension_addon = nullptr;                     \
  TEN_CONSTRUCTOR(____ctor_ten_declare_##NAME##_extension_addon____) {                   \
    g_##NAME##_default_extension_addon =                                                 \
        new NAME##_default_extension_addon_t();                                          \
    ten_string_t *base_dir =                                                             \
        ten_path_get_module_path(/* NOLINTNEXTLINE */                                    \
                                 (void *)                                                \
                                     ____ctor_ten_declare_##NAME##_extension_addon____); \
    ten_addon_register_extension(                                                        \
        #NAME, ten_string_get_raw_str(base_dir),                                         \
        g_##NAME##_default_extension_addon->get_c_addon());                              \
    ten_string_destroy(base_dir);                                                        \
  }                                                                                      \
  TEN_DESTRUCTOR(____dtor_ten_declare_##NAME##_extension_addon____) {                    \
    ten_addon_unregister_extension(#NAME);                                               \
    delete g_##NAME##_default_extension_addon;                                           \
  }

#define TEN_CPP_REGISTER_ADDON_AS_EXTENSION_V2(NAME, CLASS)                              \
  class NAME##_default_extension_addon_t : public ten::extension_addon_t {               \
   public:                                                                               \
    void on_create_instance(ten::ten_env_t &ten_env, const char *name,                   \
                            void *context) override {                                    \
      auto *instance = new CLASS(name);                                                  \
      ten_env.on_create_instance_done(instance, context);                                \
    }                                                                                    \
    void on_destroy_instance(ten::ten_env_t &ten_env, void *instance,                    \
                             void *context) override {                                   \
      delete static_cast<CLASS *>(instance);                                             \
      ten_env.on_destroy_instance_done(context);                                         \
    }                                                                                    \
  };                                                                                     \
  static ten::addon_t *g_##NAME##_default_extension_addon = nullptr;                     \
  TEN_CONSTRUCTOR(____ctor_ten_declare_##NAME##_extension_addon____) {                   \
    g_##NAME##_default_extension_addon =                                                 \
        new NAME##_default_extension_addon_t();                                          \
    ten_string_t *base_dir =                                                             \
        ten_path_get_module_path(/* NOLINTNEXTLINE */                                    \
                                 (void *)                                                \
                                     ____ctor_ten_declare_##NAME##_extension_addon____); \
    ten_addon_register_extension(                                                        \
        #NAME, ten_string_get_raw_str(base_dir),                                         \
        g_##NAME##_default_extension_addon->get_c_addon());                              \
    ten_string_destroy(base_dir);                                                        \
  }                                                                                      \
  TEN_DESTRUCTOR(____dtor_ten_declare_##NAME##_extension_addon____) {                    \
    ten_addon_unregister_extension(#NAME);                                               \
    delete g_##NAME##_default_extension_addon;                                           \
  }
