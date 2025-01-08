//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/axis_env/internal/on_xxx_done.h"

#include "include_internal/axis_runtime/addon/extension/extension.h"
#include "include_internal/axis_runtime/addon/axis_env/on_xxx.h"
#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/engine/on_xxx.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension/on_xxx.h"
#include "include_internal/axis_runtime/extension_context/extension_context.h"
#include "include_internal/axis_runtime/extension_context/axis_env/on_xxx.h"
#include "include_internal/axis_runtime/extension_group/builtin/builtin_extension_group.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/extension_group/on_xxx.h"
#include "include_internal/axis_runtime/metadata/metadata_info.h"
#include "include_internal/axis_runtime/axis_env/on_xxx_done.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

bool axis_env_on_configure_done(axis_env_t *self, axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(
      axis_env_check_integrity(
          self, self->attach_to != axis_ENV_ATTACH_TO_ADDON ? true : false),
      "Invalid use of axis_env %p.", self);

  switch (self->attach_to) {
    case axis_ENV_ATTACH_TO_EXTENSION:
      return axis_extension_on_configure_done(self);

    case axis_ENV_ATTACH_TO_APP:
      axis_app_on_configure_done(self);
      break;

    case axis_ENV_ATTACH_TO_EXTENSION_GROUP:
    case axis_ENV_ATTACH_TO_ADDON:
      axis_ASSERT(0, "Handle these types.");
      break;

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  return true;
}

bool axis_env_on_init_done(axis_env_t *self, axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(
      axis_env_check_integrity(
          self, self->attach_to != axis_ENV_ATTACH_TO_ADDON ? true : false),
      "Invalid use of axis_env %p.", self);

  switch (self->attach_to) {
    case axis_ENV_ATTACH_TO_EXTENSION:
      return axis_extension_on_init_done(self);

    case axis_ENV_ATTACH_TO_EXTENSION_GROUP:
      axis_extension_group_on_init_done(self);
      break;

    case axis_ENV_ATTACH_TO_APP:
      axis_app_on_init_done(self);
      break;

    case axis_ENV_ATTACH_TO_ADDON:
      axis_addon_on_init_done(self);
      break;

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  return true;
}

bool axis_env_on_deinit_done(axis_env_t *self, axis_UNUSED axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(
      axis_env_check_integrity(
          self, self->attach_to != axis_ENV_ATTACH_TO_ADDON ? true : false),
      "Invalid use of axis_env %p.", self);

  switch (self->attach_to) {
    case axis_ENV_ATTACH_TO_ADDON:
      axis_addon_on_deinit_done(self);
      break;

    case axis_ENV_ATTACH_TO_EXTENSION_GROUP:
      axis_extension_group_on_deinit_done(self);
      break;

    case axis_ENV_ATTACH_TO_EXTENSION:
      return axis_extension_on_deinit_done(self);

    case axis_ENV_ATTACH_TO_APP:
      axis_app_on_deinit_done(self);
      break;

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  return true;
}

bool axis_env_on_create_extensions_done(
    axis_env_t *self, axis_extension_group_create_extensions_done_ctx_t *ctx,
    axis_UNUSED axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(self, true), "Invalid use of axis_env %p.",
             self);
  axis_ASSERT(self->attach_to == axis_ENV_ATTACH_TO_EXTENSION_GROUP,
             "Should not happen.");

  axis_extension_group_t *extension_group =
      axis_env_get_attached_extension_group(self);
  axis_ASSERT(extension_group &&
                 axis_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  axis_extension_group_on_create_extensions_done(extension_group, &ctx->results);

  return true;
}

bool axis_env_on_destroy_extensions_done(axis_env_t *self,
                                        axis_UNUSED axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(self, true), "Invalid use of axis_env %p.",
             self);
  axis_ASSERT(self->attach_to == axis_ENV_ATTACH_TO_EXTENSION_GROUP,
             "Should not happen.");

  axis_extension_group_t *extension_group =
      axis_env_get_attached_extension_group(self);
  axis_ASSERT(extension_group &&
                 axis_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  axis_extension_group_on_destroy_extensions_done(extension_group);

  return true;
}

bool axis_env_on_create_instance_done(axis_env_t *self, void *instance,
                                     void *context,
                                     axis_UNUSED axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(
      axis_env_check_integrity(
          self, self->attach_to != axis_ENV_ATTACH_TO_ADDON ? true : false),
      "Invalid use of axis_env %p.", self);

  switch (self->attach_to) {
    case axis_ENV_ATTACH_TO_ADDON:
      axis_addon_on_create_instance_done(self, instance, context);
      break;

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  return true;
}

bool axis_env_on_destroy_instance_done(axis_env_t *self, void *context,
                                      axis_UNUSED axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(
      axis_env_check_integrity(
          self, self->attach_to != axis_ENV_ATTACH_TO_ADDON ? true : false),
      "Invalid use of axis_env %p.", self);

  switch (self->attach_to) {
    case axis_ENV_ATTACH_TO_ADDON:
      axis_addon_on_destroy_instance_done(self, context);
      break;

    case axis_ENV_ATTACH_TO_ENGINE:
      axis_extension_context_on_addon_destroy_extension_group_done(self,
                                                                  context);
      break;

    case axis_ENV_ATTACH_TO_EXTENSION_GROUP:
      axis_extension_group_on_addon_destroy_extension_done(self, context);
      break;

    default:
      axis_ASSERT(0, "Should not happen: %d.", self->attach_to);
      break;
  }

  return true;
}

bool axis_env_on_start_done(axis_env_t *self, axis_UNUSED axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(self, true), "Invalid use of axis_env %p.",
             self);
  axis_ASSERT(self->attach_to == axis_ENV_ATTACH_TO_EXTENSION,
             "Should not happen.");

  return axis_extension_on_start_done(self);
}

bool axis_env_on_stop_done(axis_env_t *self, axis_UNUSED axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(self, true), "Invalid use of axis_env %p.",
             self);
  axis_ASSERT(self->attach_to == axis_ENV_ATTACH_TO_EXTENSION,
             "Should not happen.");

  return axis_extension_on_stop_done(self);
}
