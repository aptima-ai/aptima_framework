//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <node_api.h>

#include "include_internal/axis_runtime/binding/nodejs/msg/msg.h"

typedef struct axis_nodejs_data_t {
  axis_nodejs_msg_t msg;
} axis_nodejs_data_t;

axis_RUNTIME_PRIVATE_API napi_value axis_nodejs_data_wrap(napi_env env,
                                                        axis_shared_ptr_t *data);

axis_RUNTIME_API napi_value axis_nodejs_data_module_init(napi_env env,
                                                       napi_value exports);
