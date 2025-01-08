//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/app/axis_env/metadata.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension/metadata.h"
#include "include_internal/axis_runtime/extension/axis_env/metadata.h"
#include "include_internal/axis_runtime/extension_context/extension_context.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/extension_group/axis_env/metadata.h"
#include "include_internal/axis_runtime/extension_thread/extension_thread.h"
#include "include_internal/axis_runtime/axis_env/metadata.h"
#include "include_internal/axis_runtime/axis_env/metadata_cb.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/app/app.h"
#include "axis_runtime/common/errno.h"
#include "axis_runtime/axis_env/internal/metadata.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

static axis_env_set_property_sync_context_t *
axis_env_set_property_sync_context_create(axis_error_t *err) {
  axis_ASSERT(err && axis_error_check_integrity(err), "Invalid argument.");

  axis_env_set_property_sync_context_t *context =
      axis_MALLOC(sizeof(axis_env_set_property_sync_context_t));
  axis_ASSERT(context, "Failed to allocate memory.");

  context->err = err;
  context->completed = axis_event_create(0, 0);

  return context;
}

static void axis_env_set_property_sync_context_destroy(
    axis_env_set_property_sync_context_t *self) {
  axis_ASSERT(self, "Should not happen.");

  self->err = NULL;
  axis_event_destroy(self->completed);
  axis_FREE(self);
}

static void axis_app_set_property_sync_cb(axis_app_t *app, axis_error_t *err,
                                         void *cb_data) {
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Should not happen.");
  axis_ASSERT(err && axis_error_check_integrity(err), "Invalid argument.");

  axis_env_set_property_sync_context_t *context = cb_data;
  axis_ASSERT(context, "Should not happen.");

  axis_error_set(context->err, axis_error_errno(err), axis_error_errmsg(err));
  axis_event_set(context->completed);
}

static axis_env_set_property_async_context_t *
axis_env_set_property_async_context_create(axis_env_t *axis_env,
                                          axis_env_set_property_async_cb_t cb,
                                          void *cb_data) {
  axis_env_set_property_async_context_t *context =
      axis_MALLOC(sizeof(axis_env_set_property_async_context_t));
  axis_ASSERT(context, "Failed to allocate memory.");

  context->axis_env = axis_env;
  context->cb = cb;
  context->cb_data = cb_data;

  context->from.extension = NULL;
  context->from.extension_group = NULL;

  return context;
}

static void axis_env_set_property_async_context_destroy(
    axis_env_set_property_async_context_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_FREE(self);
}

static void axis_extension_set_property_async_cb(axis_extension_t *extension,
                                                bool res, void *cb_data,
                                                axis_error_t *err) {
  axis_ASSERT(extension && axis_extension_check_integrity(extension, true),
             "Should not happen.");

  axis_env_set_property_async_context_t *context = cb_data;
  axis_ASSERT(context, "Should not happen.");

  if (context->cb) {
    context->cb(context->axis_env, res, context->cb_data, err);
  }

  axis_env_set_property_async_context_destroy(context);
}

