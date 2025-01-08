//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stddef.h>

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/msg/cmd_base/field/cmd_id.h"
#include "include_internal/axis_runtime/msg/cmd_base/field/field.h"
#include "include_internal/axis_runtime/msg/cmd_base/field/original_connection.h"
#include "include_internal/axis_runtime/msg/cmd_base/field/result_handler.h"
#include "include_internal/axis_runtime/msg/cmd_base/field/result_handler_data.h"
#include "include_internal/axis_runtime/msg/cmd_base/field/seq_id.h"
#include "include_internal/axis_runtime/msg/field/field.h"
#include "include_internal/axis_runtime/msg/field/field_info.h"
#include "include_internal/axis_runtime/msg/msg.h"

#ifdef __cplusplus
#error \
    "This file contains C99 array designated initializer, and Visual Studio C++ compiler can only support up to C89 by default, so we enable this checking to prevent any wrong inclusion of this file."
#endif

static const axis_msg_field_info_t axis_cmd_base_fields_info[] = {
    [axis_CMD_BASE_FIELD_MSGHDR] =
        {
            .field_name = NULL,
            .field_id = -1,
            .copy_field = axis_raw_msg_copy_field,
            .process_field = axis_raw_msg_process_field,
        },
    [axis_CMD_BASE_FIELD_CMD_ID] =
        {
            .field_name = axis_STR_CMD_ID,
            .field_id = axis_MSG_FIELD_LAST + axis_CMD_BASE_FIELD_CMD_ID,
            .copy_field = axis_cmd_base_copy_cmd_id,
            .process_field = axis_cmd_base_process_cmd_id,
        },
    [axis_CMD_BASE_FIELD_SEQ_ID] =
        {
            .field_name = axis_STR_SEQ_ID,
            .field_id = axis_MSG_FIELD_LAST + axis_CMD_BASE_FIELD_SEQ_ID,
            .copy_field = axis_cmd_base_copy_seq_id,
            .process_field = axis_cmd_base_process_seq_id,
        },
    [axis_CMD_BASE_FIELD_ORIGINAL_CONNECTION] =
        {
            .field_name = NULL,
            .field_id =
                axis_MSG_FIELD_LAST + axis_CMD_BASE_FIELD_ORIGINAL_CONNECTION,
            .copy_field = axis_cmd_base_copy_original_connection,
            .process_field = NULL,
        },
    [axis_CMD_BASE_FIELD_RESPONSE_HANDLER] =
        {
            .field_name = NULL,
            .field_id =
                axis_MSG_FIELD_LAST + axis_CMD_BASE_FIELD_RESPONSE_HANDLER,
            .copy_field = axis_cmd_base_copy_result_handler,
            .process_field = NULL,
        },
    [axis_CMD_BASE_FIELD_RESPONSE_HANDLER_DATA] =
        {
            .field_name = NULL,
            .field_id =
                axis_MSG_FIELD_LAST + axis_CMD_BASE_FIELD_RESPONSE_HANDLER_DATA,
            .copy_field = axis_cmd_base_copy_result_handler_data,
            .process_field = NULL,
        },
    [axis_CMD_BASE_FIELD_LAST] = {0},
};

static const size_t axis_cmd_base_fields_info_size =
    sizeof(axis_cmd_base_fields_info) / sizeof(axis_cmd_base_fields_info[0]);
