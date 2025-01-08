//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_runtime/axis_env/internal/on_xxx_done.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/lib/path.h"  // IWYU pragma: export
#include "axis_utils/macro/ctor.h"

#define axis_ADDON_REGISTER(TYPE, NAME, ADDON)                          \
  axis_CONSTRUCTOR(____ctor_axis_declare_##NAME##_##TYPE##_addon____) {  \
    axis_string_t *base_dir = axis_path_get_module_path(                 \
        (void *)____ctor_axis_declare_##NAME##_##TYPE##_addon____);     \
    axis_addon_register_##TYPE(#NAME, axis_string_get_raw_str(base_dir), \
                              (ADDON));                                \
    axis_string_destroy(base_dir);                                      \
  }                                                                    \
  axis_DESTRUCTOR(____dtor_axis_declare_##NAME##_##TYPE##_addon____) {   \
    axis_addon_unregister_##TYPE(#NAME);                                \
  }

typedef struct axis_addon_t axis_addon_t;
typedef struct axis_env_t axis_env_t;

typedef enum axis_ADDON_TYPE {
  axis_ADDON_TYPE_INVALID,

  axis_ADDON_TYPE_EXTENSION,
  axis_ADDON_TYPE_EXTENSION_GROUP,  // Internal use only.
  axis_ADDON_TYPE_PROTOCOL,
  axis_ADDON_TYPE_ADDON_LOADER,
} axis_ADDON_TYPE;

typedef void (*axis_addon_on_init_func_t)(axis_addon_t *addon,
                                         axis_env_t *axis_env);

typedef void (*axis_addon_on_deinit_func_t)(axis_addon_t *addon,
                                           axis_env_t *axis_env);

typedef void (*axis_addon_on_create_instance_func_t)(axis_addon_t *addon,
                                                    axis_env_t *axis_env,
                                                    const char *name,
                                                    void *context);

typedef void (*axis_addon_on_destroy_instance_func_t)(axis_addon_t *addon,
                                                     axis_env_t *axis_env,
                                                     void *instance,
                                                     void *context);

typedef void (*axis_addon_on_destroy_func_t)(axis_addon_t *addon);

axis_RUNTIME_API axis_addon_t *axis_addon_create(
    axis_addon_on_init_func_t on_init, axis_addon_on_deinit_func_t on_deinit,
    axis_addon_on_create_instance_func_t on_create_instance,
    axis_addon_on_destroy_instance_func_t on_destroy_instance,
    axis_addon_on_destroy_func_t on_destroy);

axis_RUNTIME_API void axis_addon_destroy(axis_addon_t *self);