static void axis_extension_group_set_property_async_cb(
    axis_extension_group_t *extension_group, bool res, void *cb_data) {
  axis_ASSERT(extension_group &&
                 axis_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  axis_env_set_property_async_context_t *context = cb_data;
  axis_ASSERT(context, "Should not happen.");

  if (context->cb) {
    context->cb(context->axis_env, res, context->cb_data, NULL);
  }

  axis_env_set_property_async_context_destroy(context);
}

static void axis_env_set_property_done_task(axis_UNUSED void *from, void *arg) {
  axis_env_set_property_async_context_t *context = arg;
  axis_ASSERT(context, "Should not happen.");
  axis_ASSERT(context->from.extension != NULL, "Invalid argument.");

  if (context->cb) {
    context->cb(context->axis_env, context->res, context->cb_data, NULL);
  }

  axis_env_set_property_async_context_destroy(context);
}

static void axis_app_set_property_async_cb_go_back_to_extension(axis_app_t *app,
                                                               axis_error_t *err,
                                                               void *cb_data) {
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Should not happen.");
  axis_ASSERT(err && axis_error_check_integrity(err), "Invalid argument.");

  axis_env_set_property_async_context_t *context = cb_data;
  axis_ASSERT(context, "Should not happen.");
  axis_ASSERT(context->from.extension != NULL, "Invalid argument.");

  context->res = axis_error_is_success(err);

  axis_runloop_post_task_tail(
      axis_extension_get_attached_runloop(context->from.extension),
      axis_env_set_property_done_task, NULL, context);
}

static void axis_app_set_property_async_cb_go_back_to_extension_group(
    axis_app_t *app, axis_error_t *err, void *cb_data) {
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Should not happen.");
  axis_ASSERT(err && axis_error_check_integrity(err), "Invalid argument.");

  axis_env_set_property_async_context_t *context = cb_data;
  axis_ASSERT(context, "Should not happen.");
  axis_ASSERT(context->from.extension_group != NULL, "Invalid argument.");

  context->res = axis_error_is_success(err);

  axis_runloop_post_task_tail(
      axis_extension_group_get_attached_runloop(context->from.extension_group),
      axis_env_set_property_done_task, NULL, context);
}

static void axis_app_set_property_async_cb(axis_app_t *app, axis_error_t *err,
                                          void *cb_data) {
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Should not happen.");
  axis_ASSERT(err && axis_error_check_integrity(err), "Invalid argument.");

  axis_env_set_property_async_context_t *context = cb_data;
  axis_ASSERT(context, "Should not happen.");

  if (context->cb) {
    context->cb(context->axis_env, axis_error_is_success(err), context->cb_data,
                err);
  }

  axis_env_set_property_async_context_destroy(context);
}

bool axis_env_set_property(axis_env_t *self, const char *path, axis_value_t *value,
                          axis_error_t *err) {
  axis_ASSERT(self && axis_env_check_integrity(self, true),
             "Invalid use of axis_env %p.", self);
  axis_ASSERT(path && strlen(path), "path should not be empty.");

  bool result = true;

  if (!path || !strlen(path)) {
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT,
                    "path should not be empty.");
    }
    result = false;
    goto done;
  }

  if (!value) {
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT,
                    "value should not be empty.");
    }
    result = false;
    goto done;
  }

  const char **p_path = &path;

  axis_METADATA_LEVEL level =
      axis_determine_metadata_level(self->attach_to, p_path);

  switch (self->attach_to) {
    case axis_ENV_ATTACH_TO_EXTENSION: {
      axis_extension_t *extension = axis_env_get_attached_extension(self);
      axis_ASSERT(extension && axis_extension_check_integrity(extension, true),
                 "Invalid use of extension %p.", extension);

      axis_extension_thread_t *extension_thread = extension->extension_thread;
      axis_ASSERT(extension_thread && axis_extension_thread_check_integrity(
                                         extension_thread, true),
                 "Invalid use of extension_thread %p.", extension_thread);

      switch (level) {
        case axis_METADATA_LEVEL_EXTENSION:
          result = axis_extension_set_property(extension, path, value, err);
          break;

        case axis_METADATA_LEVEL_EXTENSION_GROUP: {
          if (true) {
            result = false;
            if (err) {
              axis_error_set(
                  err, axis_ERRNO_GENERIC,
                  "Setting properties in higher-level scopes is not allowed. "
                  "Properties can only be set within the current scope.");
            }
            axis_ASSERT(0, "Should not happen.");
          } else {
            axis_extension_group_t *extension_group =
                extension->extension_thread->extension_group;
            axis_ASSERT(extension_group && axis_extension_group_check_integrity(
                                              extension_group, true),
                       "Invalid use of extension group %p", extension_group);

            result =
                axis_extension_group_set_property(extension_group, path, value);
          }
          break;
        }

        case axis_METADATA_LEVEL_APP: {
          if (true) {
            result = false;
            if (err) {
              axis_error_set(
                  err, axis_ERRNO_GENERIC,
                  "Setting properties in higher-level scopes is not allowed. "
                  "Properties can only be set within the current scope.");
            }
            axis_ASSERT(0, "Should not happen.");
          } else {
            axis_app_t *app = extension->extension_context->engine->app;
            // axis_NOLINTNEXTLINE(thread-check):
            // thread-check: Access the app's property from an extension, that
            // is, from the extension thread.
            axis_ASSERT(app && axis_app_check_integrity(app, false),
                       "Invalid use of app %p", app);

            if (axis_app_thread_call_by_me(app)) {
              result = axis_app_set_property(app, path, value, err);
            } else {
              bool new_error_created = false;
              if (!err) {
                new_error_created = true;
                err = axis_error_create();
              }

              axis_env_set_property_sync_context_t *context =
                  axis_env_set_property_sync_context_create(err);
              axis_ASSERT(context, "Should not happen.");

              axis_app_set_property_async(app, path, value,
                                         axis_app_set_property_sync_cb, context);

              axis_event_wait(context->completed, -1);
              result = axis_error_is_success(err);

              axis_env_set_property_sync_context_destroy(context);

              if (new_error_created) {
                axis_error_destroy(err);
                err = NULL;
              }
            }
          }
          break;
        }

        default:
          axis_ASSERT(0, "Should not happen.");
          break;
      }
      break;
    }

    case axis_ENV_ATTACH_TO_EXTENSION_GROUP: {
      axis_extension_group_t *extension_group =
          axis_env_get_attached_extension_group(self);
      axis_ASSERT(extension_group &&
                     axis_extension_group_check_integrity(extension_group, true),
                 "Invalid use of extension_group %p.", extension_group);

      axis_extension_thread_t *extension_thread =
          extension_group->extension_thread;
      axis_ASSERT(extension_thread && axis_extension_thread_check_integrity(
                                         extension_thread, true),
                 "Invalid use of extension_thread %p.", extension_thread);

      switch (level) {
        case axis_METADATA_LEVEL_EXTENSION_GROUP:
          if (true) {
            result = false;
            if (err) {
              axis_error_set(
                  err, axis_ERRNO_GENERIC,
                  "The set property of extension group is currently not "
                  "supported; use init_property_from_json instead.");
            }
            axis_ASSERT(0, "Should not happen.");
          } else {
            result =
                axis_extension_group_set_property(extension_group, path, value);
          }
          break;

        case axis_METADATA_LEVEL_APP: {
          if (true) {
            result = false;
            if (err) {
              axis_error_set(
                  err, axis_ERRNO_GENERIC,
                  "Setting properties in higher-level scopes is not allowed. "
                  "Properties can only be set within the current scope.");
            }
            axis_ASSERT(0, "Should not happen.");
          } else {
            axis_app_t *app = extension_group->extension_context->engine->app;
            // axis_NOLINTNEXTLINE(thread-check):
            // thread-check: Access the app's property from an extension group,
            // that is, from the extension thread.
            axis_ASSERT(app && axis_app_check_integrity(app, false),
                       "Invalid use of app %p", app);

            if (axis_app_thread_call_by_me(app)) {
              result = axis_app_set_property(app, path, value, err);
            } else {
              bool new_error_created = false;
              if (!err) {
                new_error_created = true;
                err = axis_error_create();
              }

              axis_env_set_property_sync_context_t *context =
                  axis_env_set_property_sync_context_create(err);
              axis_ASSERT(context, "Should not happen.");

              axis_app_set_property_async(app, path, value,
                                         axis_app_set_property_sync_cb, context);

              axis_event_wait(context->completed, -1);
              result = axis_error_is_success(err);

              axis_env_set_property_sync_context_destroy(context);

              if (new_error_created) {
                axis_error_destroy(err);
                err = NULL;
              }
            }
          }
          break;
        }

        default:
          axis_ASSERT(0, "Should not happen.");
          break;
      }
      break;
    }

    case axis_ENV_ATTACH_TO_APP: {
      axis_app_t *app = axis_env_get_attached_app(self);
      axis_ASSERT(app && axis_app_check_integrity(app, true),
                 "Invalid use of app %p.", app);

      switch (level) {
        case axis_METADATA_LEVEL_APP: {
          if (axis_app_thread_call_by_me(app)) {
            result = axis_app_set_property(app, path, value, err);
          } else {
            axis_ASSERT(0, "Do we really need this?");
          }
          break;
        }

        default:
          axis_ASSERT(0, "Should not happen.");
          break;
      }
      break;
    }

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

done:
  if (!result) {
    if (err) {
      axis_LOGW("Failed to set property: %s.", axis_error_errmsg(err));
    } else {
      axis_LOGW("Failed to set property.");
    }
  }

  return result;
}

