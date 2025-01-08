//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "include_internal/axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_runtime/addon/extension/extension.h"
#include "axis_runtime/binding/go/interface/ten/common.h"
#include "axis_runtime/binding/go/interface/ten/axis_env.h"
#include "axis_runtime/ten.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/lib/rwlock.h"

#define axis_GO_axis_ENV_IS_ALIVE_REGION_BEGIN(axis_env_bridge, err_stmt) \
  do {                                                                 \
    axis_rwlock_lock((axis_env_bridge)->lock, 1);                        \
    if (((axis_env_bridge)->c_axis_env == NULL) ||                       \
        (((axis_env_bridge)->c_axis_env->attach_to !=                    \
          axis_ENV_ATTACH_TO_ADDON) &&                                  \
         ((axis_env_bridge)->c_axis_env_proxy == NULL))) {               \
      axis_rwlock_unlock((axis_env_bridge)->lock, 1);                    \
      {                                                                \
        err_stmt                                                       \
      }                                                                \
      goto axis_is_close;                                               \
    }                                                                  \
  } while (0)

#define axis_GO_axis_ENV_IS_ALIVE_REGION_END(axis_env_bridge) \
  do {                                                     \
    axis_rwlock_unlock((axis_env_bridge)->lock, 1);          \
  } while (0)

typedef struct axis_go_axis_env_t {
  axis_signature_t signature;

  axis_go_bridge_t bridge;

  // Point to the corresponding C ten.
  axis_env_t *c_axis_env;

  // Point to the corresponding C axis_env_proxy if any.
  axis_env_proxy_t *c_axis_env_proxy;

  axis_rwlock_t *lock;
} axis_go_axis_env_t;

typedef struct axis_go_callback_ctx_t {
  axis_go_handle_t callback_id;
} axis_go_callback_ctx_t;

extern void tenGoOnCmdResult(axis_go_handle_t axis_env_bridge,
                             axis_go_handle_t cmd_result_bridge,
                             axis_go_handle_t result_handler,
                             axis_go_error_t cgo_error);

extern void tenGoOnError(axis_go_handle_t axis_env_bridge,
                         axis_go_handle_t error_handler,
                         axis_go_error_t cgo_error);

axis_RUNTIME_PRIVATE_API axis_go_callback_ctx_t *axis_go_callback_ctx_create(
    axis_go_handle_t handler_id);

axis_RUNTIME_PRIVATE_API void axis_go_callback_ctx_destroy(
    axis_go_callback_ctx_t *self);

axis_RUNTIME_PRIVATE_API void proxy_send_xxx_callback(
    axis_env_t *axis_env, axis_shared_ptr_t *cmd_result, void *callback_info,
    axis_error_t *err);

axis_go_handle_t tenGoCreateTenEnv(uintptr_t);

void tenGoDestroyTenEnv(axis_go_handle_t go_axis_env);

void tenGoSetPropertyCallback(axis_go_handle_t axis_env, axis_go_handle_t handler,
                              bool result);

void tenGoGetPropertyCallback(axis_go_handle_t axis_env, axis_go_handle_t handler,
                              axis_go_handle_t value);

void tenGoOnAddonCreateExtensionDone(axis_go_handle_t axis_env,
                                     axis_go_handle_t addon,
                                     axis_go_handle_t extension,
                                     axis_go_handle_t handler);

void tenGoOnAddonDestroyExtensionDone(axis_go_handle_t axis_env,
                                      axis_go_handle_t handler);
