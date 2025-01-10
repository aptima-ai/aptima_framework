//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension_context/axis_env/on_xxx.h"

#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/macro/memory.h"

void axis_extension_context_on_addon_create_extension_group_done(
    axis_env_t *self, void *instance, axis_addon_context_t *addon_context) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(self, true), "Invalid use of axis_env %p.",
             self);

  axis_ASSERT(self->attach_to == axis_ENV_ATTACH_TO_ENGINE, "Should not happen.");

  axis_UNUSED axis_engine_t *engine = axis_env_get_attached_engine(self);
  axis_ASSERT(engine && axis_engine_check_integrity(engine, true),
             "Should not happen.");

  axis_UNUSED axis_extension_group_t *extension_group = instance;
  axis_ASSERT(extension_group &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: The extension thread has not been created yet,
                 // so it is thread safe.
                 axis_extension_group_check_integrity(extension_group, false),
             "Should not happen.");

  axis_env_t *extension_group_ten = extension_group->axis_env;
  axis_ASSERT(extension_group_ten, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: The extension thread has not been created yet, so it is
  // thread safe.
  axis_ASSERT(axis_env_check_integrity(extension_group_ten, false),
             "Invalid use of axis_env %p.", extension_group_ten);

  // This happens on the engine thread, so it's thread safe.

  if (addon_context->create_instance_done_cb) {
    addon_context->create_instance_done_cb(
        self, instance, addon_context->create_instance_done_cb_data);
  }

  if (addon_context) {
    axis_addon_context_destroy(addon_context);
  }
}

void axis_extension_context_on_addon_destroy_extension_group_done(
    axis_env_t *self, axis_addon_context_t *addon_context) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(self, true), "Invalid use of axis_env %p.",
             self);

  axis_ASSERT(self->attach_to == axis_ENV_ATTACH_TO_ENGINE, "Should not happen.");

  axis_UNUSED axis_engine_t *engine = axis_env_get_attached_engine(self);
  axis_ASSERT(engine && axis_engine_check_integrity(engine, true),
             "Should not happen.");

  if (addon_context->destroy_instance_done_cb) {
    addon_context->destroy_instance_done_cb(
        self, addon_context->destroy_instance_done_cb_data);
  }

  axis_addon_context_destroy(addon_context);
}

axis_extension_context_on_addon_create_extension_group_done_ctx_t *
axis_extension_context_on_addon_create_extension_group_done_ctx_create(void) {
  axis_extension_context_on_addon_create_extension_group_done_ctx_t *self =
      axis_MALLOC(sizeof(
          axis_extension_context_on_addon_create_extension_group_done_ctx_t));

  self->addon_context = NULL;
  self->extension_group = NULL;

  return self;
}

void axis_extension_context_on_addon_create_extension_group_done_ctx_destroy(
    axis_extension_context_on_addon_create_extension_group_done_ctx_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_FREE(self);
}