bool axis_env_set_property_async(axis_env_t *self, const char *path,
                                axis_value_t *value,
                                axis_env_set_property_async_cb_t cb,
                                void *cb_data, axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(self, true), "Invalid use of axis_env %p.",
             self);
  axis_ASSERT(path && strlen(path), "path should not be empty.");

  if (!path || !strlen(path)) {
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT,
                    "path should not be empty.");
    }
    return false;
  }

  if (!value) {
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT,
                    "value should not be empty.");
    }
    return false;
  }

  const char **p_path = &path;

  axis_env_set_property_async_context_t *context =
      axis_env_set_property_async_context_create(self, cb, cb_data);
  axis_ASSERT(context, "Should not happen.");

  bool result = true;

  axis_METADATA_LEVEL level =
      axis_determine_metadata_level(self->attach_to, p_path);

  switch (self->attach_to) {
    case axis_ENV_ATTACH_TO_EXTENSION: {
      axis_extension_t *extension = axis_env_get_attached_extension(self);
      axis_ASSERT(extension && axis_extension_check_integrity(extension, true),
                 "Invalid use of extension %p.", extension);

      axis_extension_thread_t *extension_thread = extension->extension_thread;
      axis_ASSERT(extension_thread && axis_extension_thread_check_integrity(
                                         extension_thread, true),
                 "Invalid use of extension_thread %p.", extension_thread);

      switch (level) {
        case axis_METADATA_LEVEL_EXTENSION:
          axis_extension_set_property_async(extension, path, value,
                                           axis_extension_set_property_async_cb,
                                           context, err);
          break;

        case axis_METADATA_LEVEL_EXTENSION_GROUP: {
          if (true) {
            result = false;
            if (err) {
              axis_error_set(
                  err, axis_ERRNO_GENERIC,
                  "Setting properties in higher-level scopes is not allowed. "
                  "Properties can only be set within the current scope.");
            }
            axis_ASSERT(0, "Should not happen.");
          } else {
            axis_extension_group_t *extension_group =
                extension->extension_thread->extension_group;
            axis_ASSERT(extension_group && axis_extension_group_check_integrity(
                                              extension_group, true),
                       "Invalid use of extension group %p", extension_group);

            axis_extension_group_set_property_async(
                extension_group, path, value,
                axis_extension_group_set_property_async_cb, context);
          }
          break;
        }

        case axis_METADATA_LEVEL_APP: {
          if (true) {
            result = false;
            if (err) {
              axis_error_set(
                  err, axis_ERRNO_GENERIC,
                  "Setting properties in higher-level scopes is not allowed. "
                  "Properties can only be set within the current scope.");
            }
            axis_ASSERT(0, "Should not happen.");
          } else {
            axis_app_t *app = extension->extension_context->engine->app;
            // axis_NOLINTNEXTLINE(thread-check):
            // thread-check: Access the app's property from an extension, that
            // is, from the extension thread.
            axis_ASSERT(app && axis_app_check_integrity(app, false),
                       "Invalid use of app %p", app);

            context->from.extension = extension;

            axis_app_set_property_async(
                app, path, value,
                axis_app_set_property_async_cb_go_back_to_extension, context);
          }
          break;
        }

        default:
          axis_ASSERT(0, "Should not happen.");
          break;
      }
      break;
    }

    case axis_ENV_ATTACH_TO_EXTENSION_GROUP: {
      axis_extension_group_t *extension_group =
          axis_env_get_attached_extension_group(self);
      axis_ASSERT(extension_group &&
                     axis_extension_group_check_integrity(extension_group, true),
                 "Invalid use of extension_group %p.", extension_group);

      axis_extension_thread_t *extension_thread =
          extension_group->extension_thread;
      axis_ASSERT(extension_thread && axis_extension_thread_check_integrity(
                                         extension_thread, true),
                 "Invalid use of extension_thread %p.", extension_thread);

      switch (level) {
        case axis_METADATA_LEVEL_EXTENSION_GROUP:
          if (true) {
            result = false;
            if (err) {
              axis_error_set(
                  err, axis_ERRNO_GENERIC,
                  "The set property of extension group is currently not "
                  "supported; use init_property_from_json instead.");
            }
            axis_ASSERT(0, "Should not happen.");
          } else {
            axis_extension_group_set_property_async(
                extension_group, path, value,
                axis_extension_group_set_property_async_cb, context);
          }
          break;

        case axis_METADATA_LEVEL_APP: {
          if (true) {
            result = false;
            if (err) {
              axis_error_set(
                  err, axis_ERRNO_GENERIC,
                  "Setting properties in higher-level scopes is not allowed. "
                  "Properties can only be set within the current scope.");
            }
            axis_ASSERT(0, "Should not happen.");
          } else {
            axis_app_t *app = extension_group->extension_context->engine->app;
            // axis_NOLINTNEXTLINE(thread-check):
            // thread-check: Access the app's property from an extension group,
            // that is, from the extension thread.
            axis_ASSERT(app && axis_app_check_integrity(app, false),
                       "Invalid use of app %p", app);

            context->from.extension_group = extension_group;

            axis_app_set_property_async(
                app, path, value,
                axis_app_set_property_async_cb_go_back_to_extension_group,
                context);
          }
          break;
        }

        default:
          axis_ASSERT(0, "Should not happen.");
          break;
      }
      break;
    }

    case axis_ENV_ATTACH_TO_APP: {
      axis_app_t *app = axis_env_get_attached_app(self);
      axis_ASSERT(app && axis_app_check_integrity(app, true),
                 "Invalid use of app %p.", app);

      switch (level) {
        case axis_METADATA_LEVEL_APP:
          if (true) {
            result = false;
            if (err) {
              axis_error_set(err, axis_ERRNO_GENERIC,
                            "The set property of app is currently not "
                            "supported; use init_property_from_json instead.");
            }
            axis_ASSERT(0, "Should not happen.");
          } else {
            axis_app_set_property_async(app, path, value,
                                       axis_app_set_property_async_cb, context);
          }
          break;

        default:
          axis_ASSERT(0, "Should not happen.");
          break;
      }
      break;
    }

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  return result;
}
