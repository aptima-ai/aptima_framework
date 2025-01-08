//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stddef.h>

#include "include_internal/axis_runtime/msg/cmd_base/cmd/field/field.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/axis_runtime/msg/field/field_info.h"

#ifdef __cplusplus
#error \
    "This file contains C99 array designated initializer, and Visual Studio C++ compiler can only support up to C89 by default, so we enable this checking to prevent any wrong inclusion of this file."
#endif

static const axis_msg_field_info_t axis_cmd_fields_info[] = {
    [axis_CMD_FIELD_CMD_BASE_HDR] =
        {
            .field_name = NULL,
            .field_id = -1,
            .copy_field = axis_raw_cmd_base_copy_field,
            .process_field = axis_raw_cmd_base_process_field,
        },
    [axis_CMD_FIELD_LAST] = {0},
};

static const size_t axis_cmd_fields_info_size =
    sizeof(axis_cmd_fields_info) / sizeof(axis_cmd_fields_info[0]);