//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdlib.h>

#include "include_internal/axis_runtime/binding/go/axis_env/axis_env.h"
#include "include_internal/axis_runtime/binding/go/axis_env/axis_env_internal.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/binding/go/interface/aptima/axis_env.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

static void axis_env_proxy_notify_on_init_done(axis_env_t *axis_env,
                                              axis_UNUSED void *user_data) {
  axis_ASSERT(
      axis_env &&
          axis_env_check_integrity(
              axis_env,
              axis_env->attach_to != axis_ENV_ATTACH_TO_ADDON ? true : false),
      "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  bool rc = axis_env_on_init_done(axis_env, &err);
  axis_ASSERT(rc, "Should not happen.");

  axis_error_deinit(&err);
}

void axis_go_axis_env_on_init_done(uintptr_t bridge_addr) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");

  axis_GO_axis_ENV_IS_ALIVE_REGION_BEGIN(self, {});

  axis_error_t err;
  axis_error_init(&err);

  bool rc = true;

  if (self->c_axis_env->attach_to == axis_ENV_ATTACH_TO_ADDON) {
    rc = axis_env_on_init_done(self->c_axis_env, &err);
  } else {
    rc = axis_env_proxy_notify(self->c_axis_env_proxy,
                              axis_env_proxy_notify_on_init_done, NULL, false,
                              &err);
  }
  axis_ASSERT(rc, "Should not happen.");

  axis_error_deinit(&err);

  axis_GO_axis_ENV_IS_ALIVE_REGION_END(self);

axis_is_close:
  return;
}
