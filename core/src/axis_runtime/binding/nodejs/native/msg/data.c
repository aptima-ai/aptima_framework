//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/nodejs/msg/data.h"

#include <string.h>

#include "include_internal/axis_runtime/msg/msg.h"
#include "node_api.h"
#include "axis_runtime/msg/data/data.h"
#include "axis_utils/lib/buf.h"
#include "axis_utils/macro/memory.h"

static napi_ref js_data_constructor_ref = NULL;  // NOLINT

static napi_value axis_nodejs_data_register_class(napi_env env,
                                                 napi_callback_info info) {
  const size_t argc = 1;
  napi_value args[argc];  // Data constructor
  if (!axis_nodejs_get_js_func_args(env, info, args, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Incorrect number of parameters passed.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Should not happen.");
    return NULL;
  }

  napi_status status =
      napi_create_reference(env, args[0], 1, &js_data_constructor_ref);
  if (status != napi_ok) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Failed to create JS reference to JS Data constructor.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Failed to create JS reference to JS Data constructor: %d",
               status);
  }

  return js_undefined(env);
}

static void axis_nodejs_data_destroy(axis_nodejs_data_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_nodejs_msg_deinit(&self->msg);

  axis_FREE(self);
}

static void axis_nodejs_data_finalize(napi_env env, void *data, void *hint) {
  axis_nodejs_data_t *data_bridge = data;
  axis_ASSERT(data_bridge, "Should not happen.");

  napi_delete_reference(env, data_bridge->msg.bridge.js_instance_ref);

  axis_nodejs_data_destroy(data_bridge);
}

static napi_value axis_nodejs_data_create(napi_env env,
                                         napi_callback_info info) {
  const size_t argc = 2;
  napi_value args[argc];  // this, data_name
  if (!axis_nodejs_get_js_func_args(env, info, args, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Incorrect number of parameters passed.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Should not happen.");
  }

  axis_string_t data_name;
  axis_string_init(&data_name);

  bool rc = axis_nodejs_get_str_from_js(env, args[1], &data_name);
  if (!rc) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH, "Failed to get data_name.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Should not happen.");
  }

  axis_error_t error;
  axis_error_init(&error);

  axis_shared_ptr_t *c_data =
      axis_data_create(axis_string_get_raw_str(&data_name), &error);
  axis_ASSERT(c_data, "Failed to create data.");

  axis_string_deinit(&data_name);

  axis_nodejs_data_t *data_bridge = axis_MALLOC(sizeof(axis_nodejs_data_t));
  axis_ASSERT(data_bridge, "Failed to allocate memory.");

  axis_nodejs_msg_init_from_c_msg(&data_bridge->msg, c_data);
  // Decrement the reference count of c_data to indicate that the JS data takes
  // the full ownership of this c_data, in other words, when the JS data is
  // finalized, its C data would be destroyed, too.
  axis_shared_ptr_destroy(c_data);

  napi_status status =
      napi_wrap(env, args[0], data_bridge, axis_nodejs_data_finalize, NULL,
                &data_bridge->msg.bridge.js_instance_ref);
  if (status != napi_ok) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH, "Failed to wrap JS data object.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Failed to wrap JS data object: %d", status);
  }

  return js_undefined(env);
}

napi_value axis_nodejs_data_wrap(napi_env env, axis_shared_ptr_t *data) {
  axis_ASSERT(data && axis_msg_check_integrity(data), "Should not happen.");

  axis_nodejs_data_t *data_bridge = axis_MALLOC(sizeof(axis_nodejs_data_t));
  axis_ASSERT(data_bridge, "Failed to allocate memory.");

  axis_nodejs_msg_init_from_c_msg(&data_bridge->msg, data);

  napi_value js_msg_name = NULL;
  napi_value js_create_shell_only_flag = NULL;

  const char *msg_name = axis_msg_get_name(data);
  axis_ASSERT(msg_name, "Should not happen.");

  napi_status status =
      napi_create_string_utf8(env, msg_name, NAPI_AUTO_LENGTH, &js_msg_name);
  ASSERT_IF_NAPI_FAIL(status == napi_ok && js_msg_name != NULL,
                      "Failed to create JS string: %d", status);

  status = napi_get_boolean(env, true, &js_create_shell_only_flag);
  ASSERT_IF_NAPI_FAIL(status == napi_ok && js_create_shell_only_flag != NULL,
                      "Failed to create JS boolean: %d", status);

  napi_value argv[] = {js_msg_name, js_create_shell_only_flag};

  napi_value js_data = axis_nodejs_create_new_js_object_and_wrap(
      env, js_data_constructor_ref, data_bridge, axis_nodejs_data_finalize,
      &data_bridge->msg.bridge.js_instance_ref, 2, argv);
  ASSERT_IF_NAPI_FAIL(js_data != NULL, "Failed to create JS Data object.");

  return js_data;
}

