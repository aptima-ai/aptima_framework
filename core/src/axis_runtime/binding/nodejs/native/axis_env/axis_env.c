//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/nodejs/axis_env/axis_env.h"

#include "include_internal/axis_runtime/binding/common.h"
#include "include_internal/axis_runtime/binding/nodejs/common/common.h"
#include "axis_runtime/binding/common.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"
#include "axis_utils/sanitizer/thread_check.h"

static napi_ref js_axis_env_constructor_ref = NULL;  // NOLINT

static napi_value axis_nodejs_axis_env_register_class(napi_env env,
                                                    napi_callback_info info) {
  axis_ASSERT(env && info, "Should not happen.");

  const size_t argc = 1;
  napi_value argv[argc];  // TenEnv
  if (!axis_nodejs_get_js_func_args(env, info, argv, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Failed to register JS TenEnv class.", NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Should not happen.");
  }

  napi_status status =
      napi_create_reference(env, argv[0], 1, &js_axis_env_constructor_ref);
  if (status != napi_ok) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Failed to create JS reference to JS TenEnv constructor.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Failed to create JS reference to JS TenEnv constructor: %d",
               status);
  }

  return js_undefined(env);
}

static void axis_nodejs_axis_env_destroy(axis_nodejs_axis_env_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_sanitizer_thread_check_deinit(&self->thread_check);

  axis_FREE(self);
}

static void axis_nodejs_axis_env_finalize(napi_env env, void *data, void *hint) {
  axis_nodejs_axis_env_t *axis_env_bridge = data;
  axis_ASSERT(axis_env_bridge &&
                 axis_nodejs_axis_env_check_integrity(axis_env_bridge, true),
             "Should not happen.");

  axis_LOGD("APTIMA JS rte object is finalized.");

  // According to NAPI doc:
  // ====
  // Caution: The optional returned reference (if obtained) should be deleted
  // via napi_delete_reference __ONLY__ in response to the finalize callback
  // invocation. If it is deleted before then, then the finalize callback may
  // never be invoked. Therefore, when obtaining a reference a finalize callback
  // is also required in order to enable correct disposal of the reference.
  napi_delete_reference(env, axis_env_bridge->bridge.js_instance_ref);

  axis_nodejs_axis_env_destroy(axis_env_bridge);
}

bool axis_nodejs_axis_env_check_integrity(axis_nodejs_axis_env_t *self,
                                        bool check_thread) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) != axis_NODEJS_axis_ENV_SIGNATURE) {
    return false;
  }

  if (check_thread &&
      !axis_sanitizer_thread_check_do_check(&self->thread_check)) {
    return false;
  }

  return true;
}

axis_nodejs_get_property_call_ctx_t *axis_nodejs_get_property_call_ctx_create(
    axis_nodejs_tsfn_t *cb_tsfn, axis_value_t *value, axis_error_t *error) {
  axis_ASSERT(cb_tsfn, "Invalid argument.");

  axis_nodejs_get_property_call_ctx_t *ctx =
      (axis_nodejs_get_property_call_ctx_t *)axis_MALLOC(
          sizeof(axis_nodejs_get_property_call_ctx_t));
  axis_ASSERT(ctx, "Failed to allocate memory.");

  ctx->cb_tsfn = cb_tsfn;
  ctx->value = value;
  ctx->error = error;

  return ctx;
}

void axis_nodejs_get_property_call_ctx_destroy(
    axis_nodejs_get_property_call_ctx_t *ctx) {
  axis_ASSERT(ctx, "Invalid argument.");

  if (ctx->value) {
    axis_value_destroy(ctx->value);
  }

  if (ctx->error) {
    axis_error_destroy(ctx->error);
  }

  axis_FREE(ctx);
}

axis_nodejs_set_property_call_ctx_t *axis_nodejs_set_property_call_ctx_create(
    axis_nodejs_tsfn_t *cb_tsfn, bool success, axis_error_t *error) {
  axis_ASSERT(cb_tsfn, "Invalid argument.");

  axis_nodejs_set_property_call_ctx_t *ctx =
      (axis_nodejs_set_property_call_ctx_t *)axis_MALLOC(
          sizeof(axis_nodejs_set_property_call_ctx_t));
  axis_ASSERT(ctx, "Failed to allocate memory.");

  ctx->cb_tsfn = cb_tsfn;
  ctx->success = success;
  ctx->error = error;

  return ctx;
}

