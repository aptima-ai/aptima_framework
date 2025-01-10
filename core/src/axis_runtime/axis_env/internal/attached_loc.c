//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/addon/addon_host.h"
#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/extension_group/on_xxx.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/axis_env/axis_env.h"

const char *axis_env_get_attached_instance_name(axis_env_t *self,
                                               bool check_thread) {
  axis_ASSERT(self && axis_env_check_integrity(self, check_thread),
             "Invalid argument.");

  switch (self->attach_to) {
    case axis_ENV_ATTACH_TO_EXTENSION: {
      axis_extension_t *extension = axis_env_get_attached_extension(self);
      return axis_extension_get_name(extension, true);
    }
    case axis_ENV_ATTACH_TO_EXTENSION_GROUP: {
      axis_extension_group_t *extension_group =
          axis_env_get_attached_extension_group(self);
      return axis_extension_group_get_name(extension_group, true);
    }
    case axis_ENV_ATTACH_TO_ENGINE: {
      axis_engine_t *engine = axis_env_get_attached_engine(self);
      return axis_engine_get_id(engine, true);
    }
    case axis_ENV_ATTACH_TO_APP: {
      axis_app_t *app = axis_env_get_attached_app(self);
      return axis_app_get_uri(app);
    }
    case axis_ENV_ATTACH_TO_ADDON: {
      axis_addon_host_t *addon_host = axis_env_get_attached_addon(self);
      return axis_addon_host_get_name(addon_host);
    }
    default:
      axis_ASSERT(0, "Handle more types: %d", self->attach_to);
      return NULL;
  }
}
