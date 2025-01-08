//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/go/axis_env/axis_env.h"

#include <stdlib.h>

#include "include_internal/axis_runtime/binding/go/internal/common.h"
#include "include_internal/axis_runtime/binding/go/axis_env/axis_env_internal.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/addon/extension/extension.h"
#include "axis_runtime/binding/common.h"
#include "axis_runtime/binding/go/interface/ten/common.h"
#include "axis_runtime/binding/go/interface/ten/axis_env.h"
#include "axis_runtime/ten.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/macro/check.h"

bool axis_go_axis_env_check_integrity(axis_go_axis_env_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) != axis_GO_axis_ENV_SIGNATURE) {
    return false;
  }

  return true;
}

axis_go_axis_env_t *axis_go_axis_env_reinterpret(uintptr_t bridge_addr) {
  axis_ASSERT(bridge_addr, "Should not happen.");

  // NOLINTNEXTLINE(performance-no-int-to-ptr)
  axis_go_axis_env_t *self = (axis_go_axis_env_t *)bridge_addr;
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");

  return self;
}

static void axis_go_axis_env_destroy(axis_go_axis_env_t *self) {
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");

  axis_rwlock_destroy(self->lock);
  axis_FREE(self);
}

static void axis_go_axis_env_destroy_c_part(void *axis_env_bridge_) {
  axis_go_axis_env_t *axis_env_bridge = (axis_go_axis_env_t *)axis_env_bridge_;
  axis_ASSERT(axis_env_bridge && axis_go_axis_env_check_integrity(axis_env_bridge),
             "Should not happen.");
  axis_env_bridge->c_axis_env = NULL;
  axis_go_bridge_destroy_c_part(&axis_env_bridge->bridge);

  // Remove the Go ten object from the global map.
  tenGoDestroyTenEnv(axis_env_bridge->bridge.go_instance);
}

static void axis_go_axis_env_close(void *axis_env_bridge_) {
  axis_go_axis_env_t *axis_env_bridge = (axis_go_axis_env_t *)axis_env_bridge_;
  axis_ASSERT(axis_env_bridge && axis_go_axis_env_check_integrity(axis_env_bridge),
             "Should not happen.");

  axis_rwlock_lock(axis_env_bridge->lock, 0);
  axis_env_bridge->c_axis_env = NULL;
  axis_rwlock_unlock(axis_env_bridge->lock, 0);
}

axis_go_axis_env_t *axis_go_axis_env_wrap(axis_env_t *c_axis_env) {
  axis_go_axis_env_t *axis_env_bridge = axis_binding_handle_get_me_in_target_lang(
      (axis_binding_handle_t *)c_axis_env);
  if (axis_env_bridge) {
    return axis_env_bridge;
  }

  axis_env_bridge = (axis_go_axis_env_t *)axis_MALLOC(sizeof(axis_go_axis_env_t));
  axis_ASSERT(axis_env_bridge, "Failed to allocate memory.");

  axis_signature_set(&axis_env_bridge->signature, axis_GO_axis_ENV_SIGNATURE);

  uintptr_t bridge_addr = (uintptr_t)axis_env_bridge;
  axis_ASSERT(bridge_addr, "Should not happen.");

  axis_env_bridge->bridge.go_instance = tenGoCreateTenEnv(bridge_addr);

  // C ten hold one reference of ten bridge.
  axis_env_bridge->bridge.sp_ref_by_c =
      axis_shared_ptr_create(axis_env_bridge, axis_go_axis_env_destroy);
  axis_env_bridge->bridge.sp_ref_by_go =
      axis_shared_ptr_clone(axis_env_bridge->bridge.sp_ref_by_c);

  axis_env_bridge->c_axis_env = c_axis_env;
  axis_env_bridge->c_axis_env_proxy = NULL;

  axis_binding_handle_set_me_in_target_lang((axis_binding_handle_t *)c_axis_env,
                                           axis_env_bridge);
  axis_env_set_destroy_handler_in_target_lang(c_axis_env,
                                             axis_go_axis_env_destroy_c_part);
  axis_env_set_close_handler_in_target_lang(c_axis_env, axis_go_axis_env_close);

  axis_env_bridge->lock = axis_rwlock_create(axis_RW_DEFAULT_FAIRNESS);

  return axis_env_bridge;
}

axis_go_handle_t axis_go_axis_env_go_handle(axis_go_axis_env_t *self) {
  axis_ASSERT(self, "Should not happen.");

  return self->bridge.go_instance;
}

void axis_go_axis_env_finalize(uintptr_t bridge_addr) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");

  axis_go_bridge_destroy_go_part(&self->bridge);
}

const char *axis_go_axis_env_debug_info(uintptr_t bridge_addr) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");

  axis_string_t debug_info;
  axis_string_init_formatted(&debug_info, "ten attach_to type: %d",
                            self->c_axis_env->attach_to);
  const char *res = axis_go_str_dup(axis_string_get_raw_str(&debug_info));

  axis_string_deinit(&debug_info);

  return res;
}
