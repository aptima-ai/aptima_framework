//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/extension_thread/extension_thread.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "include_internal/axis_runtime/axis_env/axis_env_proxy.h"
#include "include_internal/axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_runtime/extension/extension.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/thread.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"

static axis_notify_data_t *axis_notify_data_create(
    axis_env_proxy_notify_func_t notify_func, void *user_data) {
  axis_ASSERT(notify_func, "Invalid argument.");

  axis_notify_data_t *self =
      (axis_notify_data_t *)axis_MALLOC(sizeof(axis_notify_data_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  self->notify_func = notify_func;
  self->user_data = user_data;

  return self;
}

static void axis_notify_data_destroy(axis_notify_data_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_FREE(self);
}

static void axis_notify_to_app_task(void *self_, void *arg) {
  axis_app_t *app = self_;
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Invalid argument.");

  axis_notify_data_t *notify_data = (axis_notify_data_t *)arg;
  axis_ASSERT(notify_data, "Invalid argument.");

  notify_data->notify_func(app->axis_env, notify_data->user_data);

  axis_notify_data_destroy(notify_data);
}

static void axis_notify_to_extension_task(void *self_, void *arg) {
  axis_extension_t *extension = self_;
  axis_ASSERT(extension, "Invalid argument.");

  axis_notify_data_t *notify_data = (axis_notify_data_t *)arg;
  axis_ASSERT(notify_data, "Invalid argument.");

  axis_extension_thread_t *extension_thread = extension->extension_thread;
  axis_ASSERT(extension_thread, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(extension_thread, true),
             "Invalid use of extension_thread %p.", extension_thread);

  notify_data->notify_func(extension->axis_env, notify_data->user_data);

  axis_notify_data_destroy(notify_data);
}

static void axis_notify_to_extension_group_task(void *self_, void *arg) {
  axis_extension_group_t *extension_group = self_;
  axis_ASSERT(extension_group, "Invalid argument.");

  axis_notify_data_t *notify_data = (axis_notify_data_t *)arg;
  axis_ASSERT(notify_data, "Invalid argument.");

  axis_extension_thread_t *extension_thread = extension_group->extension_thread;
  axis_ASSERT(extension_thread, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(extension_thread, true),
             "Invalid use of extension_thread %p.", extension_thread);

  notify_data->notify_func(extension_group->axis_env, notify_data->user_data);

  axis_notify_data_destroy(notify_data);
}

bool axis_env_proxy_notify(axis_env_proxy_t *self,
                          axis_env_proxy_notify_func_t notify_func,
                          void *user_data, bool sync, axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(notify_func, "Invalid argument.");

  if (!self || !notify_func || !axis_env_proxy_check_integrity(self)) {
    const char *err_msg = "Invalid argument.";
    axis_ASSERT(0, "%s", err_msg);
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT, err_msg);
    }
    return false;
  }

  bool result = true;

  axis_env_t *axis_env = self->axis_env;
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, false),
             "Should not happen.");

  switch (axis_env->attach_to) {
    case axis_ENV_ATTACH_TO_EXTENSION: {
      axis_extension_t *extension = axis_env->attached_target.extension;
      axis_ASSERT(extension, "Invalid argument.");
      // axis_NOLINTNEXTLINE(thread-check)
      // thread-check: This function is intended to be called in any threads,
      // and the use of extension instance is thread safe here.
      axis_ASSERT(axis_extension_check_integrity(extension, false),
                 "Invalid argument.");

      axis_extension_thread_t *extension_thread = extension->extension_thread;
      axis_ASSERT(extension_thread, "Invalid argument.");
      // axis_NOLINTNEXTLINE(thread-check)
      // thread-check: This function is intended to be called in any threads,
      // and the use of extension instance is thread safe here.
      axis_ASSERT(axis_extension_thread_check_integrity(extension_thread, false),
                 "Invalid argument.");

      if (axis_extension_thread_call_by_me(extension_thread)) {
        notify_func(self->axis_env, user_data);
      } else {
        if (sync) {
          axis_mutex_lock(self->lock);

          if (self->acquired_lock_mode_thread &&
              axis_thread_equal_to_current_thread(
                  self->acquired_lock_mode_thread)) {
            // The current outer thread has obtained the power of lock mode, and
            // therefore can perform sync operations.
            notify_func(axis_env, user_data);
          } else {
            if (err) {
              axis_error_set(
                  err, axis_ERRNO_GENERIC,
                  "Perform synchronous axis_notify without acquiring lock_mode "
                  "first.");
            }
            result = false;
          }

          axis_mutex_unlock(self->lock);
        } else {
          int rc = axis_runloop_post_task_tail(
              axis_extension_get_attached_runloop(extension),
              axis_notify_to_extension_task, extension,
              axis_notify_data_create(notify_func, user_data));

          result = rc == 0;
        }
      }
      break;
    }

    case axis_ENV_ATTACH_TO_EXTENSION_GROUP: {
      axis_extension_group_t *extension_group =
          axis_env->attached_target.extension_group;
      axis_ASSERT(extension_group, "Invalid argument.");
      // axis_NOLINTNEXTLINE(thread-check)
      // thread-check: This function is intended to be called in any threads,
      // and the use of extension instance is thread safe here.
      axis_ASSERT(axis_extension_group_check_integrity(extension_group, false),
                 "Invalid argument.");

      axis_extension_thread_t *extension_thread =
          extension_group->extension_thread;
      axis_ASSERT(extension_thread, "Invalid argument.");
      // axis_NOLINTNEXTLINE(thread-check)
      // thread-check: This function is intended to be called in any threads,
      // and the use of extension instance is thread safe here.
      axis_ASSERT(axis_extension_thread_check_integrity(extension_thread, false),
                 "Invalid argument.");

      if (axis_extension_thread_call_by_me(extension_thread)) {
        notify_func(self->axis_env, user_data);
      } else {
        axis_ASSERT(sync == false, "Unsupported operation.");

        int rc = axis_runloop_post_task_tail(
            axis_extension_group_get_attached_runloop(extension_group),
            axis_notify_to_extension_group_task, extension_group,
            axis_notify_data_create(notify_func, user_data));

        result = rc == 0;
      }
      break;
    }

    case axis_ENV_ATTACH_TO_APP: {
      axis_app_t *app = axis_env->attached_target.app;
      axis_ASSERT(app, "Invalid argument.");
      // axis_NOLINTNEXTLINE(thread-check)
      // thread-check: This function is intended to be called in any threads.
      axis_ASSERT(axis_app_check_integrity(app, false), "Invalid argument.");

      if (axis_app_thread_call_by_me(app)) {
        notify_func(self->axis_env, user_data);
      } else {
        axis_ASSERT(sync == false, "Unsupported operation.");

        int rc = axis_runloop_post_task_tail(
            axis_app_get_attached_runloop(app), axis_notify_to_app_task, app,
            axis_notify_data_create(notify_func, user_data));

        result = rc == 0;
      }
      break;
    }

    default:
      axis_ASSERT(0, "Handle more types: %d", axis_env->attach_to);
      break;
  }

  axis_ASSERT(result, "Should not happen.");
  return result;
}

