//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/binding/python/common/common.h"
#include "include_internal/ten_runtime/binding/python/common/error.h"
#include "include_internal/ten_runtime/binding/python/msg/cmd_result.h"
#include "include_internal/ten_runtime/binding/python/ten_env/ten_env.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd_base.h"
#include "object.h"
#include "ten_runtime/extension/extension.h"
#include "ten_runtime/msg/cmd_result/cmd_result.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/macro/mark.h"
#include "ten_utils/macro/memory.h"

typedef struct ten_env_notify_send_json_info_t {
  ten_json_t *c_json;
  PyObject *py_cb_func;
} ten_env_notify_send_json_info_t;

static ten_env_notify_send_json_info_t *ten_env_notify_send_json_info_create(
    ten_json_t *c_json, PyObject *py_cb_func) {
  TEN_ASSERT(c_json, "Invalid argument.");

  ten_env_notify_send_json_info_t *info =
      TEN_MALLOC(sizeof(ten_env_notify_send_json_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  info->c_json = ten_json_incref(c_json);
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
  if (!result || ten_py_check_and_clear_py_error()) {
    Py_XDECREF(result);  // Ensure cleanup if an error occurred.
  }

  Py_XDECREF(arglist);

  bool is_final = ten_cmd_result_get_is_final(cmd_result, NULL);
  if (is_final) {
    Py_XDECREF(cb_func);
  }

  ten_py_cmd_result_invalidate(cmd_result_bridge);

  ten_py_gil_state_release(prev_state);
}

static void ten_env_notify_send_json_info_destroy(
    ten_env_notify_send_json_info_t *info) {
  TEN_ASSERT(info, "Invalid argument.");

  if (info->c_json) {
    ten_json_destroy(info->c_json);
    info->c_json = NULL;
  }

  info->py_cb_func = NULL;

  TEN_FREE(info);
}

static void ten_env_notify_send_json(ten_env_t *ten_env, void *user_data) {
  TEN_ASSERT(user_data, "Invalid argument.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_notify_send_json_info_t *notify_info = user_data;

  ten_error_t err;
  ten_error_init(&err);

  TEN_UNUSED bool res = false;
  if (notify_info->py_cb_func == NULL) {
    res = ten_env_send_json(ten_env, notify_info->c_json, NULL, NULL, NULL);
  } else {
    res =
        ten_env_send_json(ten_env, notify_info->c_json, proxy_send_xxx_callback,
                          notify_info->py_cb_func, NULL);
  }

  ten_error_deinit(&err);

  ten_env_notify_send_json_info_destroy(notify_info);
}

PyObject *ten_py_ten_env_send_json(PyObject *self, PyObject *args) {
  ten_py_ten_env_t *py_ten = (ten_py_ten_env_t *)self;
  TEN_ASSERT(py_ten && ten_py_ten_env_check_integrity(py_ten),
             "Invalid argument.");

  bool success = true;

  ten_error_t err;
  ten_error_init(&err);

  char *json_str = NULL;
  PyObject *cb_func = NULL;

  if (!PyArg_ParseTuple(args, "sO", &json_str, &cb_func)) {
    success = false;
    goto done;
  }

  // Check if cb_func is callable.
  if (!PyCallable_Check(cb_func)) {
    cb_func = NULL;
  }

  ten_json_t *json = ten_json_from_string(json_str, &err);
  if (!json) {
    ten_py_raise_py_value_error_exception(ten_error_errmsg(&err));

    success = false;
    goto done;
  }

  ten_env_notify_send_json_info_t *notify_info =
      ten_env_notify_send_json_info_create(json, cb_func);
  ten_json_destroy(json);

  if (!ten_env_proxy_notify(py_ten->c_ten_env_proxy, ten_env_notify_send_json,
                            notify_info, false, &err)) {
    ten_py_raise_py_runtime_error_exception(ten_error_errmsg(&err));

    ten_env_notify_send_json_info_destroy(notify_info);
    success = false;
    goto done;
  }

done:
  ten_error_deinit(&err);

  if (success) {
    Py_RETURN_NONE;
  } else {
    return NULL;
  }
}
