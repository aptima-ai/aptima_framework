//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_runtime/ten_env/internal/on_xxx_done.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/macro/ctor.h"

#define TEN_ADDON_REGISTER(TYPE, NAME, ADDON)                         \
  TEN_CONSTRUCTOR(____ctor_ten_declare_##NAME##_##TYPE##_addon____) { \
    ten_addon_register_##TYPE(#NAME, (ADDON));                        \
  }                                                                   \
  TEN_DESTRUCTOR(____dtor_ten_declare_##NAME##_##TYPE##_addon____) {  \
    ten_addon_unregister_##TYPE(#NAME, (ADDON));                      \
  }

typedef struct ten_addon_t ten_addon_t;
typedef struct ten_env_t ten_env_t;

typedef void (*ten_addon_on_init_func_t)(ten_addon_t *addon,
                                         ten_env_t *ten_env);

typedef void (*ten_addon_on_deinit_func_t)(ten_addon_t *addon,
                                           ten_env_t *ten_env);

typedef void (*ten_addon_on_create_instance_async_func_t)(ten_addon_t *addon,
                                                          ten_env_t *ten_env,
                                                          const char *name,
                                                          void *context);

typedef void (*ten_addon_on_destroy_instance_async_func_t)(ten_addon_t *addon,
                                                           ten_env_t *ten_env,
                                                           void *instance,
                                                           void *context);

TEN_RUNTIME_API ten_addon_t *ten_addon_create(
    ten_addon_on_init_func_t on_init, ten_addon_on_deinit_func_t on_deinit,
    ten_addon_on_create_instance_async_func_t on_create_instance_async,
    ten_addon_on_destroy_instance_async_func_t on_destroy_instance_async);

TEN_RUNTIME_API void ten_addon_init(
    ten_addon_t *self, ten_addon_on_init_func_t on_init,
    ten_addon_on_deinit_func_t on_deinit,
    ten_addon_on_create_instance_async_func_t on_create_instance_async,
    ten_addon_on_destroy_instance_async_func_t on_destroy_instance_async);

TEN_RUNTIME_API void ten_addon_destroy(ten_addon_t *self);

TEN_RUNTIME_API ten_env_t *ten_addon_get_ten(ten_addon_t *self);
