//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <node_api.h>

#include "include_internal/axis_runtime/binding/nodejs/common/common.h"
#include "include_internal/axis_runtime/binding/nodejs/common/tsfn.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/sanitizer/thread_check.h"

#define axis_NODEJS_EXTENSION_SIGNATURE 0xB7288D534BC053C3U

typedef struct axis_nodejs_extension_t {
  axis_signature_t signature;
  axis_sanitizer_thread_check_t thread_check;

  axis_nodejs_bridge_t bridge;

  axis_extension_t *c_extension;  // The corresponding C extension.

  // @{
  // The following functions represent the JavaScript functions corresponding to
  // the extension interface API.
  axis_nodejs_tsfn_t *js_on_configure;
  axis_nodejs_tsfn_t *js_on_init;
  axis_nodejs_tsfn_t *js_on_start;
  axis_nodejs_tsfn_t *js_on_stop;
  axis_nodejs_tsfn_t *js_on_deinit;

  axis_nodejs_tsfn_t *js_on_cmd;
  axis_nodejs_tsfn_t *js_on_data;
  axis_nodejs_tsfn_t *js_on_audio_frame;
  axis_nodejs_tsfn_t *js_on_video_frame;
  // @}
} axis_nodejs_extension_t;

axis_RUNTIME_PRIVATE_API bool axis_nodejs_extension_check_integrity(
    axis_nodejs_extension_t *self, bool check_thread);

axis_RUNTIME_API napi_value axis_nodejs_extension_module_init(napi_env env,
                                                            napi_value exports);
