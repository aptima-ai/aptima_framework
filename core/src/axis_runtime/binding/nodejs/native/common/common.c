//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/nodejs/common/common.h"

#include "axis_runtime/common/errno.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_get.h"
#include "axis_utils/value/value_is.h"

napi_value js_undefined(napi_env env) {
  axis_ASSERT(env, "Should not happen.");

  napi_value js_undefined_value = NULL;
  napi_status status = napi_get_undefined(env, &js_undefined_value);
  ASSERT_IF_NAPI_FAIL(status == napi_ok,
                      "Failed to get type JS undefined value: %d", status);

  return js_undefined_value;
}

bool is_js_undefined(napi_env env, napi_value value) {
  axis_ASSERT(env, "Should not happen.");

  napi_valuetype valuetype = 0;
  napi_status status = napi_typeof(env, value, &valuetype);
  ASSERT_IF_NAPI_FAIL(status == napi_ok,
                      "Failed to get type of JS instance: %d", status);

  return (valuetype == napi_undefined) ? true : false;
}

bool is_js_string(napi_env env, napi_value value) {
  axis_ASSERT(env, "Should not happen.");

  napi_valuetype valuetype = 0;
  napi_status status = napi_typeof(env, value, &valuetype);
  ASSERT_IF_NAPI_FAIL(status == napi_ok,
                      "Failed to get type of JS instance: %d", status);

  return (valuetype == napi_string) ? true : false;
}

bool axis_nodejs_get_js_func_args(napi_env env, napi_callback_info info,
                                 napi_value *args, size_t argc) {
  axis_ASSERT(env, "Should not happen.");
  axis_ASSERT(info, "Should not happen.");
  axis_ASSERT(args, "Should not happen.");

  size_t actual_argc = argc;
  napi_status status =
      napi_get_cb_info(env, info, &actual_argc, args, NULL, NULL);
  ASSERT_IF_NAPI_FAIL(status == napi_ok,
                      "Failed to get JS function arguments: %d", status);

  if (actual_argc != argc) {
    axis_string_t err;
    axis_string_init_formatted(&err, "Expected %zu arguments, got %zu arguments",
                              argc, actual_argc);

    axis_LOGE("%s", axis_string_get_raw_str(&err));

    status = napi_throw_error(env, "EINVAL", axis_string_get_raw_str(&err));
    ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to throw JS exception: %d",
                        status);

    axis_string_deinit(&err);
    return false;
  }

  return true;
}

bool axis_nodejs_get_str_from_js(napi_env env, napi_value val,
                                axis_string_t *str) {
  axis_ASSERT(env && val && str, "Should not happen.");

  napi_status status = napi_ok;

  if (is_js_string(env, val) == false) {
    status = napi_throw_error(env, "EINVAL", "Expected a string");
    ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to throw JS exception: %d",
                        status);
    return false;
  }

  size_t str_len = 0;
  status = napi_get_value_string_utf8(env, val, NULL, 0, &str_len);
  ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to get JS string length: %d",
                      status);

  axis_string_reserve(str, str_len + 1);

  status =
      napi_get_value_string_utf8(env, val, str->buf, str_len + 1, &str_len);
  ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to get JS string: %d", status);

  str->buf[str_len] = '\0';

  return true;
}

