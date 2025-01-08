//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <node_api.h>

#include "axis_utils/lib/error.h"
#include "axis_utils/log/log.h"
#include "axis_utils/value/value.h"

typedef struct axis_smart_ptr_t axis_shared_ptr_t;

typedef struct axis_nodejs_bridge_t {
  // The following two fields are used to prevent the bridge instance from being
  // finalized. The bridge instance must only be destroyed after the
  // corresponding instances in both the C world and the JavaScript world have
  // been destroyed.
  axis_shared_ptr_t *sp_ref_by_c;
  axis_shared_ptr_t *sp_ref_by_js;

  // The reference to the JS instance.
  napi_ref js_instance_ref;
} axis_nodejs_bridge_t;

#define EXPORT_FUNC(env, exports, func)                \
  do {                                                 \
    axis_nodejs_export_func(env, exports, #func, func); \
  } while (0)

#define GOTO_LABEL_IF_NAPI_FAIL(label, expr, fmt, ...) \
  do {                                                 \
    if (!(expr)) {                                     \
      axis_LOGE(fmt, ##__VA_ARGS__);                    \
      axis_nodejs_report_and_clear_error(env, status);  \
      goto label;                                      \
    }                                                  \
  } while (0)

#define ASSERT_IF_NAPI_FAIL(expr, fmt, ...)       \
  do {                                            \
    if (!(expr)) {                                \
      axis_LOGE(fmt, ##__VA_ARGS__);               \
      axis_ASSERT(0, "Should not happen.");        \
      /* NOLINTNEXTLINE(concurrency-mt-unsafe) */ \
      exit(EXIT_FAILURE);                         \
    }                                             \
  } while (0)

#define RETURN_UNDEFINED_IF_NAPI_FAIL(expr, fmt, ...) \
  do {                                                \
    if (!(expr)) {                                    \
      axis_LOGE(fmt, ##__VA_ARGS__);                   \
      axis_nodejs_report_and_clear_error(env, status); \
      axis_ASSERT(0, "Should not happen.");            \
      return js_undefined(env);                       \
    }                                                 \
  } while (0)

axis_RUNTIME_PRIVATE_API napi_value js_undefined(napi_env env);

axis_RUNTIME_PRIVATE_API bool is_js_undefined(napi_env env, napi_value value);

axis_RUNTIME_PRIVATE_API bool is_js_string(napi_env env, napi_value value);

axis_RUNTIME_PRIVATE_API bool axis_nodejs_get_js_func_args(
    napi_env env, napi_callback_info info, napi_value *args, size_t argc);

axis_RUNTIME_PRIVATE_API bool axis_nodejs_get_str_from_js(napi_env env,
                                                        napi_value val,
                                                        axis_string_t *str);

axis_RUNTIME_PRIVATE_API void axis_nodejs_report_and_clear_error_(
    napi_env env, napi_status status, const char *func, int line);

#define axis_nodejs_report_and_clear_error(env, status) \
  axis_nodejs_report_and_clear_error_(env, status, __FUNCTION__, __LINE__)

axis_RUNTIME_PRIVATE_API napi_value axis_nodejs_get_property(
    napi_env env, napi_value js_obj, const char *property_name);

axis_RUNTIME_PRIVATE_API void axis_nodejs_export_func(napi_env env,
                                                    napi_value exports,
                                                    const char *func_name,
                                                    napi_callback func);

axis_RUNTIME_PRIVATE_API napi_value axis_nodejs_create_new_js_object_and_wrap(
    napi_env env, napi_ref constructor_ref, void *bridge_obj,
    napi_finalize finalizer, napi_ref *bridge_weak_ref, size_t argc,
    const napi_value *argv);

axis_RUNTIME_PRIVATE_API napi_value axis_nodejs_create_error(napi_env env,
                                                           axis_error_t *error);

axis_RUNTIME_PRIVATE_API napi_value axis_nodejs_create_value_number(
    napi_env env, axis_value_t *value, axis_error_t *error);

axis_RUNTIME_PRIVATE_API napi_value axis_nodejs_create_value_string(
    napi_env env, axis_value_t *value, axis_error_t *error);
