//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <node_api.h>

axis_RUNTIME_API napi_value axis_nodejs_addon_manager_register_addon_as_extension(
    napi_env env, napi_callback_info info);
