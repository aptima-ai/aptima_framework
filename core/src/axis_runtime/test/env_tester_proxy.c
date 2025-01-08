//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/test/env_tester_proxy.h"

#include "include_internal/axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "include_internal/axis_runtime/test/env_tester.h"
#include "include_internal/axis_runtime/test/env_tester_proxy.h"
#include "include_internal/axis_runtime/test/extension_tester.h"
#include "axis_runtime/common/errno.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"

static bool axis_env_tester_proxy_check_integrity(axis_env_tester_proxy_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_ENV_TESTER_PROXY_SIGNATURE) {
    return false;
  }

  return true;
}

axis_env_tester_proxy_t *axis_env_tester_proxy_create(
    axis_env_tester_t *axis_env_tester, axis_error_t *err) {
  if (!axis_env_tester) {
    const char *err_msg =
        "Create axis_env_tester_proxy without specifying axis_env_tester.";
    axis_ASSERT(0, "%s", err_msg);
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT, err_msg);
    }
    return NULL;
  }

  axis_env_tester_proxy_t *self = axis_MALLOC(sizeof(axis_env_tester_proxy_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_signature_set(&self->signature, axis_ENV_TESTER_PROXY_SIGNATURE);
  self->axis_env_tester = axis_env_tester;

  axis_extension_tester_t *tester = axis_env_tester->tester;
  axis_ASSERT(tester && axis_extension_tester_check_integrity(tester, true),
             "Should not happen.");

  // Add a reference to the `axis_env_proxy` object.
  axis_env_proxy_acquire(tester->test_extension_axis_env_proxy, NULL);

  return self;
}

static void axis_env_tester_on_proxy_deleted(void *self_, void *arg) {
  axis_env_tester_t *axis_env_tester = self_;
  axis_env_tester_proxy_t *self = arg;
  axis_ASSERT(axis_env_tester && axis_env_tester_check_integrity(axis_env_tester),
             "Should not happen.");
  axis_ASSERT(self && axis_env_tester_proxy_check_integrity(self),
             "Should not happen.");

  axis_extension_tester_t *tester = axis_env_tester->tester;
  axis_ASSERT(tester && axis_extension_tester_check_integrity(tester, true),
             "Should not happen.");

  // Release the reference to the `axis_env_proxy` object.
  axis_env_proxy_release(tester->test_extension_axis_env_proxy, NULL);

  axis_FREE(self);
}

bool axis_env_tester_proxy_release(axis_env_tester_proxy_t *self,
                                  axis_error_t *err) {
  if (!self || !axis_env_tester_proxy_check_integrity(self)) {
    const char *err_msg = "Invalid argument.";
    axis_ASSERT(0, "%s", err_msg);
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT, err_msg);
    }
    return false;
  }

  axis_env_tester_t *axis_env_tester = self->axis_env_tester;
  axis_ASSERT(axis_env_tester, "Should not happen.");

  axis_runloop_post_task_tail(axis_env_tester->tester->tester_runloop,
                             axis_env_tester_on_proxy_deleted, axis_env_tester,
                             self);

  return true;
}

bool axis_env_tester_proxy_notify(axis_env_tester_proxy_t *self,
                                 axis_env_tester_proxy_notify_func_t notify_func,
                                 void *user_data, axis_error_t *err) {
  if (!self || !notify_func || !axis_env_tester_proxy_check_integrity(self)) {
    const char *err_msg = "Invalid argument.";
    axis_ASSERT(0, "%s", err_msg);
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT, err_msg);
    }
    return false;
  }

  axis_env_tester_t *axis_env_tester = self->axis_env_tester;
  axis_ASSERT(axis_env_tester, "Should not happen.");

  int rc = axis_runloop_post_task_tail(axis_env_tester->tester->tester_runloop,
                                      (axis_runloop_task_func_t)notify_func,
                                      axis_env_tester, user_data);

  return rc == 0;
}
