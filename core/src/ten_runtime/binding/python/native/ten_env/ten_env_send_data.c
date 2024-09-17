//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/binding/python/common/error.h"
#include "include_internal/ten_runtime/binding/python/msg/data.h"
#include "include_internal/ten_runtime/binding/python/msg/msg.h"
#include "include_internal/ten_runtime/binding/python/ten_env/ten_env.h"
#include "ten_utils/macro/memory.h"

typedef struct ten_env_notify_send_data_info_t {
  ten_shared_ptr_t *c_data;
} ten_env_notify_send_data_info_t;

static ten_env_notify_send_data_info_t *ten_env_notify_send_data_info_create(
    ten_shared_ptr_t *c_data) {
  TEN_ASSERT(c_data, "Invalid argument.");

  ten_env_notify_send_data_info_t *info =
      TEN_MALLOC(sizeof(ten_env_notify_send_data_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  info->c_data = c_data;

  return info;
}

static void ten_env_notify_send_data_info_destroy(
    ten_env_notify_send_data_info_t *info) {
  TEN_ASSERT(info, "Invalid argument.");

  if (info->c_data) {
    ten_shared_ptr_destroy(info->c_data);
    info->c_data = NULL;
  }

  TEN_FREE(info);
}

static void ten_env_notify_send_data(ten_env_t *ten_env, void *user_data) {
  TEN_ASSERT(user_data, "Invalid argument.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_notify_send_data_info_t *notify_info = user_data;

  ten_env_send_data(ten_env, notify_info->c_data, NULL);

  ten_env_notify_send_data_info_destroy(notify_info);
}

PyObject *ten_py_ten_env_send_data(PyObject *self, PyObject *args) {
  ten_py_ten_env_t *py_ten = (ten_py_ten_env_t *)self;
  TEN_ASSERT(py_ten && ten_py_ten_env_check_integrity(py_ten),
             "Invalid argument.");

  bool success = true;

  ten_error_t err;
  ten_error_init(&err);

  ten_py_data_t *py_data = NULL;
  if (!PyArg_ParseTuple(args, "O!", ten_py_data_py_type(), &py_data)) {
    success = false;
    ten_py_raise_py_type_error_exception(
        "Invalid argument type when send data.");
    goto done;
  }

  ten_shared_ptr_t *cloned_data = ten_shared_ptr_clone(py_data->msg.c_msg);
  ten_env_notify_send_data_info_t *notify_info =
      ten_env_notify_send_data_info_create(cloned_data);

  if (!ten_env_proxy_notify(py_ten->c_ten_env_proxy, ten_env_notify_send_data,
                            notify_info, false, &err)) {
    ten_env_notify_send_data_info_destroy(notify_info);
    success = false;
    ten_py_raise_py_runtime_error_exception("Failed to send data.");
    goto done;
  } else {
    // Destroy the C message from the Python message as the ownership has been
    // transferred to the notify_info.
    ten_py_msg_destroy_c_msg(&py_data->msg);
  }

done:
  ten_error_deinit(&err);

  if (success) {
    Py_RETURN_NONE;
  } else {
    return NULL;
  }
}