void axis_nodejs_set_property_call_ctx_destroy(
    axis_nodejs_set_property_call_ctx_t *ctx) {
  axis_ASSERT(ctx, "Invalid argument.");

  if (ctx->error) {
    axis_error_destroy(ctx->error);
  }

  axis_FREE(ctx);
}

napi_value axis_nodejs_axis_env_create_new_js_object_and_wrap(
    napi_env env, axis_env_t *axis_env,
    axis_nodejs_axis_env_t **out_axis_env_bridge) {
  axis_ASSERT(env, "Should not happen.");

  // RTE_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, false),
             "Invalid use of axis_env %p.", axis_env);

  axis_nodejs_axis_env_t *axis_env_bridge =
      (axis_nodejs_axis_env_t *)axis_MALLOC(sizeof(axis_nodejs_axis_env_t));
  axis_ASSERT(axis_env_bridge, "Failed to allocate memory.");

  axis_signature_set(&axis_env_bridge->signature, axis_NODEJS_axis_ENV_SIGNATURE);

  axis_sanitizer_thread_check_init_with_current_thread(
      &axis_env_bridge->thread_check);

  axis_env_bridge->c_axis_env = axis_env;
  axis_env_bridge->c_axis_env_proxy = NULL;

  axis_binding_handle_set_me_in_target_lang((axis_binding_handle_t *)axis_env,
                                           axis_env_bridge);

  napi_value instance = axis_nodejs_create_new_js_object_and_wrap(
      env, js_axis_env_constructor_ref, axis_env_bridge,
      axis_nodejs_axis_env_finalize, &axis_env_bridge->bridge.js_instance_ref, 0,
      NULL);
  if (!instance) {
    goto error;
  }

  goto done;

error:
  if (axis_env_bridge) {
    axis_FREE(axis_env_bridge);
    axis_env_bridge = NULL;
  }

done:
  if (out_axis_env_bridge) {
    *out_axis_env_bridge = axis_env_bridge;
  }

  return instance;
}

napi_value axis_nodejs_axis_env_module_init(napi_env env, napi_value exports) {
  axis_ASSERT(env && exports, "Should not happen.");

  EXPORT_FUNC(env, exports, axis_nodejs_axis_env_register_class);

  EXPORT_FUNC(env, exports, axis_nodejs_axis_env_on_configure_done);
  EXPORT_FUNC(env, exports, axis_nodejs_axis_env_on_init_done);
  EXPORT_FUNC(env, exports, axis_nodejs_axis_env_on_start_done);
  EXPORT_FUNC(env, exports, axis_nodejs_axis_env_on_stop_done);
  EXPORT_FUNC(env, exports, axis_nodejs_axis_env_on_deinit_done);
  EXPORT_FUNC(env, exports, axis_nodejs_axis_env_on_create_instance_done);
  EXPORT_FUNC(env, exports, axis_nodejs_axis_env_send_cmd);
  EXPORT_FUNC(env, exports, axis_nodejs_axis_env_send_data);
  EXPORT_FUNC(env, exports, axis_nodejs_axis_env_send_video_frame);
  EXPORT_FUNC(env, exports, axis_nodejs_axis_env_send_audio_frame);
  EXPORT_FUNC(env, exports, axis_nodejs_axis_env_return_result);
  EXPORT_FUNC(env, exports, axis_nodejs_axis_env_return_result_directly);
  EXPORT_FUNC(env, exports, axis_nodejs_axis_env_is_property_exist);
  EXPORT_FUNC(env, exports, axis_nodejs_axis_env_get_property_to_json);
  EXPORT_FUNC(env, exports, axis_nodejs_axis_env_set_property_from_json);
  EXPORT_FUNC(env, exports, axis_nodejs_axis_env_get_property_number);
  EXPORT_FUNC(env, exports, axis_nodejs_axis_env_set_property_number);
  EXPORT_FUNC(env, exports, axis_nodejs_axis_env_get_property_string);
  EXPORT_FUNC(env, exports, axis_nodejs_axis_env_set_property_string);
  EXPORT_FUNC(env, exports, axis_nodejs_axis_env_log_internal);
  EXPORT_FUNC(env, exports, axis_nodejs_axis_env_init_property_from_json);

  return exports;
}
