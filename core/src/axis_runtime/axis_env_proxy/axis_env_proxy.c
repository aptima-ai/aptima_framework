//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/axis_env/axis_env_proxy.h"

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/extension_thread/extension_thread.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "include_internal/axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_runtime/common/errno.h"
#include "axis_runtime/extension/extension.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

// There's no need to check for thread-safety, as axis_env_proxy is inherently
// designed to be used in a multi-threaded environment.
bool axis_env_proxy_check_integrity(axis_env_proxy_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_PROXY_SIGNATURE) {
    return false;
  }

  if (!self->lock) {
    axis_ASSERT(0, "Invalid argument.");
    return false;
  }

  return true;
}

axis_env_proxy_t *axis_env_proxy_create(axis_env_t *axis_env,
                                      size_t initial_thread_cnt,
                                      axis_error_t *err) {
  if (!axis_env) {
    const char *err_msg = "Create axis_env_proxy without specifying axis_env.";
    axis_ASSERT(0, "%s", err_msg);
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT, err_msg);
    }
    return NULL;
  }

  // Checking 1: The platform currently only supports creating a `axis_env_proxy`
  // from the `axis_env` of an extension, an extension_group, and an app.
  switch (axis_env->attach_to) {
    case axis_ENV_ATTACH_TO_EXTENSION:
    case axis_ENV_ATTACH_TO_EXTENSION_GROUP:
    case axis_ENV_ATTACH_TO_APP:
      break;

    default: {
      const char *err_msg = "Create axis_env_proxy from unsupported aptima.";
      axis_ASSERT(0, "%s", err_msg);
      if (err) {
        axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT, err_msg);
      }
      return NULL;
    }
  }

  // Checking 2: The creation of `axis_env_proxy` must occur within the belonging
  // thread of the corresponding `axis_env`.
  switch (axis_env->attach_to) {
    case axis_ENV_ATTACH_TO_EXTENSION: {
      axis_extension_t *extension = axis_env->attached_target.extension;
      axis_ASSERT(extension && axis_extension_check_integrity(extension, true),
                 "Should not happen.");

      axis_extension_thread_t *extension_thread = extension->extension_thread;
      axis_ASSERT(extension_thread, "Should not happen.");

      if (!axis_extension_thread_call_by_me(extension_thread)) {
        const char *err_msg =
            "axis_env_proxy needs to be created in extension thread.";
        axis_ASSERT(0, "%s", err_msg);
        if (err) {
          axis_error_set(err, axis_ERRNO_GENERIC, err_msg);
        }
        return NULL;
      }
      break;
    }

    case axis_ENV_ATTACH_TO_EXTENSION_GROUP: {
      axis_extension_group_t *extension_group =
          axis_env->attached_target.extension_group;
      axis_ASSERT(extension_group &&
                     axis_extension_group_check_integrity(extension_group, true),
                 "Should not happen.");

      axis_extension_thread_t *extension_thread =
          extension_group->extension_thread;
      axis_ASSERT(extension_thread, "Should not happen.");

      if (!axis_extension_thread_call_by_me(extension_thread)) {
        const char *err_msg =
            "axis_env_proxy needs to be created in extension thread.";
        axis_ASSERT(0, "%s", err_msg);
        if (err) {
          axis_error_set(err, axis_ERRNO_GENERIC, err_msg);
        }
        return NULL;
      }
      break;
    }

    case axis_ENV_ATTACH_TO_APP: {
      axis_app_t *app = axis_env->attached_target.app;
      axis_ASSERT(app, "Should not happen.");

      if (!axis_app_thread_call_by_me(app)) {
        const char *err_msg =
            "axis_env_proxy needs to be created in app thread.";
        axis_ASSERT(0, "%s", err_msg);
        if (err) {
          axis_error_set(err, axis_ERRNO_GENERIC, err_msg);
        }
        return NULL;
      }
      break;
    }

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  axis_env_proxy_t *self = axis_MALLOC(sizeof(axis_env_proxy_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_signature_set(&self->signature, axis_PROXY_SIGNATURE);
  self->lock = axis_mutex_create();
  self->acquired_lock_mode_thread = NULL;
  self->thread_cnt = initial_thread_cnt;
  self->axis_env = axis_env;

  // The created `axis_env_proxy` needs to be recorded, and the corresponding
  // `axis_env` cannot be destroyed as long as any `axis_env_proxy` has not yet
  // been destroyed.
  axis_env_add_axis_proxy(axis_env, self);

  return self;
}

static void axis_env_proxy_destroy(axis_env_proxy_t *self) {
  axis_ASSERT(self && axis_env_proxy_check_integrity(self), "Invalid argument.");

  axis_mutex_destroy(self->lock);
  self->lock = NULL;

  if (self->acquired_lock_mode_thread) {
    axis_FREE(self->acquired_lock_mode_thread);
    self->acquired_lock_mode_thread = NULL;
  }
  self->axis_env = NULL;

  axis_FREE(self);
}

bool axis_env_proxy_acquire(axis_env_proxy_t *self, axis_error_t *err) {
  if (!self || !axis_env_proxy_check_integrity(self)) {
    const char *err_msg = "Invalid argument.";
    axis_ASSERT(0, "%s", err_msg);
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT, err_msg);
    }
    return false;
  }

  axis_mutex_lock(self->lock);
  self->thread_cnt++;
  axis_mutex_unlock(self->lock);

  return true;
}

