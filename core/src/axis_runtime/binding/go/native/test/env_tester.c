//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/go/test/env_tester.h"

#include <stdlib.h>

#include "include_internal/axis_runtime/binding/go/internal/common.h"
#include "include_internal/axis_runtime/binding/go/msg/msg.h"
#include "include_internal/axis_runtime/binding/go/axis_env/axis_env_internal.h"
#include "axis_runtime/binding/common.h"
#include "axis_runtime/binding/go/interface/aptima/common.h"
#include "axis_runtime/binding/go/interface/aptima/axis_env_tester.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/macro/check.h"

bool axis_go_axis_env_tester_check_integrity(axis_go_axis_env_tester_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) != axis_GO_axis_ENV_TESTER_SIGNATURE) {
    return false;
  }

  return true;
}

axis_go_axis_env_tester_t *axis_go_axis_env_tester_reinterpret(
    uintptr_t bridge_addr) {
  axis_ASSERT(bridge_addr, "Should not happen.");

  // NOLINTNEXTLINE(performance-no-int-to-ptr)
  axis_go_axis_env_tester_t *self = (axis_go_axis_env_tester_t *)bridge_addr;
  axis_ASSERT(self && axis_go_axis_env_tester_check_integrity(self),
             "Should not happen.");

  return self;
}

static void axis_go_axis_env_tester_destroy(axis_go_axis_env_tester_t *self) {
  axis_ASSERT(self && axis_go_axis_env_tester_check_integrity(self),
             "Should not happen.");

  axis_FREE(self);
}

static void axis_go_axis_env_tester_destroy_c_part(void *axis_env_tester_bridge_) {
  axis_go_axis_env_tester_t *axis_env_tester_bridge =
      (axis_go_axis_env_tester_t *)axis_env_tester_bridge_;
  axis_ASSERT(axis_env_tester_bridge &&
                 axis_go_axis_env_tester_check_integrity(axis_env_tester_bridge),
             "Should not happen.");
  axis_env_tester_bridge->c_axis_env_tester = NULL;
  axis_go_bridge_destroy_c_part(&axis_env_tester_bridge->bridge);

  // Remove the Go aptima object from the global map.
  tenGoDestroyTenEnv(axis_env_tester_bridge->bridge.go_instance);
}

static void axis_go_axis_env_tester_close(void *axis_env_tester_bridge_) {
  axis_go_axis_env_tester_t *axis_env_tester_bridge =
      (axis_go_axis_env_tester_t *)axis_env_tester_bridge_;
  axis_ASSERT(axis_env_tester_bridge &&
                 axis_go_axis_env_tester_check_integrity(axis_env_tester_bridge),
             "Should not happen.");

  axis_env_tester_bridge->c_axis_env_tester = NULL;
}

axis_go_axis_env_tester_t *axis_go_axis_env_tester_wrap(
    axis_env_tester_t *c_axis_env_tester) {
  axis_go_axis_env_tester_t *axis_env_tester_bridge =
      axis_binding_handle_get_me_in_target_lang(
          (axis_binding_handle_t *)c_axis_env_tester);
  if (axis_env_tester_bridge) {
    return axis_env_tester_bridge;
  }

  axis_env_tester_bridge =
      (axis_go_axis_env_tester_t *)axis_MALLOC(sizeof(axis_go_axis_env_tester_t));
  axis_ASSERT(axis_env_tester_bridge, "Failed to allocate memory.");

  axis_signature_set(&axis_env_tester_bridge->signature,
                    axis_GO_axis_ENV_TESTER_SIGNATURE);

  uintptr_t bridge_addr = (uintptr_t)axis_env_tester_bridge;
  axis_ASSERT(bridge_addr, "Should not happen.");

  axis_env_tester_bridge->bridge.go_instance =
      tenGoCreateTenEnvTester(bridge_addr);

  // C aptima hold one reference of aptima bridge.
  axis_env_tester_bridge->bridge.sp_ref_by_c = axis_shared_ptr_create(
      axis_env_tester_bridge, axis_go_axis_env_tester_destroy);
  axis_env_tester_bridge->bridge.sp_ref_by_go =
      axis_shared_ptr_clone(axis_env_tester_bridge->bridge.sp_ref_by_c);

  axis_env_tester_bridge->c_axis_env_tester = c_axis_env_tester;

  axis_binding_handle_set_me_in_target_lang(
      (axis_binding_handle_t *)c_axis_env_tester, axis_env_tester_bridge);
  axis_env_tester_set_destroy_handler_in_target_lang(
      c_axis_env_tester, axis_go_axis_env_tester_destroy_c_part);
  axis_env_tester_set_close_handler_in_target_lang(c_axis_env_tester,
                                                  axis_go_axis_env_tester_close);

  return axis_env_tester_bridge;
}

axis_go_handle_t axis_go_axis_env_tester_go_handle(axis_go_axis_env_tester_t *self) {
  axis_ASSERT(self, "Should not happen.");

  return self->bridge.go_instance;
}

void axis_go_axis_env_tester_finalize(uintptr_t bridge_addr) {
  axis_go_axis_env_tester_t *self =
      axis_go_axis_env_tester_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_tester_check_integrity(self),
             "Should not happen.");

  axis_go_bridge_destroy_go_part(&self->bridge);
}

void axis_go_axis_env_tester_on_start_done(uintptr_t bridge_addr) {
  axis_go_axis_env_tester_t *self =
      axis_go_axis_env_tester_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_tester_check_integrity(self),
             "Should not happen.");
}

axis_go_error_t axis_go_axis_env_tester_send_cmd(uintptr_t bridge_addr,
                                              uintptr_t cmd_bridge_addr,
                                              axis_go_handle_t handler_id) {
  axis_go_axis_env_tester_t *self =
      axis_go_axis_env_tester_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_tester_check_integrity(self),
             "Should not happen.");

  axis_go_msg_t *cmd = axis_go_msg_reinterpret(cmd_bridge_addr);
  axis_ASSERT(cmd && axis_go_msg_check_integrity(cmd), "Should not happen.");
  axis_ASSERT(axis_go_msg_c_msg(cmd), "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  return cgo_error;
}
