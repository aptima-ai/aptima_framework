//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/binding/nodejs/app/app.h"
#include "include_internal/ten_runtime/binding/nodejs/common/common.h"

napi_value Init(napi_env env, napi_value exports) {
  ten_nodejs_app_module_init(env, exports);
  return exports;
}

NAPI_MODULE(ten_runtime_nodejs, Init)
