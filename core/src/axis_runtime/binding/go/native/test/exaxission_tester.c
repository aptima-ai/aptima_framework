//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/go/test/extension_tester.h"

#include <stdint.h>

#include "include_internal/axis_runtime/binding/go/internal/common.h"
#include "include_internal/axis_runtime/binding/go/msg/msg.h"
#include "include_internal/axis_runtime/binding/go/test/env_tester.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/cmd.h"
#include "include_internal/axis_runtime/test/env_tester.h"
#include "axis_runtime/binding/common.h"
#include "axis_runtime/binding/go/interface/ten/common.h"
#include "axis_runtime/binding/go/interface/ten/msg.h"
#include "axis_runtime/binding/go/interface/ten/axis_env.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/macro/check.h"

extern void tenGoExtensionTesterOnStart(axis_go_handle_t go_extension_tester,
                                        axis_go_handle_t go_axis_env_tester);

extern void tenGoExtensionTesterOnCmd(axis_go_handle_t go_extension_tester,
                                      axis_go_handle_t go_axis_env_tester,
                                      uintptr_t cmd_bridge_addr);

extern void tenGoExtensionTesterOnData(axis_go_handle_t go_extension_tester,
                                       axis_go_handle_t go_axis_env_tester,
                                       uintptr_t data_bridge_addr);

extern void tenGoExtensionTesterOnAudioFrame(
    axis_go_handle_t go_extension_tester, axis_go_handle_t go_axis_env_tester,
    uintptr_t audio_frame_bridge_addr);

extern void tenGoExtensionTesterOnVideoFrame(
    axis_go_handle_t go_extension_tester, axis_go_handle_t go_axis_env_tester,
    uintptr_t video_frame_bridge_addr);

bool axis_go_extension_tester_check_integrity(axis_go_extension_tester_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      axis_GO_EXTENSION_TESTER_SIGNATURE) {
    return false;
  }

  return true;
}

axis_go_extension_tester_t *axis_go_extension_tester_reinterpret(
    uintptr_t bridge_addr) {
  axis_ASSERT(bridge_addr, "Invalid argument.");

  // NOLINTNEXTLINE(performance-no-int-to-ptr)
  axis_go_extension_tester_t *self = (axis_go_extension_tester_t *)bridge_addr;
  axis_ASSERT(axis_go_extension_tester_check_integrity(self),
             "Invalid argument.");

  return self;
}

axis_go_handle_t axis_go_extension_tester_go_handle(
    axis_go_extension_tester_t *self) {
  axis_ASSERT(axis_go_extension_tester_check_integrity(self),
             "Should not happen.");

  return self->bridge.go_instance;
}

static void axis_go_extension_tester_bridge_destroy(
    axis_go_extension_tester_t *self) {
  axis_ASSERT(axis_go_extension_tester_check_integrity(self),
             "Should not happen.");

  axis_extension_tester_t *c_extension_tester = self->c_extension_tester;
  axis_ASSERT(c_extension_tester, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: In TEN world, the destroy operation should be performed in
  // any threads.
  axis_ASSERT(axis_extension_tester_check_integrity(c_extension_tester, false),
             "Invalid use of extension_tester %p.", c_extension_tester);

  axis_extension_tester_destroy(c_extension_tester);
  axis_FREE(self);
}

static void proxy_on_start(axis_extension_tester_t *self,
                           axis_env_tester_t *axis_env_tester) {
  axis_ASSERT(self && axis_extension_tester_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(axis_env_tester && axis_env_tester_check_integrity(axis_env_tester),
             "Should not happen.");
  axis_ASSERT(axis_extension_tester_get_axis_env_tester(self) == axis_env_tester,
             "Should not happen.");

  axis_go_extension_tester_t *extension_tester_bridge =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)self);
  axis_ASSERT(axis_go_extension_tester_check_integrity(extension_tester_bridge),
             "Should not happen.");

  axis_go_axis_env_tester_t *axis_env_tester_bridge =
      axis_go_axis_env_tester_wrap(axis_env_tester);

  tenGoExtensionTesterOnStart(
      axis_go_extension_tester_go_handle(extension_tester_bridge),
      axis_go_axis_env_tester_go_handle(axis_env_tester_bridge));
}

static void proxy_on_cmd(axis_extension_tester_t *self,
                         axis_env_tester_t *axis_env_tester,
                         axis_shared_ptr_t *cmd) {
  axis_ASSERT(self && axis_extension_tester_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(axis_env_tester && axis_env_tester_check_integrity(axis_env_tester),
             "Should not happen.");
  axis_ASSERT(axis_extension_tester_get_axis_env_tester(self) == axis_env_tester,
             "Should not happen.");
  axis_ASSERT(cmd && axis_cmd_check_integrity(cmd), "Should not happen.");

  axis_go_extension_tester_t *extension_tester_bridge =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)self);
  axis_ASSERT(axis_go_extension_tester_check_integrity(extension_tester_bridge),
             "Should not happen.");

  axis_go_axis_env_tester_t *axis_env_tester_bridge =
      axis_go_axis_env_tester_wrap(axis_env_tester);

  axis_go_msg_t *msg_bridge = axis_go_msg_create(cmd);
  uintptr_t msg_bridge_addr = (uintptr_t)msg_bridge;

  tenGoExtensionTesterOnCmd(
      axis_go_extension_tester_go_handle(extension_tester_bridge),
      axis_go_axis_env_tester_go_handle(axis_env_tester_bridge), msg_bridge_addr);
}

