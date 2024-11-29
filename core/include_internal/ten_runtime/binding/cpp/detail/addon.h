//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

class extension_group_addon_t : public addon_t {
 private:
  void on_create_instance_impl(ten_env_t &ten_env, const char *name,
                               void *context) override {
    auto *cpp_context = new addon_context_t();
    cpp_context->task = ADDON_TASK_CREATE_EXTENSION_GROUP;
    cpp_context->c_context = context;

    on_create_instance(ten_env, name, cpp_context);
  }
};

#define TEN_CPP_REGISTER_ADDON_AS_EXTENSION_GROUP(NAME, CLASS)                                 \
  class NAME##_default_extension_group_addon_t                                                 \
      : public ten::extension_group_addon_t {                                                  \
   public:                                                                                     \
    void on_create_instance(ten::ten_env_t &ten_env, const char *name,                         \
                            void *context) override {                                          \
      auto *instance = new CLASS(name);                                                        \
      ten_env.on_create_instance_done(instance, context);                                      \
    }                                                                                          \
    void on_destroy_instance(ten::ten_env_t &ten_env, void *instance,                          \
                             void *context) override {                                         \
      delete static_cast<CLASS *>(instance);                                                   \
      ten_env.on_destroy_instance_done(context);                                               \
    }                                                                                          \
  };                                                                                           \
  static ten::addon_t *g_##NAME##_default_extension_group_addon = nullptr;                     \
  TEN_CONSTRUCTOR(____ctor_ten_declare_##NAME##_extension_group_addon____) {                   \
    g_##NAME##_default_extension_group_addon =                                                 \
        new NAME##_default_extension_group_addon_t();                                          \
    ten_string_t *base_dir =                                                                   \
        ten_path_get_module_path(/* NOLINTNEXTLINE */                                          \
                                 (void *)                                                      \
                                     ____ctor_ten_declare_##NAME##_extension_group_addon____); \
    ten_addon_register_extension_group(                                                        \
        #NAME, ten_string_get_raw_str(base_dir),                                               \
        g_##NAME##_default_extension_group_addon->get_c_addon());                              \
    ten_string_destroy(base_dir);                                                              \
  }                                                                                            \
  TEN_DESTRUCTOR(____dtor_ten_declare_##NAME##_extension_group_addon____) {                    \
    ten_addon_unregister_extension_group(#NAME);                                               \
    delete g_##NAME##_default_extension_group_addon;                                           \
  }
