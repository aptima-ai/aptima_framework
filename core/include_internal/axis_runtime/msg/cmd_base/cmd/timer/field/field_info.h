//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <stddef.h>

#include "include_internal/axis_runtime/msg/cmd_base/cmd/cmd.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/timer/field/field.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/timer/field/timeout_in_us.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/timer/field/timer_id.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/timer/field/times.h"
#include "include_internal/axis_runtime/msg/field/field_info.h"

#ifdef __cplusplus
#error \
    "This file contains C99 array designated initializer, and Visual Studio C++ compiler can only support up to C89 by default, so we enable this checking to prevent any wrong inclusion of this file."
#endif

static const axis_msg_field_info_t axis_cmd_timer_fields_info[] = {
    [axis_CMD_TIMER_FIELD_CMD_HDR] =
        {
            .field_name = NULL,
            .copy_field = axis_raw_cmd_copy_field,
            .process_field = axis_raw_cmd_process_field,
        },
    [axis_CMD_TIMER_FIELD_TIMER_ID] =
        {
            .field_name = NULL,
            .copy_field = NULL,
            .process_field = axis_cmd_timer_process_timer_id,
        },
    [axis_CMD_TIMER_FIELD_TIMEOUT_IN_US] =
        {
            .field_name = NULL,
            .copy_field = NULL,
            .process_field = axis_cmd_timer_process_timeout_in_us,
        },
    [axis_CMD_TIMER_FIELD_TIMES] =
        {
            .field_name = NULL,
            .copy_field = NULL,
            .process_field = axis_cmd_timer_process_times,
        },
    [axis_CMD_TIMER_FIELD_LAST] = {0},
};

static const size_t axis_cmd_timer_fields_info_size =
    sizeof(axis_cmd_timer_fields_info) / sizeof(axis_cmd_timer_fields_info[0]);
