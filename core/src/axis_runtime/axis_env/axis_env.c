//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/axis_env/axis_env.h"

#include <stdlib.h>

#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/addon/addon_host.h"
#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/common/loc.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension_context/extension_context.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/extension_thread/extension_thread.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_runtime/app/app.h"
#include "axis_runtime/binding/common.h"
#include "axis_runtime/extension/extension.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/sanitizer/thread_check.h"

bool axis_env_check_integrity(axis_env_t *self, bool check_thread) {
  axis_ASSERT(self, "Invalid argument.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_ENV_SIGNATURE) {
    return false;
  }

  if (check_thread) {
    // Utilize the check_integrity of extension_thread to examine cases
    // involving the lock_mode of extension_thread.
    axis_extension_thread_t *extension_thread = NULL;
    switch (self->attach_to) {
      case axis_ENV_ATTACH_TO_EXTENSION:
        extension_thread = self->attached_target.extension->extension_thread;
        break;
      case axis_ENV_ATTACH_TO_EXTENSION_GROUP:
        extension_thread =
            self->attached_target.extension_group->extension_thread;
        break;
      default:
        break;
    }

    if (extension_thread) {
      if (axis_extension_thread_check_integrity_if_in_lock_mode(
              extension_thread)) {
        return true;
      }
    }

    return axis_sanitizer_thread_check_do_check(&self->thread_check);
  }

  return true;
}

