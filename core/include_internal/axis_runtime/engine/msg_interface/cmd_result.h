//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/lib/error.h"
#include "axis_utils/lib/smart_ptr.h"

typedef struct axis_engine_t axis_engine_t;

axis_RUNTIME_PRIVATE_API void axis_engine_handle_cmd_result(
    axis_engine_t *self, axis_shared_ptr_t *cmd_result, axis_error_t *err);
