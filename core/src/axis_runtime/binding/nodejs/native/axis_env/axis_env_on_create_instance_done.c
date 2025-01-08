//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/nodejs/extension/extension.h"
#include "include_internal/axis_runtime/binding/nodejs/axis_env/axis_env.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"

napi_value axis_nodejs_axis_env_on_create_instance_done(napi_env env,
                                                      napi_callback_info info) {
  axis_ASSERT(env, "Should not happen.");

  const size_t argc = 3;
  napi_value args[argc];  // axis_env, instance, context
  if (!axis_nodejs_get_js_func_args(env, info, args, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Incorrect number of parameters passed.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Should not happen.");
  }

  axis_nodejs_axis_env_t *axis_env_bridge = NULL;
  napi_status status = napi_unwrap(env, args[0], (void **)&axis_env_bridge);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && axis_env_bridge != NULL,
                                "Failed to get rte bridge: %d", status);
  axis_ASSERT(axis_env_bridge &&
                 axis_nodejs_axis_env_check_integrity(axis_env_bridge, true),
             "Should not happen.");

  axis_nodejs_extension_t *extension_bridge = NULL;
  status = napi_unwrap(env, args[1], (void **)&extension_bridge);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && extension_bridge != NULL,
                                "Failed to get extension bridge: %d", status);

  axis_ASSERT(extension_bridge &&
                 axis_nodejs_extension_check_integrity(extension_bridge, true),
             "Should not happen.");

  void *context = NULL;
  status = napi_get_value_external(env, args[2], &context);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && context != NULL,
                                "Failed to get context: %d", status);

  axis_error_t err;
  axis_error_init(&err);

  axis_ASSERT(axis_env_bridge->c_axis_env->attach_to == axis_ENV_ATTACH_TO_ADDON,
             "Should not happen.");

  bool rc = axis_env_on_create_instance_done(
      axis_env_bridge->c_axis_env, extension_bridge->c_extension, context, &err);
  axis_ASSERT(rc, "Should not happen.");

  axis_error_deinit(&err);

  return js_undefined(env);
}
