//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/binding/go/interface/ten/extension.h"

#include <stdint.h>
#include <stdlib.h>

#include "include_internal/axis_runtime/binding/go/extension/extension.h"
#include "include_internal/axis_runtime/binding/go/internal/common.h"
#include "include_internal/axis_runtime/binding/go/msg/msg.h"
#include "include_internal/axis_runtime/binding/go/axis_env/axis_env.h"
#include "include_internal/axis_runtime/binding/go/axis_env/axis_env_internal.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/cmd.h"
#include "axis_runtime/binding/common.h"
#include "axis_runtime/binding/go/interface/ten/common.h"
#include "axis_runtime/binding/go/interface/ten/msg.h"
#include "axis_runtime/binding/go/interface/ten/axis_env.h"
#include "axis_runtime/extension/extension.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"

extern void tenGoExtensionOnConfigure(axis_go_handle_t go_extension,
                                      axis_go_handle_t go_axis_env);

extern void tenGoExtensionOnInit(axis_go_handle_t go_extension,
                                 axis_go_handle_t go_axis_env);

extern void tenGoExtensionOnStart(axis_go_handle_t go_extension,
                                  axis_go_handle_t go_axis_env);

extern void tenGoExtensionOnStop(axis_go_handle_t go_extension,
                                 axis_go_handle_t go_axis_env);

extern void tenGoExtensionOnDeinit(axis_go_handle_t go_extension,
                                   axis_go_handle_t go_axis_env);

extern void tenGoExtensionOnCmd(axis_go_handle_t go_extension,
                                axis_go_handle_t go_axis_env,
                                uintptr_t cmd_bridge_addr);

extern void tenGoExtensionOnData(axis_go_handle_t go_extension,
                                 axis_go_handle_t go_axis_env,
                                 uintptr_t data_bridge_addr);

extern void tenGoExtensionOnVideoFrame(axis_go_handle_t go_extension,
                                       axis_go_handle_t go_axis_env,
                                       uintptr_t video_frame_bridge_addr);

extern void tenGoExtensionOnAudioFrame(axis_go_handle_t go_extension,
                                       axis_go_handle_t go_axis_env,
                                       uintptr_t audio_frame_bridge_addr);

bool axis_go_extension_check_integrity(axis_go_extension_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) != axis_GO_EXTENSION_SIGNATURE) {
    return false;
  }

  return true;
}

axis_go_extension_t *axis_go_extension_reinterpret(uintptr_t bridge_addr) {
  axis_ASSERT(bridge_addr, "Invalid argument.");

  // NOLINTNEXTLINE(performance-no-int-to-ptr)
  axis_go_extension_t *self = (axis_go_extension_t *)bridge_addr;
  axis_ASSERT(axis_go_extension_check_integrity(self), "Invalid argument.");

  return self;
}

axis_go_handle_t axis_go_extension_go_handle(axis_go_extension_t *self) {
  axis_ASSERT(axis_go_extension_check_integrity(self), "Should not happen.");

  return self->bridge.go_instance;
}

static void axis_go_extension_bridge_destroy(axis_go_extension_t *self) {
  axis_ASSERT(axis_go_extension_check_integrity(self), "Should not happen.");

  axis_extension_t *c_extension = self->c_extension;
  axis_ASSERT(c_extension, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: In TEN world, the destroy operation should be performed in
  // any threads.
  axis_ASSERT(axis_extension_check_integrity(c_extension, false),
             "Invalid use of extension %p.", c_extension);

  axis_extension_destroy(c_extension);
  axis_FREE(self);
}

static void proxy_on_configure(axis_extension_t *self, axis_env_t *axis_env) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");
  axis_ASSERT(axis_extension_get_axis_env(self) == axis_env, "Should not happen.");

  axis_go_extension_t *extension_bridge =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)self);
  axis_ASSERT(axis_go_extension_check_integrity(extension_bridge),
             "Should not happen.");

  axis_go_axis_env_t *axis_env_bridge = axis_go_axis_env_wrap(axis_env);
  axis_env_bridge->c_axis_env_proxy = axis_env_proxy_create(axis_env, 1, NULL);

  tenGoExtensionOnConfigure(axis_go_extension_go_handle(extension_bridge),
                            axis_go_axis_env_go_handle(axis_env_bridge));
}

