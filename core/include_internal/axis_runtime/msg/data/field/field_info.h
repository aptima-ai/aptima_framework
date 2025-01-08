//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <stddef.h>

#include "include_internal/axis_runtime/msg/data/data.h"
#include "include_internal/axis_runtime/msg/data/field/buf.h"
#include "include_internal/axis_runtime/msg/data/field/field.h"
#include "include_internal/axis_runtime/msg/field/field_info.h"
#include "include_internal/axis_runtime/msg/msg.h"

#ifdef __cplusplus
#error \
    "This file contains C99 array designated initializer, and Visual Studio C++ compiler can only support up to C89 by default, so we enable this checking to prevent any wrong inclusion of this file."
#endif

static const axis_msg_field_info_t axis_data_fields_info[] = {
    [axis_DATA_FIELD_MSGHDR] =
        {
            .field_name = NULL,
            .field_id = -1,
            .copy_field = axis_raw_msg_copy_field,
            .process_field = axis_raw_msg_process_field,
        },
    [axis_DATA_FIELD_BUF] =
        {
            .field_name = NULL,
            .field_id =
                axis_MSG_FIELD_LAST + axis_DATA_FIELD_BUF - axis_DATA_FIELD_BUF,
            .copy_field = axis_raw_data_buf_copy,
            .process_field = axis_data_process_buf,
        },
    [axis_DATA_FIELD_LAST] = {0},
};

static const size_t axis_data_fields_info_size =
    sizeof(axis_data_fields_info) / sizeof(axis_data_fields_info[0]);
