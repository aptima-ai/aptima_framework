//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <string.h>

#include "include_internal/ten_runtime/binding/python/common/common.h"
#include "include_internal/ten_runtime/binding/python/ten_env/ten_env.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_runtime/binding/python/common.h"
#include "ten_runtime/ten_env/internal/on_xxx_done.h"
#include "ten_runtime/ten_env_proxy/ten_env_proxy.h"
#include "ten_utils/macro/mark.h"

static void ten_env_notify_on_deinit_done(ten_env_t *ten_env, void *user_data) {
  TEN_ASSERT(
      ten_env &&
          ten_env_check_integrity(
              ten_env,
              ten_env->attach_to != TEN_ENV_ATTACH_TO_ADDON ? true : false),
      "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  ten_py_ten_env_t *py_ten = user_data;
  TEN_ASSERT(py_ten, "Should not happen.");

  if (py_ten->c_ten_env_proxy) {
    TEN_ASSERT(ten_env_proxy_get_thread_cnt(py_ten->c_ten_env_proxy, NULL) == 1,
               "Should not happen.");

    ten_env_proxy_t *ten_env_proxy = py_ten->c_ten_env_proxy;

    py_ten->c_ten_env_proxy = NULL;

    bool rc = ten_env_proxy_release(ten_env_proxy, &err);
    TEN_ASSERT(rc, "Should not happen.");
  }

  bool rc = ten_env_on_deinit_done(ten_env, &err);
  TEN_ASSERT(rc, "Should not happen.");

  ten_error_deinit(&err);

  if (py_ten->need_to_release_gil_state) {
    if (!ten_py_is_holding_gil()) {
      // The gil is not held by the current thread, so we have to acquire the
      // gil before we can release the gil state.
      ten_py_eval_restore_thread(NULL);

      // Release the gil state and the gil.
      ten_py_gil_state_release(PyGILState_UNLOCKED);
    } else {
      // Release the gil state but keep holding the gil.
      ten_py_gil_state_release(PyGILState_LOCKED);
    }
  }
}

PyObject *ten_py_ten_env_on_deinit_done(PyObject *self,
                                        TEN_UNUSED PyObject *args) {
  ten_py_ten_env_t *py_ten = (ten_py_ten_env_t *)self;
  TEN_ASSERT(py_ten && ten_py_ten_env_check_integrity(py_ten),
             "Invalid argument.");

  ten_error_t err;
  ten_error_init(&err);

  bool rc = true;
  if (py_ten->c_ten_env->attach_to == TEN_ENV_ATTACH_TO_ADDON) {
    rc = ten_env_on_deinit_done(py_ten->c_ten_env, &err);
  } else {
    rc = ten_env_proxy_notify(py_ten->c_ten_env_proxy,
                              ten_env_notify_on_deinit_done, py_ten, false,
                              &err);
  }

  ten_error_deinit(&err);

  TEN_ASSERT(rc, "Should not happen.");

  Py_RETURN_NONE;
}
