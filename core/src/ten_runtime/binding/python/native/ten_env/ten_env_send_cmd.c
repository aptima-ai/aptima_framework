//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/binding/python/common/common.h"
#include "include_internal/ten_runtime/binding/python/common/error.h"
#include "include_internal/ten_runtime/binding/python/msg/cmd.h"
#include "include_internal/ten_runtime/binding/python/msg/cmd_result.h"
#include "include_internal/ten_runtime/binding/python/msg/msg.h"
#include "include_internal/ten_runtime/binding/python/ten_env/ten_env.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd_base.h"
#include "ten_runtime/extension/extension.h"
#include "ten_runtime/msg/cmd_result/cmd_result.h"
#include "ten_runtime/ten_env_proxy/ten_env_proxy.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/memory.h"

typedef struct ten_env_notify_send_cmd_info_t {
  ten_shared_ptr_t *c_cmd;
  PyObject *py_cb_func;
} ten_env_notify_send_cmd_info_t;

static ten_env_notify_send_cmd_info_t *ten_env_notify_send_cmd_info_create(
    ten_shared_ptr_t *c_cmd, PyObject *py_cb_func) {
  TEN_ASSERT(c_cmd, "Invalid argument.");

  ten_env_notify_send_cmd_info_t *info =
      TEN_MALLOC(sizeof(ten_env_notify_send_cmd_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  info->c_cmd = c_cmd;
  info->py_cb_func = py_cb_func;

  if (py_cb_func != NULL) {
    Py_INCREF(py_cb_func);
  }

  return info;
}

static void proxy_send_xxx_callback(ten_extension_t *extension,
                                    ten_env_t *ten_env,
                                    ten_shared_ptr_t *cmd_result,
                                    void *callback_info) {
  TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
             "Should not happen.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");
  TEN_ASSERT(cmd_result && ten_cmd_base_check_integrity(cmd_result),
             "Should not happen.");
  TEN_ASSERT(callback_info, "Should not happen.");

  // Allows C codes to work safely with Python objects.
  PyGILState_STATE prev_state = ten_py_gil_state_ensure();

  ten_py_ten_env_t *py_ten_env = ten_py_ten_wrap(ten_env);
  ten_py_cmd_result_t *cmd_result_bridge = ten_py_cmd_result_wrap(cmd_result);

  PyObject *cb_func = callback_info;
  PyObject *arglist =
      Py_BuildValue("(OO)", py_ten_env->actual_py_ten_env, cmd_result_bridge);

  PyObject *result = PyObject_CallObject(cb_func, arglist);
  Py_XDECREF(result);  // Ensure cleanup if an error occurred.

  bool err_occurred = ten_py_check_and_clear_py_error();
  TEN_ASSERT(!err_occurred, "Should not happen.");

  Py_XDECREF(arglist);

  bool is_final = ten_cmd_result_get_is_final(cmd_result, NULL);
  if (is_final) {
    Py_XDECREF(cb_func);
  }

  ten_py_cmd_result_invalidate(cmd_result_bridge);

  ten_py_gil_state_release(prev_state);
}

static void ten_env_notify_send_cmd_info_destroy(
    ten_env_notify_send_cmd_info_t *info) {
  TEN_ASSERT(info, "Invalid argument.");

  if (info->c_cmd) {
    ten_shared_ptr_destroy(info->c_cmd);
    info->c_cmd = NULL;
  }

  info->py_cb_func = NULL;

  TEN_FREE(info);
}

static void ten_env_notify_send_cmd(ten_env_t *ten_env, void *user_data) {
  TEN_ASSERT(user_data, "Invalid argument.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_notify_send_cmd_info_t *notify_info = user_data;

  ten_error_t err;
  ten_error_init(&err);

  TEN_UNUSED bool res = false;
  if (notify_info->py_cb_func == NULL) {
    res = ten_env_send_cmd(ten_env, notify_info->c_cmd, NULL, NULL, NULL);
  } else {
    res = ten_env_send_cmd(ten_env, notify_info->c_cmd, proxy_send_xxx_callback,
                           notify_info->py_cb_func, NULL);
  }

  ten_error_deinit(&err);

  ten_env_notify_send_cmd_info_destroy(notify_info);
}

PyObject *ten_py_ten_env_send_cmd(PyObject *self, PyObject *args) {
  ten_py_ten_env_t *py_ten = (ten_py_ten_env_t *)self;
  TEN_ASSERT(py_ten && ten_py_ten_env_check_integrity(py_ten),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 2) {
    return ten_py_raise_py_value_error_exception(
        "Invalid argument count when ten.ten_py_ten_env_send_cmd.");
  }

  bool success = true;

  ten_error_t err;
  ten_error_init(&err);

  ten_py_cmd_t *py_cmd = NULL;
  PyObject *cb_func = NULL;

  if (!PyArg_ParseTuple(args, "O!O", ten_py_cmd_py_type(), &py_cmd, &cb_func)) {
    success = false;
    ten_py_raise_py_type_error_exception(
        "Invalid argument type when send cmd.");
    goto done;
  }

  // Check if cb_func is callable.
  if (!PyCallable_Check(cb_func)) {
    cb_func = NULL;
  }

  ten_shared_ptr_t *cloned_cmd = ten_shared_ptr_clone(py_cmd->msg.c_msg);
  ten_env_notify_send_cmd_info_t *notify_info =
      ten_env_notify_send_cmd_info_create(cloned_cmd, cb_func);

  if (!ten_env_proxy_notify(py_ten->c_ten_env_proxy, ten_env_notify_send_cmd,
                            notify_info, false, &err)) {
    ten_env_notify_send_cmd_info_destroy(notify_info);
    success = false;
    ten_py_raise_py_runtime_error_exception("Failed to send cmd.");
    goto done;
  } else {
    // Destroy the C message from the Python message as the ownership has been
    // transferred to the notify_info.
    ten_py_msg_destroy_c_msg(&py_cmd->msg);
  }

done:
  ten_error_deinit(&err);

  if (success) {
    Py_RETURN_NONE;
  } else {
    return NULL;
  }
}
