//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "include_internal/ten_runtime/ten_env_proxy/ten_env_proxy.h"
#include "ten_runtime/addon/extension/extension.h"
#include "ten_runtime/binding/go/interface/ten/common.h"
#include "ten_runtime/binding/go/interface/ten/ten.h"
#include "ten_runtime/ten.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/lib/rwlock.h"

#define TEN_GO_TEN_IS_ALIVE_REGION_BEGIN(ten_bridge, err_stmt)          \
  do {                                                                  \
    ten_rwlock_lock((ten_bridge)->lock, 1);                             \
    if (((ten_bridge)->c_ten == NULL) ||                                \
        (((ten_bridge)->c_ten->attach_to != TEN_ENV_ATTACH_TO_ADDON) && \
         ((ten_bridge)->c_ten_proxy == NULL))) {                        \
      ten_rwlock_unlock((ten_bridge)->lock, 1);                         \
      {                                                                 \
        err_stmt                                                        \
      }                                                                 \
      goto ten_is_close;                                                \
    }                                                                   \
  } while (0)

#define TEN_GO_TEN_IS_ALIVE_REGION_END(ten_bridge) \
  do {                                             \
    ten_rwlock_unlock((ten_bridge)->lock, 1);      \
  } while (0)

typedef struct ten_go_ten_env_t {
  ten_signature_t signature;

  ten_go_bridge_t bridge;

  ten_env_t *c_ten;  // Point to the corresponding C ten.
  ten_env_proxy_t
      *c_ten_proxy;  // Point to the corresponding C ten_env_proxy if any.

  ten_rwlock_t *lock;
} ten_go_ten_env_t;

typedef struct ten_go_callback_info_t {
  ten_go_handle_t callback_id;
} ten_go_callback_info_t;

TEN_RUNTIME_PRIVATE_API ten_go_callback_info_t *ten_go_callback_info_create(
    ten_go_handle_t handler_id);

TEN_RUNTIME_PRIVATE_API void ten_go_callback_info_destroy(
    ten_go_callback_info_t *self);

TEN_RUNTIME_PRIVATE_API void proxy_send_xxx_callback(
    ten_extension_t *extension, ten_env_t *ten_env,
    ten_shared_ptr_t *cmd_result, void *callback_info);

ten_go_handle_t tenGoCreateTen(uintptr_t);

void tenGoDestroyTen(ten_go_handle_t go_ten);

void tenGoSetPropertyCallback(ten_go_handle_t ten, ten_go_handle_t handler,
                              bool result);

void tenGoGetPropertyCallback(ten_go_handle_t ten, ten_go_handle_t handler,
                              ten_go_handle_t value);

void tenGoOnAddonCreateExtensionDone(ten_go_handle_t ten, ten_go_handle_t addon,
                                     ten_go_handle_t extension,
                                     ten_go_handle_t handler);

void tenGoOnAddonDestroyExtensionDone(ten_go_handle_t ten,
                                      ten_go_handle_t handler);
