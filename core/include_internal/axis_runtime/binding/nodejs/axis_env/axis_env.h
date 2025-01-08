//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <node_api.h>

#include "include_internal/axis_runtime/binding/nodejs/common/common.h"
#include "include_internal/axis_runtime/binding/nodejs/common/tsfn.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/sanitizer/thread_check.h"

#define axis_NODEJS_axis_ENV_SIGNATURE 0x180B00AACEEF06E0U

typedef struct axis_nodejs_axis_env_t {
  axis_signature_t signature;
  axis_sanitizer_thread_check_t thread_check;

  axis_nodejs_bridge_t bridge;

  // Point to the corresponding C axis_env.
  axis_env_t *c_axis_env;

  // Point to the corresponding C axis_env_proxy if any.
  axis_env_proxy_t *c_axis_env_proxy;
} axis_nodejs_axis_env_t;

typedef struct axis_nodejs_get_property_call_ctx_t {
  axis_nodejs_tsfn_t *cb_tsfn;
  axis_value_t *value;
  axis_error_t *error;
} axis_nodejs_get_property_call_ctx_t;

typedef struct axis_nodejs_set_property_call_ctx_t {
  axis_nodejs_tsfn_t *cb_tsfn;
  bool success;
  axis_error_t *error;
} axis_nodejs_set_property_call_ctx_t;

axis_RUNTIME_PRIVATE_API axis_nodejs_get_property_call_ctx_t *
axis_nodejs_get_property_call_ctx_create(axis_nodejs_tsfn_t *cb_tsfn,
                                        axis_value_t *value, axis_error_t *error);

axis_RUNTIME_PRIVATE_API void axis_nodejs_get_property_call_ctx_destroy(
    axis_nodejs_get_property_call_ctx_t *ctx);

axis_RUNTIME_PRIVATE_API axis_nodejs_set_property_call_ctx_t *
axis_nodejs_set_property_call_ctx_create(axis_nodejs_tsfn_t *cb_tsfn,
                                        bool success, axis_error_t *error);

axis_RUNTIME_PRIVATE_API void axis_nodejs_set_property_call_ctx_destroy(
    axis_nodejs_set_property_call_ctx_t *ctx);

axis_RUNTIME_API napi_value axis_nodejs_axis_env_module_init(napi_env env,
                                                          napi_value exports);

axis_RUNTIME_API napi_value axis_nodejs_axis_env_create_new_js_object_and_wrap(
    napi_env env, axis_env_t *axis_env,
    axis_nodejs_axis_env_t **out_axis_env_bridge);

axis_RUNTIME_PRIVATE_API bool axis_nodejs_axis_env_check_integrity(
    axis_nodejs_axis_env_t *self, bool check_thread);

axis_RUNTIME_PRIVATE_API napi_value
axis_nodejs_axis_env_on_configure_done(napi_env env, napi_callback_info info);

axis_RUNTIME_PRIVATE_API napi_value
axis_nodejs_axis_env_on_init_done(napi_env env, napi_callback_info info);

axis_RUNTIME_PRIVATE_API napi_value
axis_nodejs_axis_env_on_start_done(napi_env env, napi_callback_info info);

axis_RUNTIME_PRIVATE_API napi_value
axis_nodejs_axis_env_on_stop_done(napi_env env, napi_callback_info info);

axis_RUNTIME_PRIVATE_API napi_value
axis_nodejs_axis_env_on_deinit_done(napi_env env, napi_callback_info info);

axis_RUNTIME_PRIVATE_API napi_value axis_nodejs_axis_env_on_create_instance_done(
    napi_env env, napi_callback_info info);

axis_RUNTIME_PRIVATE_API napi_value
axis_nodejs_axis_env_send_cmd(napi_env env, napi_callback_info info);

axis_RUNTIME_PRIVATE_API napi_value
axis_nodejs_axis_env_send_data(napi_env env, napi_callback_info info);

axis_RUNTIME_PRIVATE_API napi_value
axis_nodejs_axis_env_send_video_frame(napi_env env, napi_callback_info info);

axis_RUNTIME_PRIVATE_API napi_value
axis_nodejs_axis_env_send_audio_frame(napi_env env, napi_callback_info info);

axis_RUNTIME_PRIVATE_API napi_value
axis_nodejs_axis_env_return_result(napi_env env, napi_callback_info info);

axis_RUNTIME_PRIVATE_API napi_value axis_nodejs_axis_env_return_result_directly(
    napi_env env, napi_callback_info info);

axis_RUNTIME_PRIVATE_API bool axis_nodejs_axis_env_get_property_value(
    axis_nodejs_axis_env_t *self, const char *path, axis_nodejs_tsfn_t *cb_tsfn,
    axis_error_t *error);

axis_RUNTIME_PRIVATE_API bool axis_nodejs_axis_env_set_property_value(
    axis_nodejs_axis_env_t *self, const char *path, axis_value_t *value,
    axis_nodejs_tsfn_t *cb_tsfn, axis_error_t *error);

axis_RUNTIME_PRIVATE_API napi_value
axis_nodejs_axis_env_is_property_exist(napi_env env, napi_callback_info info);

axis_RUNTIME_PRIVATE_API napi_value
axis_nodejs_axis_env_get_property_to_json(napi_env env, napi_callback_info info);

axis_RUNTIME_PRIVATE_API napi_value axis_nodejs_axis_env_set_property_from_json(
    napi_env env, napi_callback_info info);

axis_RUNTIME_PRIVATE_API napi_value
axis_nodejs_axis_env_get_property_number(napi_env env, napi_callback_info info);

axis_RUNTIME_PRIVATE_API napi_value
axis_nodejs_axis_env_set_property_number(napi_env env, napi_callback_info info);

axis_RUNTIME_PRIVATE_API napi_value
axis_nodejs_axis_env_get_property_string(napi_env env, napi_callback_info info);

axis_RUNTIME_PRIVATE_API napi_value
axis_nodejs_axis_env_set_property_string(napi_env env, napi_callback_info info);

axis_RUNTIME_PRIVATE_API napi_value
axis_nodejs_axis_env_log_internal(napi_env env, napi_callback_info info);

axis_RUNTIME_PRIVATE_API napi_value axis_nodejs_axis_env_init_property_from_json(
    napi_env env, napi_callback_info info);
