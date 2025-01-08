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
#include "axis_utils/lib/signature.h"

#define axis_NODEJS_MSG_SIGNATURE 0x544E62A3726AAF6EU

typedef struct axis_nodejs_msg_t {
  axis_signature_t signature;

  axis_nodejs_bridge_t bridge;

  axis_shared_ptr_t *msg;
} axis_nodejs_msg_t;

axis_RUNTIME_PRIVATE_API void axis_nodejs_msg_init_from_c_msg(
    axis_nodejs_msg_t *self, axis_shared_ptr_t *msg);

axis_RUNTIME_PRIVATE_API void axis_nodejs_msg_deinit(axis_nodejs_msg_t *self);

axis_RUNTIME_API napi_value axis_nodejs_msg_module_init(napi_env env,
                                                      napi_value exports);
