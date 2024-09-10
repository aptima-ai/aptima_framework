//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/binding/python/common/error.h"
#include "include_internal/ten_runtime/binding/python/msg/cmd.h"
#include "include_internal/ten_runtime/binding/python/msg/cmd_result.h"
#include "include_internal/ten_runtime/binding/python/msg/msg.h"
#include "include_internal/ten_runtime/binding/python/ten_env/ten_env.h"
#include "ten_runtime/msg/cmd_result/cmd_result.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/macro/memory.h"

typedef struct ten_env_notify_return_result_info_t {
  ten_shared_ptr_t *c_cmd;
  ten_shared_ptr_t *c_target_cmd;
} ten_env_notify_return_result_info_t;

static ten_env_notify_return_result_info_t *
ten_env_notify_return_result_info_create(ten_shared_ptr_t *c_cmd,
                                         ten_shared_ptr_t *c_target_cmd) {
  TEN_ASSERT(c_cmd, "Invalid argument.");

  ten_env_notify_return_result_info_t *info =
      TEN_MALLOC(sizeof(ten_env_notify_return_result_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  info->c_cmd = c_cmd;
  info->c_target_cmd = c_target_cmd;

  return info;
}

static void ten_env_notify_return_result_info_destroy(
    ten_env_notify_return_result_info_t *info) {
  TEN_ASSERT(info, "Invalid argument.");

  if (info->c_cmd) {
    ten_shared_ptr_destroy(info->c_cmd);
    info->c_cmd = NULL;
  }

  if (info->c_target_cmd) {
    ten_shared_ptr_destroy(info->c_target_cmd);
    info->c_target_cmd = NULL;
  }

  TEN_FREE(info);
}

static void ten_notify_return_result(ten_env_t *ten_env, void *user_data) {
  TEN_ASSERT(user_data, "Invalid argument.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_notify_return_result_info_t *info = user_data;

  ten_error_t err;
  ten_error_init(&err);

  bool rc = false;
  if (info->c_target_cmd) {
    rc = ten_env_return_result(ten_env, info->c_cmd, info->c_target_cmd, &err);
    TEN_ASSERT(rc, "Should not happen.");
  } else {
    rc = ten_env_return_result_directly(ten_env, info->c_cmd, &err);
    TEN_ASSERT(rc, "Should not happen.");
  }

  ten_error_deinit(&err);

  ten_env_notify_return_result_info_destroy(info);
}

PyObject *ten_py_ten_env_return_result(PyObject *self, PyObject *args) {
  ten_py_ten_env_t *py_ten = (ten_py_ten_env_t *)self;
  TEN_ASSERT(py_ten && ten_py_ten_env_check_integrity(py_ten),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 2) {
    return ten_py_raise_py_value_error_exception(
        "Invalid argument count when ten.ten_py_ten_env_return_result.");
  }

  bool success = true;

  ten_error_t err;
  ten_error_init(&err);

  ten_py_cmd_t *py_target_cmd = NULL;
  ten_py_cmd_result_t *py_cmd_result = NULL;

  if (!PyArg_ParseTuple(args, "O!O!", ten_py_cmd_result_py_type(),
                        &py_cmd_result, ten_py_cmd_py_type(), &py_target_cmd)) {
    success = false;
    ten_py_raise_py_type_error_exception(
        "Invalid argument type when return result.");
    goto done;
  }

  ten_shared_ptr_t *c_target_cmd =
      ten_shared_ptr_clone(py_target_cmd->msg.c_msg);
  ten_shared_ptr_t *c_result_cmd =
      ten_shared_ptr_clone(py_cmd_result->msg.c_msg);

  ten_env_notify_return_result_info_t *notify_info =
      ten_env_notify_return_result_info_create(c_result_cmd, c_target_cmd);

  bool rc =
      ten_env_proxy_notify(py_ten->c_ten_env_proxy, ten_notify_return_result,
                           notify_info, false, &err);
  if (!rc) {
    ten_env_notify_return_result_info_destroy(notify_info);
    success = false;
    ten_py_raise_py_runtime_error_exception("Failed to return result.");
    goto done;
  } else {
    if (ten_cmd_result_get_is_final(py_cmd_result->msg.c_msg, &err)) {
      // Remove the C message from the python target message if it is the final
      // cmd result.
      ten_py_msg_destroy_c_msg(&py_target_cmd->msg);
    }

    // Destroy the C message from the Python message as the ownership has been
    // transferred to the notify_info.
    ten_py_msg_destroy_c_msg(&py_cmd_result->msg);
  }

done:
  ten_error_deinit(&err);

  if (success) {
    Py_RETURN_NONE;
  } else {
    return NULL;
  }
}

PyObject *ten_py_ten_env_return_result_directly(PyObject *self,
                                                PyObject *args) {
  ten_py_ten_env_t *py_ten = (ten_py_ten_env_t *)self;
  TEN_ASSERT(py_ten && ten_py_ten_env_check_integrity(py_ten),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 1) {
    return ten_py_raise_py_value_error_exception(
        "Invalid argument count when "
        "ten.ten_py_ten_env_return_result_directly.");
  }

  bool success = true;

  ten_error_t err;
  ten_error_init(&err);

  ten_py_cmd_result_t *py_cmd_result = NULL;

  if (!PyArg_ParseTuple(args, "O!", ten_py_cmd_result_py_type(),
                        &py_cmd_result)) {
    success = false;
    ten_py_raise_py_type_error_exception(
        "Invalid argument type when return result directly.");
    goto done;
  }

  ten_shared_ptr_t *c_result_cmd =
      ten_shared_ptr_clone(py_cmd_result->msg.c_msg);

  ten_env_notify_return_result_info_t *notify_info =
      ten_env_notify_return_result_info_create(c_result_cmd, NULL);

  if (!ten_env_proxy_notify(py_ten->c_ten_env_proxy, ten_notify_return_result,
                            notify_info, false, &err)) {
    ten_env_notify_return_result_info_destroy(notify_info);
    success = false;
    ten_py_raise_py_runtime_error_exception(
        "Failed to return result directly.");
    goto done;
  } else {
    // Destroy the C message from the Python message as the ownership has been
    // transferred to the notify_info.
    ten_py_msg_destroy_c_msg(&py_cmd_result->msg);
  }

done:
  ten_error_deinit(&err);

  if (success) {
    Py_RETURN_NONE;
  } else {
    return NULL;
  }
}
