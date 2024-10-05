//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <string.h>

#include "include_internal/ten_runtime/binding/python/common/error.h"
#include "include_internal/ten_runtime/binding/python/ten_env/ten_env.h"
#include "ten_runtime/ten_env/internal/metadata.h"
#include "ten_runtime/ten_env_proxy/ten_env_proxy.h"
#include "ten_utils/macro/memory.h"

typedef struct ten_env_notify_init_property_info_t {
  ten_string_t value;
  ten_event_t *completed;
} ten_env_notify_init_property_info_t;

static ten_env_notify_init_property_info_t *
ten_env_notify_init_property_info_create(const void *value, size_t value_len) {
  ten_env_notify_init_property_info_t *info =
      TEN_MALLOC(sizeof(ten_env_notify_init_property_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  ten_string_init_formatted(&info->value, "%.*s", value_len, value);
  info->completed = ten_event_create(0, 1);

  return info;
}

static void ten_env_notify_init_property_info_destroy(
    ten_env_notify_init_property_info_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  ten_string_deinit(&self->value);
  ten_event_destroy(self->completed);

  TEN_FREE(self);
}

static void ten_env_proxy_notify_init_property_from_json(ten_env_t *ten_env,
                                                         void *user_data) {
  TEN_ASSERT(user_data, "Invalid argument.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_notify_init_property_info_t *info = user_data;
  TEN_ASSERT(info, "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  ten_env_init_property_from_json(ten_env, ten_string_get_raw_str(&info->value),
                                  &err);

  ten_event_set(info->completed);

  ten_error_deinit(&err);
}

PyObject *ten_py_ten_env_init_property_from_json(PyObject *self,
                                                 PyObject *args) {
  ten_py_ten_env_t *py_ten_env = (ten_py_ten_env_t *)self;
  TEN_ASSERT(py_ten_env && ten_py_ten_env_check_integrity(py_ten_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 1) {
    return ten_py_raise_py_value_error_exception(
        "Invalid argument count when ten_env.init_property_from_json.");
  }

  const char *json_str = NULL;
  if (!PyArg_ParseTuple(args, "s", &json_str)) {
    return ten_py_raise_py_value_error_exception(
        "Failed to parse arguments when "
        "ten_env.init_property_from_json.");
  }

  ten_error_t err;
  ten_error_init(&err);

  ten_env_notify_init_property_info_t *info =
      ten_env_notify_init_property_info_create(json_str, strlen(json_str));

  if (!ten_env_proxy_notify(py_ten_env->c_ten_env_proxy,
                            ten_env_proxy_notify_init_property_from_json, info,
                            false, NULL)) {
    goto done;
  }

  PyThreadState *saved_py_thread_state = PyEval_SaveThread();
  ten_event_wait(info->completed, -1);
  PyEval_RestoreThread(saved_py_thread_state);

done:
  ten_env_notify_init_property_info_destroy(info);
  ten_error_deinit(&err);

  Py_RETURN_NONE;
}