bool axis_env_proxy_notify_async(axis_env_proxy_t *self,
                                axis_env_proxy_notify_func_t notify_func,
                                void *user_data, axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(notify_func, "Invalid argument.");

  if (!self || !notify_func || !axis_env_proxy_check_integrity(self)) {
    const char *err_msg = "Invalid argument.";
    axis_ASSERT(0, "%s", err_msg);
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT, err_msg);
    }
    return false;
  }

  bool result = true;

  axis_env_t *axis_env = self->axis_env;
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, false),
             "Should not happen.");

  switch (axis_env->attach_to) {
    case axis_ENV_ATTACH_TO_EXTENSION: {
      axis_extension_t *extension = axis_env->attached_target.extension;
      axis_ASSERT(extension, "Invalid argument.");
      // axis_NOLINTNEXTLINE(thread-check)
      // thread-check: This function is intended to be called in any threads,
      // and the use of extension instance is thread safe here.
      axis_ASSERT(axis_extension_check_integrity(extension, false),
                 "Invalid argument.");

      axis_runloop_post_task_tail(
          axis_extension_get_attached_runloop(extension),
          axis_notify_to_extension_task, extension,
          axis_notify_data_create(notify_func, user_data));
      break;
    }

    case axis_ENV_ATTACH_TO_EXTENSION_GROUP: {
      axis_extension_group_t *extension_group =
          axis_env->attached_target.extension_group;
      axis_ASSERT(extension_group, "Invalid argument.");
      // axis_NOLINTNEXTLINE(thread-check)
      // thread-check: This function is intended to be called in any threads,
      // and the use of extension instance is thread safe here.
      axis_ASSERT(axis_extension_group_check_integrity(extension_group, false),
                 "Invalid argument.");

      axis_runloop_post_task_tail(
          axis_extension_group_get_attached_runloop(extension_group),
          axis_notify_to_extension_group_task, extension_group,
          axis_notify_data_create(notify_func, user_data));
      break;
    }

    case axis_ENV_ATTACH_TO_APP: {
      axis_app_t *app = axis_env->attached_target.app;
      axis_ASSERT(app, "Invalid argument.");
      // axis_NOLINTNEXTLINE(thread-check)
      // thread-check: This function is intended to be called in any threads.
      axis_ASSERT(axis_app_check_integrity(app, false), "Invalid argument.");

      axis_runloop_post_task_tail(
          axis_app_get_attached_runloop(app), axis_notify_to_app_task, app,
          axis_notify_data_create(notify_func, user_data));
      break;
    }

    default:
      axis_ASSERT(0, "Handle more types: %d", axis_env->attach_to);
      break;
  }

  axis_ASSERT(result, "Should not happen.");
  return result;
}
