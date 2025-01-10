//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <node_api.h>

#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/binding/nodejs/common/common.h"
#include "include_internal/axis_runtime/binding/nodejs/common/tsfn.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/sanitizer/thread_check.h"

#define axis_NODEJS_ADDON_SIGNATURE 0xAB6B372F015D9CE9U

typedef struct axis_nodejs_addon_t {
  axis_signature_t signature;
  axis_sanitizer_thread_check_t thread_check;

  axis_nodejs_bridge_t bridge;

  axis_string_t addon_name;
  axis_addon_t c_addon;  // The corresponding C addon.

  axis_addon_host_t *c_addon_host;

  // @{
  // The following functions represent the JavaScript functions corresponding to
  // the addon interface API.
  axis_nodejs_tsfn_t *js_on_init;
  axis_nodejs_tsfn_t *js_on_deinit;
  axis_nodejs_tsfn_t *js_on_create_instance;
  // @}
} axis_nodejs_addon_t;

axis_RUNTIME_PRIVATE_API bool axis_nodejs_addon_check_integrity(
    axis_nodejs_addon_t *self, bool check_thread);

axis_RUNTIME_PRIVATE_API void axis_nodejs_invoke_addon_js_on_init(napi_env env,
                                                                napi_value fn,
                                                                void *context,
                                                                void *data);

axis_RUNTIME_PRIVATE_API void axis_nodejs_invoke_addon_js_on_deinit(napi_env env,
                                                                  napi_value fn,
                                                                  void *context,
                                                                  void *data);

axis_RUNTIME_PRIVATE_API void axis_nodejs_invoke_addon_js_on_create_instance(
    napi_env env, napi_value fn, void *context, void *data);

axis_RUNTIME_API napi_value axis_nodejs_addon_module_init(napi_env env,
                                                        napi_value exports);
