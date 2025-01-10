//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdlib.h>

#include "include_internal/axis_runtime/binding/go/axis_env/axis_env.h"
#include "include_internal/axis_runtime/binding/go/axis_env/axis_env_internal.h"
#include "axis_runtime/binding/go/interface/aptima/axis_env.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_utils/lib/rwlock.h"
#include "axis_utils/macro/check.h"

static void axis_env_proxy_notify_on_deinit_done(axis_env_t *axis_env,
                                                void *user_data) {
  axis_ASSERT(
      axis_env &&
          axis_env_check_integrity(
              axis_env,
              axis_env->attach_to != axis_ENV_ATTACH_TO_ADDON ? true : false),
      "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  axis_go_axis_env_t *axis_env_bridge = user_data;
  axis_ASSERT(axis_env_bridge, "Should not happen.");

  if (axis_env_bridge->c_axis_env_proxy) {
    axis_ASSERT(axis_env_proxy_get_thread_cnt(axis_env_bridge->c_axis_env_proxy,
                                            NULL) == 1,
               "Should not happen.");

    axis_env_proxy_t *axis_env_proxy = axis_env_bridge->c_axis_env_proxy;

    axis_rwlock_lock(axis_env_bridge->lock, 0);
    axis_env_bridge->c_axis_env_proxy = NULL;
    axis_rwlock_unlock(axis_env_bridge->lock, 0);

    bool rc = axis_env_proxy_release(axis_env_proxy, &err);
    axis_ASSERT(rc, "Should not happen.");
  }

  bool rc = axis_env_on_deinit_done(axis_env, &err);
  axis_ASSERT(rc, "Should not happen.");

  axis_error_deinit(&err);
}

/**
 * This function should be called only once.
 */
void axis_go_axis_env_on_deinit_done(uintptr_t bridge_addr) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");

  // Because on_deinit_done() will destroy axis_env_proxy, and when
  // on_deinit_done() is executed, axis_env_proxy must exist (since axis_env_proxy
  // is created during the on_init() process, and calling on_deinit_done()
  // before on_init_done() is not allowed), it's safe here not to check whether
  // axis_env_proxy exists or not. However, one thing to note is that
  // on_deinit_done() can only be called once, and this is a principle that
  // already exists.

  axis_error_t err;
  axis_error_init(&err);

  bool rc = true;
  if (self->c_axis_env->attach_to == axis_ENV_ATTACH_TO_ADDON) {
    rc = axis_env_on_deinit_done(self->c_axis_env, &err);
  } else {
    rc = axis_env_proxy_notify(self->c_axis_env_proxy,
                              axis_env_proxy_notify_on_deinit_done, self, false,
                              &err);
  }

  axis_ASSERT(rc, "Should not happen.");

  axis_error_deinit(&err);
}