static napi_value axis_nodejs_data_alloc_buf(napi_env env,
                                            napi_callback_info info) {
  const size_t argc = 2;
  napi_value args[argc];  // this, size

  if (!axis_nodejs_get_js_func_args(env, info, args, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Incorrect number of parameters passed.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Should not happen.");
  }

  axis_nodejs_data_t *data_bridge = NULL;
  napi_status status = napi_unwrap(env, args[0], (void **)&data_bridge);
  if (status != napi_ok) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH, "Failed to unwrap JS data object.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Failed to unwrap JS data object: %d", status);
  }

  uint32_t size = 0;
  status = napi_get_value_uint32(env, args[1], &size);
  if (status != napi_ok) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH, "Failed to get size.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Failed to get size: %d", status);
  }

  axis_data_alloc_buf(data_bridge->msg.msg, size);

  return js_undefined(env);
}
static napi_value axis_nodejs_data_lock_buf(napi_env env,
                                           napi_callback_info info) {
  const size_t argc = 1;
  napi_value args[argc];  // this

  if (!axis_nodejs_get_js_func_args(env, info, args, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Incorrect number of parameters passed.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Should not happen.");
  }

  axis_nodejs_data_t *data_bridge = NULL;
  napi_status status = napi_unwrap(env, args[0], (void **)&data_bridge);
  if (status != napi_ok) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH, "Failed to unwrap JS data object.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Failed to unwrap JS data object: %d", status);
  }

  axis_error_t err;
  axis_error_init(&err);

  if (!axis_msg_add_locked_res_buf(data_bridge->msg.msg,
                                  axis_data_peek_buf(data_bridge->msg.msg)->data,
                                  &err)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH, "Failed to lock buffer.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Failed to lock buffer.");
  }

  axis_buf_t *buf = axis_data_peek_buf(data_bridge->msg.msg);

  napi_value js_buf = NULL;
  status = napi_create_external_arraybuffer(
      env, axis_buf_get_data(buf), axis_buf_get_size(buf), NULL, NULL, &js_buf);
  if (status != napi_ok) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH, "Failed to create JS buffer.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Failed to create JS buffer: %d", status);
  }

  return js_buf;
}
static napi_value axis_nodejs_data_unlock_buf(napi_env env,
                                             napi_callback_info info) {
  const size_t argc = 2;
  napi_value args[argc];  // this, buffer

  if (!axis_nodejs_get_js_func_args(env, info, args, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Incorrect number of parameters passed.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Should not happen.");
  }

  axis_nodejs_data_t *data_bridge = NULL;
  napi_status status = napi_unwrap(env, args[0], (void **)&data_bridge);
  if (status != napi_ok) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH, "Failed to unwrap JS data object.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Failed to unwrap JS data object: %d", status);
  }

  void *data = NULL;
  status = napi_get_arraybuffer_info(env, args[1], &data, NULL);
  if (status != napi_ok) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH, "Failed to get buffer.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Failed to get buffer: %d", status);
  }

  axis_error_t err;
  axis_error_init(&err);

  if (!axis_msg_remove_locked_res_buf(data_bridge->msg.msg, data, &err)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH, "Failed to unlock buffer.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Failed to unlock buffer.");
  }

  return js_undefined(env);
}

static napi_value axis_nodejs_data_get_buf(napi_env env,
                                          napi_callback_info info) {
  const size_t argc = 1;
  napi_value args[argc];  // this

  if (!axis_nodejs_get_js_func_args(env, info, args, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Incorrect number of parameters passed.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Should not happen.");
  }

  axis_nodejs_data_t *data_bridge = NULL;
  napi_status status = napi_unwrap(env, args[0], (void **)&data_bridge);
  if (status != napi_ok) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH, "Failed to unwrap JS data object.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Failed to unwrap JS data object: %d", status);
  }

  axis_buf_t *buf = axis_data_peek_buf(data_bridge->msg.msg);
  if (!buf) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH, "Failed to get buffer.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Failed to get buffer.");
  }

  napi_value js_buf = NULL;
  status = napi_create_buffer_copy(env, axis_buf_get_size(buf),
                                   axis_buf_get_data(buf), NULL, &js_buf);
  if (status != napi_ok) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH, "Failed to create JS buffer.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Failed to create JS buffer: %d", status);
  }

  return js_buf;
}

napi_value axis_nodejs_data_module_init(napi_env env, napi_value exports) {
  EXPORT_FUNC(env, exports, axis_nodejs_data_register_class);
  EXPORT_FUNC(env, exports, axis_nodejs_data_create);
  EXPORT_FUNC(env, exports, axis_nodejs_data_alloc_buf);
  EXPORT_FUNC(env, exports, axis_nodejs_data_lock_buf);
  EXPORT_FUNC(env, exports, axis_nodejs_data_unlock_buf);
  EXPORT_FUNC(env, exports, axis_nodejs_data_get_buf);

  return exports;
}