static void axis_notify_proxy_is_deleted(void *self_, axis_UNUSED void *arg) {
  axis_env_t *self = (axis_env_t *)self_;
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(self, true), "Invalid use of axis_env %p.",
             self);

  axis_env_proxy_t *axis_env_proxy = arg;
  axis_ASSERT(axis_env_proxy && axis_env_proxy_check_integrity(axis_env_proxy),
             "Invalid argument.");

  axis_env_delete_axis_proxy(self, axis_env_proxy);
  axis_env_proxy_destroy(axis_env_proxy);
}

bool axis_env_proxy_release(axis_env_proxy_t *self, axis_error_t *err) {
  if (!self || !axis_env_proxy_check_integrity(self)) {
    const char *err_msg = "Invalid argument.";
    axis_ASSERT(0, "%s", err_msg);
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT, err_msg);
    }
    return false;
  }

  bool result = true;

  axis_mutex_lock(self->lock);

  axis_ASSERT(self->thread_cnt, "Should not happen.");

  if (!self->thread_cnt) {
    const char *err_msg =
        "Unpaired calls of axis_proxy_acquire and axis_proxy_release.";
    axis_ASSERT(0, "%s", err_msg);
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT, err_msg);
    }

    goto done;
  }

  self->thread_cnt--;
  if (!self->thread_cnt) {
    axis_env_t *axis_env = self->axis_env;
    // axis_NOLINTNEXTLINE(thread-check)
    // thread-check: axis_env_proxy is used in any threads other then the
    // belonging extension thread, so we cannot check thread safety here, and
    // the following post_task is thread safe.
    axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, false),
               "Should not happen.");

    axis_mutex_unlock(self->lock);

    axis_runloop_post_task_tail(axis_env_get_attached_runloop(axis_env),
                               axis_notify_proxy_is_deleted, axis_env, self);

    return true;
  }

done:
  axis_mutex_unlock(self->lock);
  return result;
}

size_t axis_env_proxy_get_thread_cnt(axis_env_proxy_t *self, axis_error_t *err) {
  if (!self || !axis_env_proxy_check_integrity(self)) {
    const char *err_msg = "Invalid argument.";
    axis_ASSERT(0, "%s", err_msg);
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT, err_msg);
    }
    return -1;
  }

  axis_mutex_lock(self->lock);
  size_t cnt = self->thread_cnt;
  axis_mutex_unlock(self->lock);

  return cnt;
}
