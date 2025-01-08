//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/nodejs/msg/msg.h"

#include "include_internal/axis_runtime/binding/nodejs/common/common.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_utils/lib/buf.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/memory.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_get.h"

void axis_nodejs_msg_init_from_c_msg(axis_nodejs_msg_t *self,
                                    axis_shared_ptr_t *msg) {
  axis_ASSERT(self, "Should not happen.");
  axis_ASSERT(msg && axis_msg_check_integrity(msg), "Should not happen.");

  axis_signature_set(&self->signature, axis_NODEJS_MSG_SIGNATURE);

  self->msg = axis_shared_ptr_clone(msg);
}

void axis_nodejs_msg_deinit(axis_nodejs_msg_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (self->msg) {
    axis_shared_ptr_destroy(self->msg);
    self->msg = NULL;
  }

  axis_signature_set(&self->signature, 0);
}

static napi_value axis_nodejs_msg_get_name(napi_env env,
                                          napi_callback_info info) {
  const size_t argc = 1;
  napi_value args[argc];  // this
  if (!axis_nodejs_get_js_func_args(env, info, args, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Incorrect number of parameters passed.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Should not happen.");
    return js_undefined(env);
  }

  axis_nodejs_msg_t *msg_bridge = NULL;
  napi_status status = napi_unwrap(env, args[0], (void **)&msg_bridge);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && msg_bridge != NULL,
                                "Failed to get msg bridge: %d", status);
  axis_ASSERT(msg_bridge, "Should not happen.");

  axis_shared_ptr_t *msg = msg_bridge->msg;
  axis_ASSERT(msg && axis_msg_check_integrity(msg), "Should not happen.");

  const char *name = axis_msg_get_name(msg);
  axis_ASSERT(name, "Should not happen.");

  napi_value js_msg_name = NULL;
  status = napi_create_string_utf8(env, name, NAPI_AUTO_LENGTH, &js_msg_name);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && js_msg_name != NULL,
                                "Failed to create JS string: %d", status);

  return js_msg_name;
}

