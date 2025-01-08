//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/app/close.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension_context/extension_context.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "include_internal/axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_runtime/app/app.h"
#include "axis_runtime/extension/extension.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/container/list_ptr.h"
#include "axis_utils/macro/check.h"

void axis_env_add_axis_proxy(axis_env_t *self, axis_env_proxy_t *axis_env_proxy) {
  axis_ASSERT(self && axis_env_check_integrity(self, true), "Invalid argument.");
  axis_ASSERT(axis_env_proxy && axis_env_proxy_check_integrity(axis_env_proxy),
             "Invalid argument.");

  axis_list_push_ptr_back(&self->axis_proxy_list, axis_env_proxy, NULL);
}

void axis_env_delete_axis_proxy(axis_env_t *self, axis_env_proxy_t *axis_env_proxy) {
  axis_ASSERT(self && axis_env_check_integrity(self, true), "Invalid argument.");
  axis_ASSERT(axis_env_proxy && axis_env_proxy_check_integrity(axis_env_proxy),
             "Invalid argument.");

  bool found = axis_list_remove_ptr(&self->axis_proxy_list, axis_env_proxy);
  axis_ASSERT(found, "Should not happen.");

  if (axis_list_is_empty(&self->axis_proxy_list)) {
    switch (self->attach_to) {
      case axis_ENV_ATTACH_TO_EXTENSION: {
        axis_extension_t *extension = self->attached_target.extension;
        axis_ASSERT(extension && axis_extension_check_integrity(extension, true),
                   "Should not happen.");

        if (extension->state == axis_EXTENSION_STATE_ON_DEINIT) {
          axis_env_on_deinit_done(self, NULL);
        }
        break;
      }

      case axis_ENV_ATTACH_TO_EXTENSION_GROUP: {
        axis_extension_group_t *extension_group =
            self->attached_target.extension_group;
        axis_ASSERT(extension_group && axis_extension_group_check_integrity(
                                          extension_group, true),
                   "Should not happen.");

        if (extension_group->state == axis_EXTENSION_GROUP_STATE_DEINITING) {
          axis_env_on_deinit_done(self, NULL);
        }
        break;
      }

      case axis_ENV_ATTACH_TO_APP: {
        axis_app_t *app = self->attached_target.app;
        axis_ASSERT(app && axis_app_check_integrity(app, true),
                   "Should not happen.");

        if (axis_app_is_closing(app)) {
          axis_env_on_deinit_done(self, NULL);
        }
        break;
      }

      default:
        axis_ASSERT(0, "Handle more types: %d", self->attach_to);
        break;
    }
  }
}
