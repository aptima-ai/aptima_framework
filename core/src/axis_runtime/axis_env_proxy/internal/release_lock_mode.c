//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/extension_thread/extension_thread.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "include_internal/axis_runtime/axis_env/axis_env_proxy.h"
#include "include_internal/axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_runtime/extension/extension.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/lib/thread.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"

bool axis_env_proxy_release_lock_mode(axis_env_proxy_t *self, axis_error_t *err) {
  // The outer thread must ensure the validity of the axis_env_proxy instance.
  if (!self) {
    const char *err_msg = "Invalid argument.";
    axis_ASSERT(0, "%s", err_msg);
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT, err_msg);
    }
    return false;
  }
  if (!axis_env_proxy_check_integrity(self)) {
    const char *err_msg = "Invalid argument.";
    axis_ASSERT(0, "%s", err_msg);
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT, err_msg);
    }
    return false;
  }

  bool result = true;

  axis_env_t *axis_env = self->axis_env;
  axis_ASSERT(axis_env, "Should not happen.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads other
  // then the belonging extension thread, and within this function, we only
  // utilize the immutable fields of aptima or aptima fields protected by locks.
  axis_ASSERT(axis_env_check_integrity(axis_env, false), "Should not happen.");
  axis_ASSERT(axis_env->attach_to == axis_ENV_ATTACH_TO_EXTENSION,
             "Invalid argument.");

  // If any axis_env_proxy instance exists, then the APTIMA world will not
  // disappear, and therefore things related to the extension world, such as
  // extension and extension thread, will still exist and will not change.
  // Therefore, it is safe to access extension and extension_thread below.
  axis_extension_thread_t *extension_thread =
      axis_env->attached_target.extension->extension_thread;
  axis_ASSERT(extension_thread, "Should not happen.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads other
  // then the belonging extension thread, and within this function, we only
  // utilize the immutable fields of aptima or aptima fields protected by locks.
  axis_ASSERT(axis_extension_thread_check_integrity(extension_thread, false),
             "Should not happen.");

  axis_mutex_lock(self->lock);

  if (self->acquired_lock_mode_thread &&
      axis_thread_equal_to_current_thread(self->acquired_lock_mode_thread)) {
    // It is the current outer thread that has acquired the lock mode lock,
    // therefore release it.
    int rc = axis_mutex_unlock(extension_thread->lock_mode_lock);
    axis_ASSERT(!rc, "Should not happen.");

    axis_FREE(self->acquired_lock_mode_thread);
    self->acquired_lock_mode_thread = NULL;
  }

  axis_mutex_unlock(self->lock);

  axis_ASSERT(result, "Should not happen.");
  return result;
}
