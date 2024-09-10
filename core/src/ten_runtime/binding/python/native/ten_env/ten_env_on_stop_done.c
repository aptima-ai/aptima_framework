//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include <string.h>

#include "include_internal/ten_runtime/binding/python/ten_env/ten_env.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_runtime/msg/msg.h"
#include "ten_runtime/ten_env/internal/on_xxx_done.h"

static void ten_env_notify_on_stop_done(ten_env_t *ten_env,
                                        TEN_UNUSED void *user_data) {
  TEN_ASSERT(
      ten_env &&
          ten_env_check_integrity(
              ten_env,
              ten_env->attach_to != TEN_ENV_ATTACH_TO_ADDON ? true : false),
      "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  bool rc = ten_env_on_stop_done(ten_env, &err);
  TEN_ASSERT(rc, "Should not happen.");

  ten_error_deinit(&err);
}

PyObject *ten_py_ten_env_on_stop_done(PyObject *self,
                                      TEN_UNUSED PyObject *args) {
  ten_py_ten_env_t *py_ten = (ten_py_ten_env_t *)self;
  TEN_ASSERT(py_ten && ten_py_ten_env_check_integrity(py_ten),
             "Invalid argument.");

  ten_error_t err;
  ten_error_init(&err);

  TEN_UNUSED bool rc = ten_env_proxy_notify(
      py_ten->c_ten_env_proxy, ten_env_notify_on_stop_done, NULL, false, &err);

  ten_error_deinit(&err);

  Py_RETURN_NONE;
}
