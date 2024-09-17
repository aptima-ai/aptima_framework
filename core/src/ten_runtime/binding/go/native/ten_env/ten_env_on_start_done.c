//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include <stdlib.h>

#include "include_internal/ten_runtime/binding/go/ten_env/ten_env.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env_internal.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_utils/macro/check.h"
#include "ten_runtime/binding/go/interface/ten/ten_env.h"
#include "ten_runtime/ten_env_proxy/ten_env_proxy.h"
#include "ten_utils/macro/mark.h"

static void ten_env_notify_on_start_done(ten_env_t *ten_env,
                                         TEN_UNUSED void *user_data) {
  TEN_ASSERT(
      ten_env &&
          ten_env_check_integrity(
              ten_env,
              ten_env->attach_to != TEN_ENV_ATTACH_TO_ADDON ? true : false),
      "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  bool rc = ten_env_on_start_done(ten_env, &err);
  TEN_ASSERT(rc, "Should not happen.");

  ten_error_deinit(&err);
}

void ten_go_ten_env_on_start_done(uintptr_t bridge_addr) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  TEN_GO_TEN_IS_ALIVE_REGION_BEGIN(self, {});

  ten_error_t err;
  ten_error_init(&err);

  TEN_UNUSED bool rc = ten_env_proxy_notify(
      self->c_ten_env_proxy, ten_env_notify_on_start_done, NULL, false, &err);
  TEN_ASSERT(rc, "Should not happen.");

  ten_error_deinit(&err);
  TEN_GO_TEN_IS_ALIVE_REGION_END(self);
ten_is_close:
  return;
}
