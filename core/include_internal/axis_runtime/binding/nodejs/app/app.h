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
#include "axis_runtime/app/app.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/sanitizer/thread_check.h"

#define axis_NODEJS_APP_SIGNATURE 0x133066C5116F5EA4U

typedef struct axis_nodejs_app_t {
  axis_signature_t signature;
  axis_sanitizer_thread_check_t thread_check;

  axis_nodejs_bridge_t bridge;

  axis_app_t *c_app;  // The corresponding C app.

  // @{
  // The following functions represent the JavaScript functions corresponding to
  // the app interface API.
  axis_nodejs_tsfn_t *js_on_configure;
  axis_nodejs_tsfn_t *js_on_init;
  axis_nodejs_tsfn_t *js_on_deinit;
  // @}
} axis_nodejs_app_t;

axis_RUNTIME_API napi_value axis_nodejs_app_module_init(napi_env env,
                                                      napi_value exports);