static void proxy_on_data(axis_extension_tester_t *self,
                          axis_env_tester_t *axis_env_tester,
                          axis_shared_ptr_t *data) {
  axis_ASSERT(self && axis_extension_tester_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(axis_env_tester && axis_env_tester_check_integrity(axis_env_tester),
             "Should not happen.");
  axis_ASSERT(axis_extension_tester_get_axis_env_tester(self) == axis_env_tester,
             "Should not happen.");
  axis_ASSERT(data && axis_msg_check_integrity(data), "Should not happen.");

  axis_go_extension_tester_t *extension_tester_bridge =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)self);
  axis_ASSERT(axis_go_extension_tester_check_integrity(extension_tester_bridge),
             "Should not happen.");

  axis_go_axis_env_tester_t *axis_env_tester_bridge =
      axis_go_axis_env_tester_wrap(axis_env_tester);

  axis_go_msg_t *msg_bridge = axis_go_msg_create(data);
  uintptr_t msg_bridge_addr = (uintptr_t)msg_bridge;

  tenGoExtensionTesterOnData(
      axis_go_extension_tester_go_handle(extension_tester_bridge),
      axis_go_axis_env_tester_go_handle(axis_env_tester_bridge), msg_bridge_addr);
}

static void proxy_on_audio_frame(axis_extension_tester_t *self,
                                 axis_env_tester_t *axis_env_tester,
                                 axis_shared_ptr_t *audio_frame) {
  axis_ASSERT(self && axis_extension_tester_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(axis_env_tester && axis_env_tester_check_integrity(axis_env_tester),
             "Should not happen.");
  axis_ASSERT(axis_extension_tester_get_axis_env_tester(self) == axis_env_tester,
             "Should not happen.");
  axis_ASSERT(audio_frame && axis_msg_check_integrity(audio_frame),
             "Should not happen.");

  axis_go_extension_tester_t *extension_tester_bridge =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)self);
  axis_ASSERT(axis_go_extension_tester_check_integrity(extension_tester_bridge),
             "Should not happen.");

  axis_go_axis_env_tester_t *axis_env_tester_bridge =
      axis_go_axis_env_tester_wrap(axis_env_tester);

  axis_go_msg_t *msg_bridge = axis_go_msg_create(audio_frame);
  uintptr_t msg_bridge_addr = (uintptr_t)msg_bridge;

  tenGoExtensionTesterOnAudioFrame(
      axis_go_extension_tester_go_handle(extension_tester_bridge),
      axis_go_axis_env_tester_go_handle(axis_env_tester_bridge), msg_bridge_addr);
}

static void proxy_on_video_frame(axis_extension_tester_t *self,
                                 axis_env_tester_t *axis_env_tester,
                                 axis_shared_ptr_t *video_frame) {
  axis_ASSERT(self && axis_extension_tester_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(axis_env_tester && axis_env_tester_check_integrity(axis_env_tester),
             "Should not happen.");
  axis_ASSERT(axis_extension_tester_get_axis_env_tester(self) == axis_env_tester,
             "Should not happen.");
  axis_ASSERT(video_frame && axis_msg_check_integrity(video_frame),
             "Should not happen.");

  axis_go_extension_tester_t *extension_tester_bridge =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)self);
  axis_ASSERT(axis_go_extension_tester_check_integrity(extension_tester_bridge),
             "Should not happen.");

  axis_go_axis_env_tester_t *axis_env_tester_bridge =
      axis_go_axis_env_tester_wrap(axis_env_tester);

  axis_go_msg_t *msg_bridge = axis_go_msg_create(video_frame);
  uintptr_t msg_bridge_addr = (uintptr_t)msg_bridge;

  tenGoExtensionTesterOnVideoFrame(
      axis_go_extension_tester_go_handle(extension_tester_bridge),
      axis_go_axis_env_tester_go_handle(axis_env_tester_bridge), msg_bridge_addr);
}

axis_go_error_t axis_go_extension_tester_create(
    axis_go_handle_t go_extension_tester, uintptr_t *bridge_addr) {
  axis_ASSERT(go_extension_tester > 0 && bridge_addr, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_extension_tester_t *extension_tester =
      axis_go_extension_tester_create_internal(go_extension_tester);

  *bridge_addr = (uintptr_t)extension_tester;

  return cgo_error;
}

void axis_go_extension_tester_finalize(uintptr_t bridge_addr) {
  axis_go_extension_tester_t *self =
      axis_go_extension_tester_reinterpret(bridge_addr);
  axis_ASSERT(axis_go_extension_tester_check_integrity(self),
             "Should not happen.");

  axis_go_bridge_destroy_go_part(&self->bridge);
}

axis_go_extension_tester_t *axis_go_extension_tester_create_internal(
    axis_go_handle_t go_extension_tester) {
  axis_go_extension_tester_t *extension_tester_bridge =
      (axis_go_extension_tester_t *)axis_MALLOC(
          sizeof(axis_go_extension_tester_t));
  axis_ASSERT(extension_tester_bridge, "Failed to allocate memory.");

  axis_signature_set(&extension_tester_bridge->signature,
                    axis_GO_EXTENSION_TESTER_SIGNATURE);
  extension_tester_bridge->bridge.go_instance = go_extension_tester;

  extension_tester_bridge->bridge.sp_ref_by_go = axis_shared_ptr_create(
      extension_tester_bridge, axis_go_extension_tester_bridge_destroy);
  extension_tester_bridge->bridge.sp_ref_by_c = NULL;

  extension_tester_bridge->c_extension_tester =
      axis_extension_tester_create(proxy_on_start, proxy_on_cmd, proxy_on_data,
                                  proxy_on_audio_frame, proxy_on_video_frame);

  axis_binding_handle_set_me_in_target_lang(
      &extension_tester_bridge->c_extension_tester->binding_handle,
      extension_tester_bridge);

  return extension_tester_bridge;
}