static napi_value axis_nodejs_msg_set_dest(napi_env env,
                                          napi_callback_info info) {
  const size_t argc = 5;
  napi_value args[argc];  // this, app_uri, graph_id, extension_group, extension
  if (!axis_nodejs_get_js_func_args(env, info, args, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Incorrect number of parameters passed.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Should not happen.");
    return js_undefined(env);
  }

  axis_nodejs_msg_t *msg_bridge = NULL;
  napi_status status = napi_unwrap(env, args[0], (void **)&msg_bridge);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && msg_bridge != NULL,
                                "Failed to get msg bridge: %d", status);
  axis_ASSERT(msg_bridge, "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  axis_string_t app_uri;
  axis_string_init(&app_uri);

  axis_string_t graph_id;
  axis_string_init(&graph_id);

  axis_string_t extension_group;
  axis_string_init(&extension_group);

  axis_string_t extension;
  axis_string_init(&extension);

  if (!is_js_undefined(env, args[1])) {
    bool rc = axis_nodejs_get_str_from_js(env, args[1], &app_uri);
    RETURN_UNDEFINED_IF_NAPI_FAIL(rc, "Failed to get app URI", NULL);
  }

  if (!is_js_undefined(env, args[2])) {
    bool rc = axis_nodejs_get_str_from_js(env, args[2], &graph_id);
    RETURN_UNDEFINED_IF_NAPI_FAIL(rc, "Failed to get graph ID", NULL);
  }

  if (!is_js_undefined(env, args[3])) {
    bool rc = axis_nodejs_get_str_from_js(env, args[3], &extension_group);
    RETURN_UNDEFINED_IF_NAPI_FAIL(rc, "Failed to get extension group", NULL);
  }

  if (!is_js_undefined(env, args[4])) {
    bool rc = axis_nodejs_get_str_from_js(env, args[4], &extension);
    RETURN_UNDEFINED_IF_NAPI_FAIL(rc, "Failed to get extension", NULL);
  }

  bool rc = axis_msg_clear_and_set_dest(
      msg_bridge->msg, axis_string_get_raw_str(&app_uri),
      axis_string_get_raw_str(&graph_id),
      axis_string_get_raw_str(&extension_group),
      axis_string_get_raw_str(&extension), &err);
  if (!rc) {
    axis_string_t code_str;
    axis_string_init_formatted(&code_str, "%d", axis_error_errno(&err));

    napi_throw_error(env, axis_string_get_raw_str(&code_str),
                     axis_error_errmsg(&err));

    axis_string_deinit(&code_str);
  }

  axis_string_deinit(&app_uri);
  axis_string_deinit(&graph_id);
  axis_string_deinit(&extension_group);
  axis_string_deinit(&extension);
  axis_error_deinit(&err);

  return js_undefined(env);
}

static napi_value axis_nodejs_msg_set_property_from_json(
    napi_env env, napi_callback_info info) {
  const size_t argc = 3;
  napi_value args[argc];  // this, path, json_str
  if (!axis_nodejs_get_js_func_args(env, info, args, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Incorrect number of parameters passed.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Should not happen.");
    return js_undefined(env);
  }

  axis_nodejs_msg_t *msg_bridge = NULL;
  napi_status status = napi_unwrap(env, args[0], (void **)&msg_bridge);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && msg_bridge != NULL,
                                "Failed to get msg bridge: %d", status);
  axis_ASSERT(msg_bridge, "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  axis_string_t path;
  axis_string_init(&path);

  axis_json_t *c_json = NULL;

  bool rc = axis_nodejs_get_str_from_js(env, args[1], &path);
  RETURN_UNDEFINED_IF_NAPI_FAIL(rc, "Failed to get property path", NULL);

  axis_string_t json_str;
  axis_string_init(&json_str);

  rc = axis_nodejs_get_str_from_js(env, args[2], &json_str);
  RETURN_UNDEFINED_IF_NAPI_FAIL(rc, "Failed to get property value JSON", NULL);

  c_json = axis_json_from_string(axis_string_get_raw_str(&json_str), &err);
  if (!c_json) {
    axis_string_t code_str;
    axis_string_init_formatted(&code_str, "%d", axis_error_errno(&err));

    napi_throw_error(env, axis_string_get_raw_str(&code_str),
                     axis_error_errmsg(&err));

    axis_string_deinit(&code_str);

    goto done;
  }

  axis_value_t *value = axis_value_from_json(c_json);

  rc = axis_msg_set_property(msg_bridge->msg, axis_string_get_raw_str(&path),
                            value, &err);
  if (!rc) {
    axis_string_t code_str;
    axis_string_init_formatted(&code_str, "%d", axis_error_errno(&err));

    napi_throw_error(env, axis_string_get_raw_str(&code_str),
                     axis_error_errmsg(&err));

    axis_string_deinit(&code_str);
  }

done:
  axis_string_deinit(&path);
  axis_string_deinit(&json_str);
  axis_error_deinit(&err);
  if (c_json) {
    axis_json_destroy(c_json);
  }

  return js_undefined(env);
}

static napi_value axis_nodejs_msg_get_property_to_json(napi_env env,
                                                      napi_callback_info info) {
  const size_t argc = 2;
  napi_value args[argc];  // this, path
  if (!axis_nodejs_get_js_func_args(env, info, args, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Incorrect number of parameters passed.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Should not happen.");
    return js_undefined(env);
  }

  axis_nodejs_msg_t *msg_bridge = NULL;
  napi_status status = napi_unwrap(env, args[0], (void **)&msg_bridge);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && msg_bridge != NULL,
                                "Failed to get msg bridge: %d", status);
  axis_ASSERT(msg_bridge, "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  axis_string_t path;
  axis_string_init(&path);

  napi_value js_res = NULL;

  bool rc = axis_nodejs_get_str_from_js(env, args[1], &path);
  RETURN_UNDEFINED_IF_NAPI_FAIL(rc, "Failed to get property path", NULL);

  axis_value_t *c_value = axis_msg_peek_property(
      msg_bridge->msg, axis_string_get_raw_str(&path), &err);
  if (!c_value) {
    axis_string_t code_str;
    axis_string_init_formatted(&code_str, "%d", axis_error_errno(&err));

    napi_throw_error(env, axis_string_get_raw_str(&code_str),
                     axis_error_errmsg(&err));

    axis_string_deinit(&code_str);
    goto done;
  }

  axis_json_t *c_json = axis_value_to_json(c_value);
  axis_ASSERT(c_json, "Should not happen.");

  bool must_free = false;
  const char *json_str = axis_json_to_string(c_json, NULL, &must_free);
  axis_ASSERT(json_str, "Should not happen.");

  axis_json_destroy(c_json);

  status = napi_create_string_utf8(env, json_str, NAPI_AUTO_LENGTH, &js_res);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && js_res != NULL,
                                "Failed to create JS string: %d", status);

  if (must_free) {
    axis_FREE(json_str);
  }

done:
  axis_string_deinit(&path);
  axis_error_deinit(&err);

  if (!js_res) {
    js_res = js_undefined(env);
  }

  return js_res;
}

static napi_value axis_nodejs_msg_set_property_number(napi_env env,
                                                     napi_callback_info info) {
  const size_t argc = 3;
  napi_value args[argc];  // this, path, value
  if (!axis_nodejs_get_js_func_args(env, info, args, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Incorrect number of parameters passed.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Should not happen.");
    return js_undefined(env);
  }

  axis_nodejs_msg_t *msg_bridge = NULL;
  napi_status status = napi_unwrap(env, args[0], (void **)&msg_bridge);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && msg_bridge != NULL,
                                "Failed to get msg bridge: %d", status);
  axis_ASSERT(msg_bridge, "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  axis_string_t path;
  axis_string_init(&path);

  napi_value js_value = args[2];
  double value = 0;
  status = napi_get_value_double(env, js_value, &value);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok, "Failed to get value", NULL);

  bool rc = axis_nodejs_get_str_from_js(env, args[1], &path);
  RETURN_UNDEFINED_IF_NAPI_FAIL(rc, "Failed to get property path", NULL);

  axis_value_t *c_value = axis_value_create_float64(value);
  axis_ASSERT(c_value, "Should not happen.");

  rc = axis_msg_set_property(msg_bridge->msg, axis_string_get_raw_str(&path),
                            c_value, &err);
  if (!rc) {
    axis_string_t code_str;
    axis_string_init_formatted(&code_str, "%d", axis_error_errno(&err));

    napi_throw_error(env, axis_string_get_raw_str(&code_str),
                     axis_error_errmsg(&err));

    axis_string_deinit(&code_str);
  }

  axis_string_deinit(&path);
  axis_error_deinit(&err);

  return js_undefined(env);
}

static napi_value axis_nodejs_msg_get_property_number(napi_env env,
                                                     napi_callback_info info) {
  const size_t argc = 2;
  napi_value args[argc];  // this, path
  if (!axis_nodejs_get_js_func_args(env, info, args, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Incorrect number of parameters passed.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Should not happen.");
    return js_undefined(env);
  }

  axis_nodejs_msg_t *msg_bridge = NULL;
  napi_status status = napi_unwrap(env, args[0], (void **)&msg_bridge);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && msg_bridge != NULL,
                                "Failed to get msg bridge: %d", status);
  axis_ASSERT(msg_bridge, "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  axis_string_t path;
  axis_string_init(&path);

  napi_value js_res = NULL;

  bool rc = axis_nodejs_get_str_from_js(env, args[1], &path);
  RETURN_UNDEFINED_IF_NAPI_FAIL(rc, "Failed to get property path", NULL);

  axis_value_t *c_value = axis_msg_peek_property(
      msg_bridge->msg, axis_string_get_raw_str(&path), &err);
  if (!c_value) {
    axis_string_t code_str;
    axis_string_init_formatted(&code_str, "%d", axis_error_errno(&err));

    napi_throw_error(env, axis_string_get_raw_str(&code_str),
                     axis_error_errmsg(&err));

    axis_string_deinit(&code_str);
    goto done;
  }

  double value = axis_value_get_float64(c_value, &err);
  if (axis_error_errno(&err) != axis_ERRNO_OK) {
    axis_string_t code_str;
    axis_string_init_formatted(&code_str, "%d", axis_error_errno(&err));

    napi_throw_error(env, axis_string_get_raw_str(&code_str),
                     axis_error_errmsg(&err));

    axis_string_deinit(&code_str);
    goto done;
  }

  status = napi_create_double(env, value, &js_res);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && js_res != NULL,
                                "Failed to create JS number: %d", status);

done:
  axis_string_deinit(&path);
  axis_error_deinit(&err);

  if (!js_res) {
    js_res = js_undefined(env);
  }

  return js_res;
}

static napi_value axis_nodejs_msg_set_property_string(napi_env env,
                                                     napi_callback_info info) {
  const size_t argc = 3;
  napi_value args[argc];  // this, path, value
  if (!axis_nodejs_get_js_func_args(env, info, args, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Incorrect number of parameters passed.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Should not happen.");
  }

  axis_nodejs_msg_t *msg_bridge = NULL;
  napi_status status = napi_unwrap(env, args[0], (void **)&msg_bridge);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && msg_bridge != NULL,
                                "Failed to get msg bridge: %d", status);
  axis_ASSERT(msg_bridge, "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  axis_string_t path;
  axis_string_init(&path);

  bool rc = axis_nodejs_get_str_from_js(env, args[1], &path);
  RETURN_UNDEFINED_IF_NAPI_FAIL(rc, "Failed to get property path", NULL);

  axis_string_t value_str;
  axis_string_init(&value_str);

  rc = axis_nodejs_get_str_from_js(env, args[2], &value_str);
  RETURN_UNDEFINED_IF_NAPI_FAIL(rc, "Failed to get property value", NULL);

  axis_value_t *c_value =
      axis_value_create_string(axis_string_get_raw_str(&value_str));
  axis_ASSERT(c_value, "Failed to create string value.");

  rc = axis_msg_set_property(msg_bridge->msg, axis_string_get_raw_str(&path),
                            c_value, &err);
  if (!rc) {
    axis_string_t code_str;
    axis_string_init_formatted(&code_str, "%d", axis_error_errno(&err));

    napi_throw_error(env, axis_string_get_raw_str(&code_str),
                     axis_error_errmsg(&err));

    axis_string_deinit(&code_str);
  }

  axis_string_deinit(&path);
  axis_string_deinit(&value_str);
  axis_error_deinit(&err);

  return js_undefined(env);
}

static napi_value axis_nodejs_msg_get_property_string(napi_env env,
                                                     napi_callback_info info) {
  const size_t argc = 2;
  napi_value args[argc];  // this, path
  if (!axis_nodejs_get_js_func_args(env, info, args, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Incorrect number of parameters passed.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Should not happen.");
    return js_undefined(env);
  }

  axis_nodejs_msg_t *msg_bridge = NULL;
  napi_status status = napi_unwrap(env, args[0], (void **)&msg_bridge);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && msg_bridge != NULL,
                                "Failed to get msg bridge: %d", status);
  axis_ASSERT(msg_bridge, "Should not happen.");

  napi_value js_res = NULL;

  axis_error_t err;
  axis_error_init(&err);

  axis_string_t path;
  axis_string_init(&path);

  bool rc = axis_nodejs_get_str_from_js(env, args[1], &path);
  RETURN_UNDEFINED_IF_NAPI_FAIL(rc, "Failed to get property path", NULL);

  axis_value_t *c_value = axis_msg_peek_property(
      msg_bridge->msg, axis_string_get_raw_str(&path), &err);
  if (!c_value) {
    axis_string_t code_str;
    axis_string_init_formatted(&code_str, "%d", axis_error_errno(&err));

    napi_throw_error(env, axis_string_get_raw_str(&code_str),
                     axis_error_errmsg(&err));

    axis_string_deinit(&code_str);
    goto done;
  }

  const char *value = axis_value_peek_raw_str(c_value, &err);
  if (!value) {
    axis_string_t code_str;
    axis_string_init_formatted(&code_str, "%d", axis_error_errno(&err));

    napi_throw_error(env, axis_string_get_raw_str(&code_str),
                     axis_error_errmsg(&err));

    axis_string_deinit(&code_str);
    goto done;
  }

  status = napi_create_string_utf8(env, value, NAPI_AUTO_LENGTH, &js_res);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && js_res != NULL,
                                "Failed to create JS string: %d", status);

done:
  axis_string_deinit(&path);
  axis_error_deinit(&err);

  if (!js_res) {
    js_res = js_undefined(env);
  }

  return js_res;
}

static napi_value axis_nodejs_msg_set_property_bool(napi_env env,
                                                   napi_callback_info info) {
  const size_t argc = 3;
  napi_value args[argc];  // this, path, value
  if (!axis_nodejs_get_js_func_args(env, info, args, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Incorrect number of parameters passed.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Should not happen.");
  }

  axis_nodejs_msg_t *msg_bridge = NULL;
  napi_status status = napi_unwrap(env, args[0], (void **)&msg_bridge);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && msg_bridge != NULL,
                                "Failed to get msg bridge: %d", status);
  axis_ASSERT(msg_bridge, "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  axis_string_t path;
  axis_string_init(&path);

  bool rc = axis_nodejs_get_str_from_js(env, args[1], &path);
  RETURN_UNDEFINED_IF_NAPI_FAIL(rc, "Failed to get property path", NULL);

  napi_value js_value = args[2];
  bool value = false;
  status = napi_get_value_bool(env, js_value, &value);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok, "Failed to get value", NULL);

  axis_value_t *c_value = axis_value_create_bool(value);
  axis_ASSERT(c_value, "Failed to create bool value.");

  rc = axis_msg_set_property(msg_bridge->msg, axis_string_get_raw_str(&path),
                            c_value, &err);
  if (!rc) {
    axis_string_t code_str;
    axis_string_init_formatted(&code_str, "%d", axis_error_errno(&err));

    napi_throw_error(env, axis_string_get_raw_str(&code_str),
                     axis_error_errmsg(&err));

    axis_string_deinit(&code_str);
  }

  axis_string_deinit(&path);
  axis_error_deinit(&err);

  return js_undefined(env);
}

static napi_value axis_nodejs_msg_get_property_bool(napi_env env,
                                                   napi_callback_info info) {
  const size_t argc = 2;
  napi_value args[argc];  // this, path
  if (!axis_nodejs_get_js_func_args(env, info, args, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Incorrect number of parameters passed.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Should not happen.");
    return js_undefined(env);
  }

  axis_nodejs_msg_t *msg_bridge = NULL;
  napi_status status = napi_unwrap(env, args[0], (void **)&msg_bridge);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && msg_bridge != NULL,
                                "Failed to get msg bridge: %d", status);
  axis_ASSERT(msg_bridge, "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  axis_string_t path;
  axis_string_init(&path);

  napi_value js_res = NULL;

  bool rc = axis_nodejs_get_str_from_js(env, args[1], &path);
  RETURN_UNDEFINED_IF_NAPI_FAIL(rc, "Failed to get property path", NULL);

  axis_value_t *c_value = axis_msg_peek_property(
      msg_bridge->msg, axis_string_get_raw_str(&path), &err);
  if (!c_value) {
    axis_string_t code_str;
    axis_string_init_formatted(&code_str, "%d", axis_error_errno(&err));

    napi_throw_error(env, axis_string_get_raw_str(&code_str),
                     axis_error_errmsg(&err));

    axis_string_deinit(&code_str);
    goto done;
  }

  bool value = axis_value_get_bool(c_value, &err);
  if (axis_error_errno(&err) != axis_ERRNO_OK) {
    axis_string_t code_str;
    axis_string_init_formatted(&code_str, "%d", axis_error_errno(&err));

    napi_throw_error(env, axis_string_get_raw_str(&code_str),
                     axis_error_errmsg(&err));

    axis_string_deinit(&code_str);
    goto done;
  }

  status = napi_get_boolean(env, value, &js_res);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && js_res != NULL,
                                "Failed to create JS boolean: %d", status);

done:
  axis_string_deinit(&path);
  axis_error_deinit(&err);

  if (!js_res) {
    js_res = js_undefined(env);
  }

  return js_res;
}

static napi_value axis_nodejs_msg_set_property_buf(napi_env env,
                                                  napi_callback_info info) {
  const size_t argc = 3;
  napi_value args[argc];  // this, path, value
  if (!axis_nodejs_get_js_func_args(env, info, args, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Incorrect number of parameters passed.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Should not happen.");
  }

  axis_nodejs_msg_t *msg_bridge = NULL;
  napi_status status = napi_unwrap(env, args[0], (void **)&msg_bridge);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && msg_bridge != NULL,
                                "Failed to get msg bridge: %d", status);
  axis_ASSERT(msg_bridge, "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  axis_string_t path;
  axis_string_init(&path);

  bool rc = axis_nodejs_get_str_from_js(env, args[1], &path);
  RETURN_UNDEFINED_IF_NAPI_FAIL(rc, "Failed to get property path", NULL);

  void *data = NULL;
  size_t size = 0;
  status = napi_get_arraybuffer_info(env, args[2], &data, &size);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok, "Failed to get buffer",
                                NULL);

  axis_buf_t buf;
  axis_buf_init_with_copying_data(&buf, data, size);

  axis_value_t *c_value = axis_value_create_buf_with_move(buf);
  axis_ASSERT(c_value && axis_value_check_integrity(c_value),
             "Failed to create buffer value.");

  rc = axis_msg_set_property(msg_bridge->msg, axis_string_get_raw_str(&path),
                            c_value, &err);
  if (!rc) {
    axis_string_t code_str;
    axis_string_init_formatted(&code_str, "%d", axis_error_errno(&err));

    napi_throw_error(env, axis_string_get_raw_str(&code_str),
                     axis_error_errmsg(&err));

    axis_string_deinit(&code_str);
  }

  axis_string_deinit(&path);
  axis_error_deinit(&err);

  return js_undefined(env);
}

static napi_value axis_nodejs_msg_get_property_buf(napi_env env,
                                                  napi_callback_info info) {
  const size_t argc = 2;
  napi_value args[argc];  // this, path
  if (!axis_nodejs_get_js_func_args(env, info, args, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Incorrect number of parameters passed.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Should not happen.");
    return js_undefined(env);
  }

  axis_nodejs_msg_t *msg_bridge = NULL;
  napi_status status = napi_unwrap(env, args[0], (void **)&msg_bridge);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && msg_bridge != NULL,
                                "Failed to get msg bridge: %d", status);
  axis_ASSERT(msg_bridge, "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  axis_string_t path;
  axis_string_init(&path);

  napi_value js_res = NULL;

  bool rc = axis_nodejs_get_str_from_js(env, args[1], &path);
  RETURN_UNDEFINED_IF_NAPI_FAIL(rc, "Failed to get property path", NULL);

  axis_value_t *c_value = axis_msg_peek_property(
      msg_bridge->msg, axis_string_get_raw_str(&path), &err);
  if (!c_value) {
    axis_string_t code_str;
    axis_string_init_formatted(&code_str, "%d", axis_error_errno(&err));

    napi_throw_error(env, axis_string_get_raw_str(&code_str),
                     axis_error_errmsg(&err));

    axis_string_deinit(&code_str);
    goto done;
  }

  if (!axis_value_is_buf(c_value)) {
    napi_throw_error(env, "EINVAL", "Property is not a buffer.");
    goto done;
  }

  axis_buf_t *buf = axis_value_peek_buf(c_value);
  axis_ASSERT(buf, "Should not happen.");

  status = napi_create_buffer_copy(env, axis_buf_get_size(buf),
                                   axis_buf_get_data(buf), NULL, &js_res);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && js_res != NULL,
                                "Failed to create JS buffer: %d", status);

done:
  axis_string_deinit(&path);
  axis_error_deinit(&err);

  if (!js_res) {
    js_res = js_undefined(env);
  }

  return js_res;
}

napi_value axis_nodejs_msg_module_init(napi_env env, napi_value exports) {
  EXPORT_FUNC(env, exports, axis_nodejs_msg_get_name);
  EXPORT_FUNC(env, exports, axis_nodejs_msg_set_dest);
  EXPORT_FUNC(env, exports, axis_nodejs_msg_set_property_from_json);
  EXPORT_FUNC(env, exports, axis_nodejs_msg_get_property_to_json);
  EXPORT_FUNC(env, exports, axis_nodejs_msg_set_property_number);
  EXPORT_FUNC(env, exports, axis_nodejs_msg_get_property_number);
  EXPORT_FUNC(env, exports, axis_nodejs_msg_set_property_string);
  EXPORT_FUNC(env, exports, axis_nodejs_msg_get_property_string);
  EXPORT_FUNC(env, exports, axis_nodejs_msg_set_property_bool);
  EXPORT_FUNC(env, exports, axis_nodejs_msg_get_property_bool);
  EXPORT_FUNC(env, exports, axis_nodejs_msg_set_property_buf);
  EXPORT_FUNC(env, exports, axis_nodejs_msg_get_property_buf);

  return exports;
}