static void proxy_on_init(axis_extension_t *self, axis_env_t *axis_env) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");
  axis_ASSERT(axis_extension_get_axis_env(self) == axis_env, "Should not happen.");

  axis_go_extension_t *extension_bridge =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)self);
  axis_ASSERT(axis_go_extension_check_integrity(extension_bridge),
             "Should not happen.");

  axis_go_axis_env_t *axis_env_bridge = axis_go_axis_env_wrap(axis_env);

  tenGoExtensionOnInit(axis_go_extension_go_handle(extension_bridge),
                       axis_go_axis_env_go_handle(axis_env_bridge));
}

static void proxy_on_start(axis_extension_t *self, axis_env_t *axis_env) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");
  axis_ASSERT(axis_extension_get_axis_env(self) == axis_env, "Should not happen.");

  axis_go_extension_t *extension_bridge =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)self);
  axis_ASSERT(axis_go_extension_check_integrity(extension_bridge),
             "Should not happen.");

  axis_go_axis_env_t *axis_env_bridge = axis_go_axis_env_wrap(axis_env);

  tenGoExtensionOnStart(axis_go_extension_go_handle(extension_bridge),
                        axis_go_axis_env_go_handle(axis_env_bridge));
}

static void proxy_on_stop(axis_extension_t *self, axis_env_t *axis_env) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");
  axis_ASSERT(axis_extension_get_axis_env(self) == axis_env, "Should not happen.");

  axis_go_extension_t *extension_bridge =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)self);
  axis_ASSERT(axis_go_extension_check_integrity(extension_bridge),
             "Should not happen.");

  axis_go_axis_env_t *axis_env_bridge = axis_go_axis_env_wrap(axis_env);

  tenGoExtensionOnStop(axis_go_extension_go_handle(extension_bridge),
                       axis_go_axis_env_go_handle(axis_env_bridge));
}

static void proxy_on_deinit(axis_extension_t *self, axis_env_t *axis_env) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");
  axis_ASSERT(axis_extension_get_axis_env(self) == axis_env, "Should not happen.");

  axis_go_extension_t *extension_bridge =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)self);
  axis_ASSERT(axis_go_extension_check_integrity(extension_bridge),
             "Should not happen.");

  axis_go_axis_env_t *axis_env_bridge = axis_go_axis_env_wrap(axis_env);

  tenGoExtensionOnDeinit(axis_go_extension_go_handle(extension_bridge),
                         axis_go_axis_env_go_handle(axis_env_bridge));
}

static void proxy_on_cmd(axis_extension_t *self, axis_env_t *axis_env,
                         axis_shared_ptr_t *cmd) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");
  axis_ASSERT(axis_extension_get_axis_env(self) == axis_env, "Should not happen.");
  axis_ASSERT(cmd && axis_cmd_check_integrity(cmd), "Should not happen.");

  axis_go_extension_t *extension_bridge =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)self);
  axis_ASSERT(axis_go_extension_check_integrity(extension_bridge),
             "Should not happen.");

  axis_go_axis_env_t *axis_env_bridge = axis_go_axis_env_wrap(axis_env);

  // We only create the bridge instance in C world, and do _NOT_ call GO
  // function to create a GO cmd instance here. As the GO cmd instance is only
  // used by the GO extension, it can be created in GO world.
  axis_go_msg_t *msg_bridge = axis_go_msg_create(cmd);
  uintptr_t msg_bridge_addr = (uintptr_t)msg_bridge;

  tenGoExtensionOnCmd(axis_go_extension_go_handle(extension_bridge),
                      axis_go_axis_env_go_handle(axis_env_bridge),
                      msg_bridge_addr);
}

static void proxy_on_data(axis_extension_t *self, axis_env_t *axis_env,
                          axis_shared_ptr_t *data) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");
  axis_ASSERT(axis_extension_get_axis_env(self) == axis_env, "Should not happen.");

  axis_go_extension_t *extension_bridge =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)self);
  axis_ASSERT(axis_go_extension_check_integrity(extension_bridge),
             "Should not happen.");

  axis_go_axis_env_t *axis_env_bridge = axis_go_axis_env_wrap(axis_env);
  axis_go_msg_t *msg_bridge = axis_go_msg_create(data);

  uintptr_t msg_bridge_addr = (uintptr_t)msg_bridge;
  tenGoExtensionOnData(axis_go_extension_go_handle(extension_bridge),
                       axis_go_axis_env_go_handle(axis_env_bridge),
                       msg_bridge_addr);
}

