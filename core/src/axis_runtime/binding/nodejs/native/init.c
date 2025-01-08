//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/nodejs/addon/addon.h"
#include "include_internal/axis_runtime/binding/nodejs/app/app.h"
#include "include_internal/axis_runtime/binding/nodejs/extension/extension.h"
#include "include_internal/axis_runtime/binding/nodejs/msg/audio_frame.h"
#include "include_internal/axis_runtime/binding/nodejs/msg/cmd.h"
#include "include_internal/axis_runtime/binding/nodejs/msg/cmd_result.h"
#include "include_internal/axis_runtime/binding/nodejs/msg/data.h"
#include "include_internal/axis_runtime/binding/nodejs/msg/msg.h"
#include "include_internal/axis_runtime/binding/nodejs/msg/video_frame.h"
#include "include_internal/axis_runtime/binding/nodejs/axis_env/axis_env.h"

napi_value Init(napi_env env, napi_value exports) {
  axis_nodejs_addon_module_init(env, exports);
  axis_nodejs_app_module_init(env, exports);
  axis_nodejs_axis_env_module_init(env, exports);
  axis_nodejs_extension_module_init(env, exports);
  axis_nodejs_msg_module_init(env, exports);
  axis_nodejs_data_module_init(env, exports);
  axis_nodejs_cmd_module_init(env, exports);
  axis_nodejs_cmd_result_module_init(env, exports);
  axis_nodejs_video_frame_module_init(env, exports);
  axis_nodejs_audio_frame_module_init(env, exports);

  return exports;
}

NAPI_MODULE(axis_runtime_nodejs, Init)
