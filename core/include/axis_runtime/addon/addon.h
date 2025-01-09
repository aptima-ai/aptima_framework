//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include "aptima_runtime/aptima_env/internal/on_xxx_done.h"
#include "aptima_runtime/aptima_env/aptima_env.h"
#include "aptima_utils/lib/path.h"  // IWYU pragma: export
#include "aptima_utils/macro/ctor.h"

#define aptima_ADDON_REGISTER(TYPE, NAME, ADDON)                          \
  aptima_CONSTRUCTOR(____ctor_aptima_declare_##NAME##_##TYPE##_addon____) {  \
    aptima_string_t *base_dir = aptima_path_get_module_path(                 \
        (void *)____ctor_aptima_declare_##NAME##_##TYPE##_addon____);     \
    aptima_addon_register_##TYPE(#NAME, aptima_string_get_raw_str(base_dir), \
                              (ADDON));                                \
    aptima_string_destroy(base_dir);                                      \
  }                                                                    \
  aptima_DESTRUCTOR(____dtor_aptima_declare_##NAME##_##TYPE##_addon____) {   \
    aptima_addon_unregister_##TYPE(#NAME);                                \
  }

typedef struct aptima_addon_t aptima_addon_t;
typedef struct aptima_env_t aptima_env_t;

typedef enum aptima_ADDON_TYPE {
  aptima_ADDON_TYPE_INVALID,

  aptima_ADDON_TYPE_EXTENSION,
  aptima_ADDON_TYPE_EXTENSION_GROUP,  // Internal use only.
  aptima_ADDON_TYPE_PROTOCOL,
  aptima_ADDON_TYPE_ADDON_LOADER,
} aptima_ADDON_TYPE;

typedef void (*aptima_addon_on_init_func_t)(aptima_addon_t *addon,
                                         aptima_env_t *aptima_env);

typedef void (*aptima_addon_on_deinit_func_t)(aptima_addon_t *addon,
                                           aptima_env_t *aptima_env);

typedef void (*aptima_addon_on_create_instance_func_t)(aptima_addon_t *addon,
                                                    aptima_env_t *aptima_env,
                                                    const char *name,
                                                    void *context);

typedef void (*aptima_addon_on_destroy_instance_func_t)(aptima_addon_t *addon,
                                                     aptima_env_t *aptima_env,
                                                     void *instance,
                                                     void *context);

typedef void (*aptima_addon_on_destroy_func_t)(aptima_addon_t *addon);

aptima_RUNTIME_API aptima_addon_t *aptima_addon_create(
    aptima_addon_on_init_func_t on_init, aptima_addon_on_deinit_func_t on_deinit,
    aptima_addon_on_create_instance_func_t on_create_instance,
    aptima_addon_on_destroy_instance_func_t on_destroy_instance,
    aptima_addon_on_destroy_func_t on_destroy);

aptima_RUNTIME_API void aptima_addon_destroy(aptima_addon_t *self);