axis_env_t *axis_env_create(void) {
  axis_env_t *self = (axis_env_t *)axis_MALLOC(sizeof(axis_env_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_signature_set(&self->signature, (axis_signature_t)axis_ENV_SIGNATURE);
  axis_sanitizer_thread_check_init_with_current_thread(&self->thread_check);

  self->binding_handle.me_in_target_lang = NULL;
  self->close_handler = NULL;
  self->destroy_handler = NULL;
  axis_list_init(&self->axis_proxy_list);

  self->attach_to = axis_ENV_ATTACH_TO_INVALID;
  self->attached_target.addon_host = NULL;

  return self;
}

static axis_env_t *axis_create_with_attach_to(axis_ENV_ATTACH_TO attach_to_type,
                                            void *attach_to) {
  axis_ASSERT(attach_to, "Should not happen.");
  axis_ASSERT(attach_to_type != axis_ENV_ATTACH_TO_INVALID, "Should not happen.");

  axis_env_t *self = axis_env_create();
  axis_ASSERT(self, "Should not happen.");

  axis_env_set_attach_to(self, attach_to_type, attach_to);

  return self;
}

axis_env_t *axis_env_create_for_addon(axis_addon_host_t *addon_host) {
  axis_ASSERT(addon_host, "Invalid argument.");
  axis_ASSERT(axis_addon_host_check_integrity(addon_host),
             "Invalid use of addon_host %p.", addon_host);

  return axis_create_with_attach_to(axis_ENV_ATTACH_TO_ADDON, addon_host);
}

axis_env_t *axis_env_create_for_extension(axis_extension_t *extension) {
  axis_ASSERT(extension, "Invalid argument.");
  axis_ASSERT(axis_extension_check_integrity(extension, true),
             "Invalid use of extension %p.", extension);

  return axis_create_with_attach_to(axis_ENV_ATTACH_TO_EXTENSION, extension);
}

axis_env_t *axis_env_create_for_extension_group(
    axis_extension_group_t *extension_group) {
  axis_ASSERT(extension_group &&
                 axis_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  return axis_create_with_attach_to(axis_ENV_ATTACH_TO_EXTENSION_GROUP,
                                   extension_group);
}

axis_env_t *axis_env_create_for_app(axis_app_t *app) {
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: this function would be called before the app thread started,
  // so it's thread safe.
  axis_ASSERT(app && axis_app_check_integrity(app, false), "Should not happen.");

  return axis_create_with_attach_to(axis_ENV_ATTACH_TO_APP, app);
}

axis_env_t *axis_env_create_for_engine(axis_engine_t *engine) {
  axis_ASSERT(engine && axis_engine_check_integrity(engine, true),
             "Should not happen.");

  return axis_create_with_attach_to(axis_ENV_ATTACH_TO_ENGINE, engine);
}

void axis_env_destroy(axis_env_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: this function would be called after the app thread is
  // destroyed.
  axis_ASSERT(axis_env_check_integrity(self, false), "Invalid use of axis_env %p.",
             self);

  if (self->destroy_handler && self->binding_handle.me_in_target_lang) {
    self->destroy_handler(self->binding_handle.me_in_target_lang);
  }

  axis_ASSERT(axis_list_is_empty(&self->axis_proxy_list), "Should not happen.");

  axis_sanitizer_thread_check_deinit(&self->thread_check);
  axis_signature_set(&self->signature, 0);

  axis_FREE(self);
}

void axis_env_close(axis_env_t *self) {
  axis_ASSERT(self && axis_env_check_integrity(self, true), "Should not happen.");

  switch (self->attach_to) {
    case axis_ENV_ATTACH_TO_APP:
      axis_LOGD("[%s] Close aptima of app.",
               axis_app_get_uri(self->attached_target.app));
      break;
    case axis_ENV_ATTACH_TO_ENGINE:
      axis_LOGD("[%s] Close aptima of engine.",
               axis_engine_get_id(self->attached_target.engine, true));
      break;
    case axis_ENV_ATTACH_TO_EXTENSION_GROUP:
      axis_LOGD(
          "[%s] Close aptima of extension group.",
          axis_string_get_raw_str(&self->attached_target.extension_group->name));
      break;
    case axis_ENV_ATTACH_TO_EXTENSION:
      axis_LOGD("[%s] Close aptima of extension.",
               axis_string_get_raw_str(&self->attached_target.extension->name));
      break;
    case axis_ENV_ATTACH_TO_ADDON:
      axis_LOGD("[%s] Close aptima of addon.",
               axis_string_get_raw_str(&self->attached_target.addon_host->name));
      break;
    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  if (self->close_handler && self->binding_handle.me_in_target_lang) {
    self->close_handler(self->binding_handle.me_in_target_lang);
  }
}

void axis_env_set_close_handler_in_target_lang(
    axis_env_t *self,
    axis_env_close_handler_in_target_lang_func_t close_handler) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  axis_ASSERT(axis_env_check_integrity(self, false), "Invalid use of axis_env %p.",
             self);

  self->close_handler = close_handler;
}

void axis_env_set_destroy_handler_in_target_lang(
    axis_env_t *self,
    axis_env_destroy_handler_in_target_lang_func_t destroy_handler) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  axis_ASSERT(axis_env_check_integrity(self, false), "Invalid use of axis_env %p.",
             self);

  self->destroy_handler = destroy_handler;
}

axis_runloop_t *axis_env_get_attached_runloop(axis_env_t *self) {
  axis_ASSERT(self && axis_env_check_integrity(self, false),
             "Should not happen.");

  switch (self->attach_to) {
    case axis_ENV_ATTACH_TO_APP:
      return axis_app_get_attached_runloop(axis_env_get_attached_app(self));
    case axis_ENV_ATTACH_TO_EXTENSION_GROUP:
      return axis_extension_group_get_attached_runloop(
          axis_env_get_attached_extension_group(self));
    case axis_ENV_ATTACH_TO_EXTENSION:
      return axis_extension_get_attached_runloop(
          axis_env_get_attached_extension(self));
    case axis_ENV_ATTACH_TO_ENGINE:
      return axis_engine_get_attached_runloop(axis_env_get_attached_engine(self));
    default:
      axis_ASSERT(0, "Handle more types: %d", self->attach_to);
      break;
  }

  return NULL;
}

void *axis_env_get_attached_target(axis_env_t *self) {
  axis_ASSERT(self && axis_env_check_integrity(self, true), "Should not happen.");

  switch (self->attach_to) {
    case axis_ENV_ATTACH_TO_EXTENSION:
      return axis_env_get_attached_extension(self);
    case axis_ENV_ATTACH_TO_EXTENSION_GROUP:
      return axis_env_get_attached_extension_group(self);
    case axis_ENV_ATTACH_TO_ENGINE:
      return axis_env_get_attached_engine(self);
    case axis_ENV_ATTACH_TO_APP:
      return axis_env_get_attached_app(self);
    case axis_ENV_ATTACH_TO_ADDON:
      return axis_env_get_attached_addon(self);
    default:
      axis_ASSERT(0, "Handle more types: %d", self->attach_to);
      return NULL;
  }
}

axis_ENV_ATTACH_TO axis_env_get_attach_to(axis_env_t *self) {
  axis_ASSERT(self && axis_env_check_integrity(self, true), "Should not happen.");
  return self->attach_to;
}

void axis_env_set_attach_to(axis_env_t *self, axis_ENV_ATTACH_TO attach_to_type,
                           void *attach_to) {
  axis_ASSERT(self, "Should not happen.");
  axis_ASSERT(axis_env_check_integrity(self, true), "Should not happen.");

  self->attach_to = attach_to_type;
  switch (attach_to_type) {
    case axis_ENV_ATTACH_TO_EXTENSION:
      self->attached_target.extension = attach_to;
      break;

    case axis_ENV_ATTACH_TO_EXTENSION_GROUP:
      self->attached_target.extension_group = attach_to;
      break;

    case axis_ENV_ATTACH_TO_APP:
      self->attached_target.app = attach_to;
      break;

    case axis_ENV_ATTACH_TO_ADDON:
      self->attached_target.addon_host = attach_to;
      break;

    case axis_ENV_ATTACH_TO_ENGINE:
      self->attached_target.engine = attach_to;
      break;

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }
}

axis_app_t *axis_env_get_belonging_app(axis_env_t *self) {
  axis_ASSERT(self && axis_env_check_integrity(self, true), "Should not happen.");

  switch (self->attach_to) {
    case axis_ENV_ATTACH_TO_APP:
      return axis_env_get_attached_app(self);
    case axis_ENV_ATTACH_TO_EXTENSION: {
      axis_extension_t *extension = axis_env_get_attached_extension(self);
      axis_ASSERT(extension && axis_extension_check_integrity(extension, true),
                 "Should not happen.");

      axis_extension_thread_t *extension_thread = extension->extension_thread;
      axis_ASSERT(extension_thread && axis_extension_thread_check_integrity(
                                         extension_thread, true),
                 "Should not happen.");

      axis_extension_context_t *extension_context =
          extension_thread->extension_context;
      axis_ASSERT(extension_context && axis_extension_context_check_integrity(
                                          extension_context, false),
                 "Should not happen.");

      axis_engine_t *engine = extension_context->engine;
      axis_ASSERT(engine && axis_engine_check_integrity(engine, false),
                 "Should not happen.");

      axis_app_t *app = engine->app;
      axis_ASSERT(app && axis_app_check_integrity(app, false),
                 "Should not happen.");

      return app;
    }
    case axis_ENV_ATTACH_TO_EXTENSION_GROUP: {
      axis_extension_group_t *extension_group =
          axis_env_get_attached_extension_group(self);
      axis_ASSERT(extension_group &&
                     axis_extension_group_check_integrity(extension_group, true),
                 "Should not happen.");

      axis_extension_thread_t *extension_thread =
          extension_group->extension_thread;
      axis_ASSERT(extension_thread && axis_extension_thread_check_integrity(
                                         extension_thread, true),
                 "Should not happen.");

      axis_extension_context_t *extension_context =
          extension_thread->extension_context;
      axis_ASSERT(extension_context && axis_extension_context_check_integrity(
                                          extension_context, false),
                 "Should not happen.");

      axis_engine_t *engine = extension_context->engine;
      axis_ASSERT(engine && axis_engine_check_integrity(engine, false),
                 "Should not happen.");

      axis_app_t *app = engine->app;
      axis_ASSERT(app && axis_app_check_integrity(app, false),
                 "Should not happen.");

      return app;
    }

    case axis_ENV_ATTACH_TO_ENGINE: {
      axis_engine_t *engine = axis_env_get_attached_engine(self);
      axis_ASSERT(engine && axis_engine_check_integrity(engine, true),
                 "Should not happen.");

      axis_app_t *app = engine->app;
      axis_ASSERT(app && axis_app_check_integrity(app, false),
                 "Should not happen.");

      return app;
    }

    default:
      axis_ASSERT(0, "Handle more types: %d", self->attach_to);
      return NULL;
  }
}
