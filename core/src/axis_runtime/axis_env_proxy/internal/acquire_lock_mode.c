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
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/lib/thread.h"
#include "axis_utils/macro/check.h"

bool axis_env_proxy_acquire_lock_mode(axis_env_proxy_t *self, axis_error_t *err) {
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

  // Competing for the lock_mode_lock so that only _1_ outer thread can proceed,
  // if the competition is successful, it does _not_ mean that the extension
  // thread has been blocked. The extension thread may still be running, so some
  // judgment is needed to confirm that the extension thread has been blocked.
  // Only then can the outer thread safely use the things in the APTIMA world
  // directly.
  int rc = axis_mutex_lock(extension_thread->lock_mode_lock);
  axis_ASSERT(!rc, "Should not happen.");

  rc = axis_mutex_lock(self->lock);
  axis_ASSERT(!rc, "Should not happen.");

  axis_ASSERT(!self->acquired_lock_mode_thread, "Should not happen.");
  self->acquired_lock_mode_thread = axis_thread_create_fake(NULL);

  if (extension_thread->in_lock_mode == false) {
    // This means that the extension thread may still be running, so there needs
    // to be some way to wait for the extension thread to be blocked. Only after
    // confirming that the extension thread has been blocked can the outer
    // thread safely directly use the APTIMA world.
    //
    // The method is to insert a special task into the extension thread's
    // runloop. When the extension thread runloop executes that task, the outer
    // thread can safely continue and directly use the APTIMA world.

    axis_acquire_lock_mode_result_t *suspend_result =
        axis_MALLOC(sizeof(axis_acquire_lock_mode_result_t));
    axis_ASSERT(suspend_result, "Failed to allocate memory.");

    axis_error_init(&suspend_result->err);
    suspend_result->completed = NULL;

    suspend_result->completed = axis_event_create(0, 1);

    int rc = axis_runloop_post_task_front(
        extension_thread->runloop,
        axis_extension_thread_process_acquire_lock_mode_task, extension_thread,
        suspend_result);
    axis_ASSERT(!rc, "Should not happen.");

    // Wait for the extension thread to be suspended successfully.
    rc = axis_event_wait(suspend_result->completed, -1);
    axis_ASSERT(!rc, "Should not happen.");

    axis_event_destroy(suspend_result->completed);

    if (axis_error_errno(&suspend_result->err) > 0) {
      axis_ASSERT(0, "Should not happen.");

      result = false;

      rc = axis_mutex_unlock(extension_thread->lock_mode_lock);
      axis_ASSERT(!rc, "Should not happen.");
    }

    axis_ASSERT(extension_thread->in_lock_mode, "Should not happen.");

    axis_FREE(suspend_result);
  }

  axis_mutex_unlock(self->lock);

  axis_ASSERT(result, "Should not happen.");
  return result;
}