void axis_nodejs_report_and_clear_error_(napi_env env, napi_status orig_status,
                                        const char *func, int line) {
  axis_ASSERT(func, "Should not happen.");

  axis_LOGE("(%s:%d) Failed to invoke napi function, status: %d", func, line,
           orig_status);

  const napi_extended_error_info *error_info = NULL;
  napi_status status = napi_get_last_error_info((env), &error_info);
  ASSERT_IF_NAPI_FAIL(status == napi_ok,
                      "Failed to get napi last error info: %d", status);

  const char *err_message = error_info->error_message;

  axis_LOGE("napi error message: %s", err_message);

  // Check if there is any pending JS exception.
  bool pending = false;
  status = napi_is_exception_pending(env, &pending);
  ASSERT_IF_NAPI_FAIL(status == napi_ok,
                      "Failed to check if there is any pending JS exceptions "
                      "in the JS world: %d",
                      status);
  if (pending) {
    napi_value ex = NULL;
    status = napi_get_and_clear_last_exception(env, &ex);
    ASSERT_IF_NAPI_FAIL(status == napi_ok || ex != NULL,
                        "Failed to get latest JS exception: %d", status);

    napi_value ex_str = NULL;
    status = napi_coerce_to_string(env, ex, &ex_str);
    ASSERT_IF_NAPI_FAIL(status == napi_ok && ex_str != NULL,
                        "Failed to coerce JS exception string: %d", status);

    size_t str_size = 0;
    status = napi_get_value_string_utf8(env, ex_str, NULL, str_size, &str_size);
    ASSERT_IF_NAPI_FAIL(status == napi_ok,
                        "Failed to get the JS exception string length: %d",
                        status);

    char *buf = axis_MALLOC(str_size + 1);
    axis_ASSERT(buf, "Failed to allocate memory.");

    status = napi_get_value_string_utf8(env, ex_str, buf, str_size + 1, NULL);
    ASSERT_IF_NAPI_FAIL(status == napi_ok,
                        "Failed to get JS exception string: %d", status);

    buf[str_size] = '\0';
    axis_LOGE("Exception: %s", buf);

    axis_FREE(buf);

    // Trigger an 'uncaughtException' in JavaScript. Useful if an async callback
    // throws an exception with no way to recover.
    status = napi_fatal_exception(env, ex);
    ASSERT_IF_NAPI_FAIL(status == napi_ok,
                        "Failed to throw JS fatal exception: %d", status);
  } else {
    axis_LOGW("No pending exceptions when napi API failed.");

    // Encountering napi error but can not get any exceptions. It means JS
    // runtime went wrong (one possible reason is that the JS runtime is
    // closing), so we don't throw exceptions into JS runtime here.
  }
}

napi_value axis_nodejs_get_property(napi_env env, napi_value js_obj,
                                   const char *property_name) {
  axis_ASSERT(env, "Should not happen.");
  axis_ASSERT(property_name, "Should not happen.");

  napi_value key = NULL;
  napi_status status =
      napi_create_string_utf8(env, property_name, strlen(property_name), &key);
  ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to create JS string: %d",
                      status);

  napi_value value = NULL;
  status = napi_get_property(env, js_obj, key, &value);
  ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to get JS property: %d",
                      status);

  return value;
}

void axis_nodejs_export_func(napi_env env, napi_value exports,
                            const char *func_name, napi_callback func) {
  axis_ASSERT(func_name && strlen(func_name), "Should not happen.");

  napi_value fn = NULL;
  napi_status status =
      napi_create_function(env, func_name, NAPI_AUTO_LENGTH, func, NULL, &fn);
  ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to create JS function: %d",
                      status);

  status = napi_set_named_property(env, exports, func_name, fn);
  ASSERT_IF_NAPI_FAIL(
      status == napi_ok,
      "Failed to add newly created JS function to 'exports': %d", status);
}

/**
 * @brief Create a JavaScript object by using a 'constructor' function in the
 * JavaScript world, and binding the newly created JavaScript object to a native
 * object in 'bridge_obj'.
 */
napi_value axis_nodejs_create_new_js_object_and_wrap(
    napi_env env, napi_ref js_constructor_ref, void *bridge_obj,
    napi_finalize finalizer, napi_ref *bridge_weak_ref, size_t argc,
    const napi_value *argv) {
  axis_ASSERT(env, "Should not happen.");
  axis_ASSERT(js_constructor_ref, "Should not happen.");
  axis_ASSERT(bridge_obj, "Should not happen.");

  napi_value js_instance = NULL;

  // Get the JavaScript constructor function corresponding to the
  // 'constructor_ref'.
  napi_value js_constructor = NULL;
  napi_status status =
      napi_get_reference_value(env, js_constructor_ref, &js_constructor);
  GOTO_LABEL_IF_NAPI_FAIL(done, status == napi_ok && js_constructor != NULL,
                          "Failed to get JS constructor: %d", status);

  // Create a JS instance.
  status = napi_new_instance(env, js_constructor, argc, argv, &js_instance);
  GOTO_LABEL_IF_NAPI_FAIL(done, status == napi_ok,
                          "Failed to create JS instance: %d", status);

  // Wrap the native 'bridge_obj' to the newly created JS instance.
  status =
      napi_wrap(env, js_instance, bridge_obj, finalizer, NULL, bridge_weak_ref);
  GOTO_LABEL_IF_NAPI_FAIL(done, status == napi_ok,
                          "Failed to bind JS instance & bridge: %d", status);

done:
  return js_instance;
}

