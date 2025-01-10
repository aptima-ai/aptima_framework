//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/nodejs/axis_env/axis_env.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "include_internal/axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_runtime/axis_env/internal/on_xxx_done.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/mark.h"

static void axis_env_proxy_notify_on_stop_done(axis_env_t *axis_env,
                                              axis_UNUSED void *user_data) {
  axis_ASSERT(
      axis_env &&
          axis_env_check_integrity(
              axis_env,
              axis_env->attach_to != axis_ENV_ATTACH_TO_ADDON ? true : false),
      "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  bool rc = axis_env_on_stop_done(axis_env, &err);
  axis_ASSERT(rc, "Should not happen.");

  axis_error_deinit(&err);
}

napi_value axis_nodejs_axis_env_on_stop_done(napi_env env,
                                           napi_callback_info info) {
  axis_ASSERT(env, "Should not happen.");

  const size_t argc = 1;
  napi_value args[argc];  // axis_env
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

  axis_error_t err;
  axis_error_init(&err);
  bool rc = false;

  if (axis_env_bridge->c_axis_env->attach_to == axis_ENV_ATTACH_TO_ADDON) {
    rc = axis_env_on_stop_done(axis_env_bridge->c_axis_env, &err);
  } else {
    rc = axis_env_proxy_notify_async(axis_env_bridge->c_axis_env_proxy,
                                    axis_env_proxy_notify_on_stop_done, NULL,
                                    &err);
  }

  if (!rc) {
    axis_LOGD("APTIMA/JS failed to on_stop_done.");

    axis_string_t code_str;
    axis_string_init_formatted(&code_str, "%d", axis_error_errno(&err));

    status = napi_throw_error(env, axis_string_get_raw_str(&code_str),
                              axis_error_errmsg(&err));
    ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to throw JS exception: %d",
                        status);

    axis_string_deinit(&code_str);
  }

  axis_error_deinit(&err);

  return js_undefined(env);
}
