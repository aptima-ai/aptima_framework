//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env.h"

#include <stdlib.h>

#include "include_internal/ten_runtime/binding/go/internal/common.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env_internal.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_utils/macro/check.h"
#include "ten_runtime/addon/extension/extension.h"
#include "ten_runtime/binding/common.h"
#include "ten_runtime/binding/go/interface/ten/common.h"
#include "ten_runtime/binding/go/interface/ten/ten_env.h"
#include "ten_runtime/ten.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/smart_ptr.h"

bool ten_go_ten_env_check_integrity(ten_go_ten_env_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  if (ten_signature_get(&self->signature) != TEN_GO_TEN_SIGNATURE) {
    return false;
  }

  return true;
}

ten_go_ten_env_t *ten_go_ten_env_reinterpret(uintptr_t bridge_addr) {
  TEN_ASSERT(bridge_addr > 0, "Should not happen.");

  // NOLINTNEXTLINE(performance-no-int-to-ptr)
  ten_go_ten_env_t *self = (ten_go_ten_env_t *)bridge_addr;
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  return self;
}

static void ten_go_ten_env_destroy(ten_go_ten_env_t *self) {
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  ten_rwlock_destroy(self->lock);
  TEN_FREE(self);
}

static void ten_go_ten_env_destroy_c_part(void *ten_bridge_) {
  ten_go_ten_env_t *ten_bridge = (ten_go_ten_env_t *)ten_bridge_;
  TEN_ASSERT(ten_bridge && ten_go_ten_env_check_integrity(ten_bridge),
             "Should not happen.");
  ten_bridge->c_ten_env = NULL;
  ten_go_bridge_destroy_c_part(&ten_bridge->bridge);

  // Remove the Go ten object from the global map.
  tenGoDestroyTen(ten_bridge->bridge.go_instance);
}

static void ten_go_ten_env_close(void *ten_bridge_) {
  ten_go_ten_env_t *ten_bridge = (ten_go_ten_env_t *)ten_bridge_;
  TEN_ASSERT(ten_bridge && ten_go_ten_env_check_integrity(ten_bridge),
             "Should not happen.");

  ten_rwlock_lock(ten_bridge->lock, 0);
  ten_bridge->c_ten_env = NULL;
  ten_rwlock_unlock(ten_bridge->lock, 0);
}

ten_go_ten_env_t *ten_go_ten_env_wrap(ten_env_t *c_ten) {
  ten_go_ten_env_t *ten_bridge =
      ten_binding_handle_get_me_in_target_lang((ten_binding_handle_t *)c_ten);
  if (ten_bridge) {
    return ten_bridge;
  }

  ten_bridge = (ten_go_ten_env_t *)TEN_MALLOC(sizeof(ten_go_ten_env_t));
  TEN_ASSERT(ten_bridge, "Failed to allocate memory.");

  ten_signature_set(&ten_bridge->signature, TEN_GO_TEN_SIGNATURE);

  uintptr_t bridge_addr = (uintptr_t)ten_bridge;
  TEN_ASSERT(bridge_addr > 0, "Should not happen.");

  ten_bridge->bridge.go_instance = tenGoCreateTen(bridge_addr);

  // C ten hold one reference of ten bridge.
  ten_bridge->bridge.sp_ref_by_c =
      ten_shared_ptr_create(ten_bridge, ten_go_ten_env_destroy);
  ten_bridge->bridge.sp_ref_by_go =
      ten_shared_ptr_clone(ten_bridge->bridge.sp_ref_by_c);

  ten_bridge->c_ten_env = c_ten;
  ten_bridge->c_ten_env_proxy = NULL;

  ten_binding_handle_set_me_in_target_lang((ten_binding_handle_t *)c_ten,
                                           ten_bridge);
  ten_env_set_destroy_handler_in_target_lang(c_ten,
                                             ten_go_ten_env_destroy_c_part);
  ten_env_set_close_handler_in_target_lang(c_ten, ten_go_ten_env_close);

  ten_bridge->lock = ten_rwlock_create(TEN_RW_DEFAULT_FAIRNESS);

  return ten_bridge;
}

ten_go_handle_t ten_go_ten_env_go_handle(ten_go_ten_env_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  return self->bridge.go_instance;
}

void ten_go_ten_env_finalize(uintptr_t bridge_addr) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  ten_go_bridge_destroy_go_part(&self->bridge);
}

const char *ten_go_ten_env_debug_info(uintptr_t bridge_addr) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  ten_string_t debug_info;
  ten_string_init_formatted(&debug_info, "ten attach_to type: %d",
                            self->c_ten_env->attach_to);
  const char *res = ten_go_str_dup(ten_string_get_raw_str(&debug_info));

  ten_string_deinit(&debug_info);

  return res;
}