napi_value axis_nodejs_create_error(napi_env env, axis_error_t *error) {
  axis_ASSERT(env, "Should not happen.");
  axis_ASSERT(error, "Should not happen.");

  napi_value js_error = NULL;
  napi_value _code = NULL;
  napi_value _msg = NULL;

  axis_string_t code_str;
  axis_string_init_formatted(&code_str, "%d", axis_error_errno(error));

  napi_status status = napi_create_string_utf8(
      env, axis_string_get_raw_str(&code_str), NAPI_AUTO_LENGTH, &_code);
  axis_ASSERT(status == napi_ok, "Failed to create JS string.");

  axis_string_deinit(&code_str);

  status = napi_create_string_utf8(env, axis_error_errmsg(error),
                                   NAPI_AUTO_LENGTH, &_msg);
  axis_ASSERT(status == napi_ok, "Failed to create JS string.");

  status = napi_create_error(env, _code, _msg, &js_error);
  axis_ASSERT(status == napi_ok, "Failed to create JS error.");

  return js_error;
}

napi_value axis_nodejs_create_value_number(napi_env env, axis_value_t *value,
                                          axis_error_t *error) {
  axis_ASSERT(env, "Should not happen.");
  axis_ASSERT(value && axis_value_check_integrity(value), "Should not happen.");
  axis_ASSERT(error, "Should not happen.");

  napi_value js_value = NULL;
  napi_status status = napi_ok;

  switch (value->type) {
    case axis_TYPE_INVALID:
    case axis_TYPE_NULL:
    case axis_TYPE_BOOL:
    case axis_TYPE_STRING:
    case axis_TYPE_BUF:
    case axis_TYPE_ARRAY:
    case axis_TYPE_OBJECT:
    case axis_TYPE_PTR:
    case axis_TYPE_INT8: {
      status =
          napi_create_int32(env, axis_value_get_int8(value, error), &js_value);
      ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to create JS int8: %d",
                          status);
      break;
    }
    case axis_TYPE_INT16: {
      status =
          napi_create_int32(env, axis_value_get_int16(value, error), &js_value);
      ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to create JS int16: %d",
                          status);
      break;
    }
    case axis_TYPE_INT32: {
      status =
          napi_create_int32(env, axis_value_get_int32(value, error), &js_value);
      ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to create JS int32: %d",
                          status);
      break;
    }
    case axis_TYPE_INT64: {
      status =
          napi_create_int64(env, axis_value_get_int64(value, error), &js_value);
      ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to create JS int64: %d",
                          status);
      break;
    }
    case axis_TYPE_UINT8: {
      status =
          napi_create_uint32(env, axis_value_get_uint8(value, error), &js_value);
      ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to create JS uint8: %d",
                          status);
      break;
    }
    case axis_TYPE_UINT16: {
      status = napi_create_uint32(env, axis_value_get_uint16(value, error),
                                  &js_value);
      ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to create JS uint16: %d",
                          status);
      break;
    }
    case axis_TYPE_UINT32: {
      status = napi_create_uint32(env, axis_value_get_uint32(value, error),
                                  &js_value);
      ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to create JS uint32: %d",
                          status);
      break;
    }
    case axis_TYPE_UINT64: {
      // We need to convert uint64 to int64 first, because Javascript's number
      // type supports up to int64, and values exceeding that should use bigint.
      int64_t value_int64 = axis_value_get_int64(value, error);
      if (!axis_error_is_success(error)) {
        break;
      }

      status = napi_create_int64(env, value_int64, &js_value);
      ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to create JS uint64: %d",
                          status);

      break;
    }

    case axis_TYPE_FLOAT32: {
      status = napi_create_double(env, axis_value_get_float32(value, error),
                                  &js_value);
      ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to create JS float32: %d",
                          status);
      break;
    }
    case axis_TYPE_FLOAT64: {
      status = napi_create_double(env, axis_value_get_float64(value, error),
                                  &js_value);
      ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to create JS float64: %d",
                          status);
      break;
    }
  }

  return js_value;
}

napi_value axis_nodejs_create_value_string(napi_env env, axis_value_t *value,
                                          axis_error_t *error) {
  axis_ASSERT(env, "Should not happen.");
  axis_ASSERT(value && axis_value_check_integrity(value), "Should not happen.");

  napi_value js_value = NULL;
  napi_status status = napi_ok;

  if (axis_value_is_string(value)) {
    axis_string_t *value_string = axis_value_peek_string(value);

    status = napi_create_string_utf8(env, axis_string_get_raw_str(value_string),
                                     axis_string_len(value_string), &js_value);
    ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to create JS string: %d",
                        status);
  } else {
    if (error) {
      axis_error_set(error, axis_ERRNO_INVALID_TYPE, "Invalid value type.");
    }
  }

  return js_value;
}