static void proxy_on_video_frame(axis_extension_t *self, axis_env_t *axis_env,
                                 axis_shared_ptr_t *video_frame) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");
  axis_ASSERT(axis_extension_get_axis_env(self) == axis_env, "Should not happen.");

  axis_go_extension_t *extension_bridge =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)self);
  axis_ASSERT(axis_go_extension_check_integrity(extension_bridge),
             "Should not happen.");

  axis_go_axis_env_t *axis_env_bridge = axis_go_axis_env_wrap(axis_env);

  // Same as `on_cmd()`, the C world only care about the C bridge instance, but
  // does not need to create the GO instance from C. The GO instance can be
  // created in the GO world once the `tenGoExtensionOnVideoFrame()` is called
  // in GO world.
  axis_go_msg_t *msg_bridge = axis_go_msg_create(video_frame);
  uintptr_t msg_bridge_addr = (uintptr_t)msg_bridge;

  tenGoExtensionOnVideoFrame(axis_go_extension_go_handle(extension_bridge),
                             axis_go_axis_env_go_handle(axis_env_bridge),
                             msg_bridge_addr);
}

static void proxy_on_audio_frame(axis_extension_t *self, axis_env_t *axis_env,
                                 axis_shared_ptr_t *audio_frame) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");
  axis_ASSERT(axis_extension_get_axis_env(self) == axis_env, "Should not happen.");

  axis_go_extension_t *extension_bridge =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)self);
  axis_ASSERT(axis_go_extension_check_integrity(extension_bridge),
             "Should not happen.");

  axis_go_axis_env_t *axis_env_bridge = axis_go_axis_env_wrap(axis_env);
  axis_go_msg_t *msg_bridge = axis_go_msg_create(audio_frame);
  uintptr_t msg_bridge_addr = (uintptr_t)msg_bridge;

  tenGoExtensionOnAudioFrame(axis_go_extension_go_handle(extension_bridge),
                             axis_go_axis_env_go_handle(axis_env_bridge),
                             msg_bridge_addr);
}

static axis_go_extension_t *axis_go_extension_create_internal(
    axis_go_handle_t go_extension, const char *name) {
  axis_ASSERT(name, "Invalid argument.");

  axis_go_extension_t *extension_bridge =
      (axis_go_extension_t *)axis_MALLOC(sizeof(axis_go_extension_t));
  axis_ASSERT(extension_bridge, "Failed to allocate memory.");

  axis_signature_set(&extension_bridge->signature, axis_GO_EXTENSION_SIGNATURE);
  extension_bridge->bridge.go_instance = go_extension;

  // The extension bridge instance is created and managed only by Go. When the
  // Go extension is finalized, the extension bridge instance will be destroyed.
  // Therefore, the C part should not hold any reference to the extension bridge
  // instance.
  extension_bridge->bridge.sp_ref_by_go =
      axis_shared_ptr_create(extension_bridge, axis_go_extension_bridge_destroy);
  extension_bridge->bridge.sp_ref_by_c = NULL;

  extension_bridge->c_extension = axis_extension_create(
      name, proxy_on_configure, proxy_on_init, proxy_on_start, proxy_on_stop,
      proxy_on_deinit, proxy_on_cmd, proxy_on_data, proxy_on_audio_frame,
      proxy_on_video_frame, NULL);

  axis_binding_handle_set_me_in_target_lang(
      &extension_bridge->c_extension->binding_handle, extension_bridge);

  return extension_bridge;
}

axis_go_error_t axis_go_extension_create(axis_go_handle_t go_extension,
                                       const void *name, int name_len,
                                       uintptr_t *bridge_addr) {
  axis_ASSERT(go_extension > 0 && name && name_len > 0 && bridge_addr,
             "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_string_t extension_name;
  axis_string_init_formatted(&extension_name, "%.*s", name_len, name);

  axis_go_extension_t *extension = axis_go_extension_create_internal(
      go_extension, axis_string_get_raw_str(&extension_name));

  axis_string_deinit(&extension_name);

  *bridge_addr = (uintptr_t)extension;

  return cgo_error;
}

axis_extension_t *axis_go_extension_c_extension(axis_go_extension_t *self) {
  axis_ASSERT(axis_go_extension_check_integrity(self), "Should not happen.");

  return self->c_extension;
}

void axis_go_extension_finalize(uintptr_t bridge_addr) {
  axis_go_extension_t *self = axis_go_extension_reinterpret(bridge_addr);
  axis_ASSERT(axis_go_extension_check_integrity(self), "Should not happen.");

  axis_go_bridge_destroy_go_part(&self->bridge);
}
