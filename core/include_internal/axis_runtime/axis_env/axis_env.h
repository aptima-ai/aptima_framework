//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/binding/common.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/macro/check.h"

#define axis_ENV_SIGNATURE 0x1336D348DA779EA6U

typedef struct axis_engine_t axis_engine_t;

typedef void (*axis_env_close_handler_in_target_lang_func_t)(
    void *me_in_target_lang);

typedef void (*axis_env_destroy_handler_in_target_lang_func_t)(
    void *me_in_target_lang);

typedef enum axis_ENV_ATTACH_TO {
  axis_ENV_ATTACH_TO_INVALID,

  axis_ENV_ATTACH_TO_EXTENSION,
  axis_ENV_ATTACH_TO_EXTENSION_GROUP,
  axis_ENV_ATTACH_TO_APP,
  axis_ENV_ATTACH_TO_ADDON,
  axis_ENV_ATTACH_TO_ENGINE,
} axis_ENV_ATTACH_TO;

typedef struct axis_env_t {
  axis_binding_handle_t binding_handle;

  axis_signature_t signature;
  axis_sanitizer_thread_check_t thread_check;

  axis_ENV_ATTACH_TO attach_to;

  union {
    // This is the extension which this axis_env_t object applies.
    axis_extension_t *extension;

    // This is the extension group which this axis_env_t object applies.
    axis_extension_group_t *extension_group;

    // This is the app which this axis_env_t object applies.
    axis_app_t *app;

    // This is the addon which this axis_env_t object applies.
    axis_addon_host_t *addon_host;

    // This is the engine which this axis_env_t object applies.
    axis_engine_t *engine;
  } attached_target;

  // TODO(Wei): Do we need this close_handler?
  axis_env_close_handler_in_target_lang_func_t close_handler;

  axis_env_destroy_handler_in_target_lang_func_t destroy_handler;

  axis_list_t axis_proxy_list;
} axis_env_t;

axis_RUNTIME_PRIVATE_API axis_runloop_t *axis_env_get_attached_runloop(
    axis_env_t *self);

axis_RUNTIME_PRIVATE_API axis_env_t *axis_env_create_for_extension_group(
    axis_extension_group_t *extension_group);

axis_RUNTIME_PRIVATE_API axis_env_t *axis_env_create_for_addon(
    axis_addon_host_t *addon_host);

axis_RUNTIME_PRIVATE_API axis_env_t *axis_env_create_for_extension(
    axis_extension_t *extension);

axis_RUNTIME_PRIVATE_API axis_env_t *axis_env_create_for_app(axis_app_t *app);

axis_RUNTIME_PRIVATE_API axis_env_t *axis_env_create_for_engine(
    axis_engine_t *engine);

axis_RUNTIME_PRIVATE_API void axis_env_close(axis_env_t *self);

axis_RUNTIME_PRIVATE_API axis_env_t *axis_env_create(void);

axis_RUNTIME_API void axis_env_set_close_handler_in_target_lang(
    axis_env_t *self, axis_env_close_handler_in_target_lang_func_t handler);

axis_RUNTIME_API void axis_env_set_destroy_handler_in_target_lang(
    axis_env_t *self, axis_env_destroy_handler_in_target_lang_func_t handler);

axis_RUNTIME_API axis_ENV_ATTACH_TO axis_env_get_attach_to(axis_env_t *self);

axis_RUNTIME_PRIVATE_API void axis_env_set_attach_to(
    axis_env_t *self, axis_ENV_ATTACH_TO attach_to_type, void *attach_to);

axis_RUNTIME_PRIVATE_API const char *axis_env_get_attached_instance_name(
    axis_env_t *self, bool check_thread);

axis_RUNTIME_PRIVATE_API axis_app_t *axis_env_get_belonging_app(axis_env_t *self);

inline axis_extension_t *axis_env_get_attached_extension(axis_env_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: self->attach_to is not changed after ten is created.
  axis_ASSERT(axis_env_check_integrity(self, false), "Invalid use of axis_env %p.",
             self);

  axis_ASSERT(self->attach_to == axis_ENV_ATTACH_TO_EXTENSION,
             "Should not happen.");

  return self->attached_target.extension;
}

inline axis_extension_group_t *axis_env_get_attached_extension_group(
    axis_env_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: self->attach_to is not changed after ten is created.
  axis_ASSERT(axis_env_check_integrity(self, false), "Invalid use of axis_env %p.",
             self);

  axis_ASSERT(self->attach_to == axis_ENV_ATTACH_TO_EXTENSION_GROUP,
             "Should not happen.");

  return self->attached_target.extension_group;
}

inline axis_app_t *axis_env_get_attached_app(axis_env_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: self->attach_to is not changed after ten is created.
  axis_ASSERT(axis_env_check_integrity(self, false), "Invalid use of axis_env %p.",
             self);

  axis_ASSERT(self->attach_to == axis_ENV_ATTACH_TO_APP, "Should not happen.");

  return self->attached_target.app;
}

inline axis_addon_host_t *axis_env_get_attached_addon(axis_env_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: self->attach_to is not changed after ten is created.
  axis_ASSERT(axis_env_check_integrity(self, false), "Invalid use of axis_env %p.",
             self);

  axis_ASSERT(self->attach_to == axis_ENV_ATTACH_TO_ADDON, "Should not happen.");

  return self->attached_target.addon_host;
}

inline axis_engine_t *axis_env_get_attached_engine(axis_env_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: self->attach_to is not changed after ten is created.
  axis_ASSERT(axis_env_check_integrity(self, false), "Invalid use of axis_env %p.",
             self);

  axis_ASSERT(self->attach_to == axis_ENV_ATTACH_TO_ENGINE, "Should not happen.");

  return self->attached_target.engine;
}
