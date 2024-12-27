//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdlib.h>

#include "include_internal/ten_runtime/binding/go/ten_env/ten_env.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env_internal.h"
#include "ten_runtime/binding/go/interface/ten/ten_env.h"
#include "ten_runtime/ten_env_proxy/ten_env_proxy.h"
#include "ten_utils/lib/rwlock.h"
#include "ten_utils/macro/check.h"

static void ten_env_proxy_notify_on_deinit_done(ten_env_t *ten_env,
                                                void *user_data) {
  TEN_ASSERT(
      ten_env && ten_env_check_integrity(
                     ten_env, ten_env->attach_to != TEN_ENV_ATTACH_TO_ADDON_HOST
                                  ? true
                                  : false),
      "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  ten_go_ten_env_t *ten_env_bridge = user_data;
  TEN_ASSERT(ten_env_bridge, "Should not happen.");

  if (ten_env_bridge->c_ten_env_proxy) {
    TEN_ASSERT(ten_env_proxy_get_thread_cnt(ten_env_bridge->c_ten_env_proxy,
                                            NULL) == 1,
               "Should not happen.");

    ten_env_proxy_t *ten_env_proxy = ten_env_bridge->c_ten_env_proxy;

    ten_rwlock_lock(ten_env_bridge->lock, 0);
    ten_env_bridge->c_ten_env_proxy = NULL;
    ten_rwlock_unlock(ten_env_bridge->lock, 0);

    bool rc = ten_env_proxy_release(ten_env_proxy, &err);
    TEN_ASSERT(rc, "Should not happen.");
  }

  bool rc = ten_env_on_deinit_done(ten_env, &err);
  TEN_ASSERT(rc, "Should not happen.");

  ten_error_deinit(&err);
}

/**
 * This function should be called only once.
 */
void ten_go_ten_env_on_deinit_done(uintptr_t bridge_addr) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  // Because on_deinit_done() will destroy ten_env_proxy, and when
  // on_deinit_done() is executed, ten_env_proxy must exist (since ten_env_proxy
  // is created during the on_init() process, and calling on_deinit_done()
  // before on_init_done() is not allowed), it's safe here not to check whether
  // ten_env_proxy exists or not. However, one thing to note is that
  // on_deinit_done() can only be called once, and this is a principle that
  // already exists.

  ten_error_t err;
  ten_error_init(&err);

  bool rc = true;
  if (self->c_ten_env->attach_to == TEN_ENV_ATTACH_TO_ADDON_HOST) {
    rc = ten_env_on_deinit_done(self->c_ten_env, &err);
  } else {
    rc = ten_env_proxy_notify(self->c_ten_env_proxy,
                              ten_env_proxy_notify_on_deinit_done, self, false,
                              &err);
  }

  TEN_ASSERT(rc, "Should not happen.");

  ten_error_deinit(&err);
}
