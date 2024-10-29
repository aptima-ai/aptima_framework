//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <stddef.h>

#include "include_internal/ten_runtime/msg/cmd_base/cmd/close_app/field/field.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd/cmd.h"
#include "include_internal/ten_runtime/msg/field/field_info.h"

#ifdef __cplusplus
#error \
    "This file contains C99 array designated initializer, and Visual Studio C++ compiler can only support up to C89 by default, so we enable this checking to prevent any wrong inclusion of this file."
#endif

static const ten_msg_field_info_t ten_cmd_close_app_fields_info[] = {
    [TEN_CMD_CLOSE_APP_FIELD_CMD_HDR] =
        {
            .field_name = NULL,
            .put_field_to_json = ten_raw_cmd_put_field_to_json,
            .get_field_from_json = ten_raw_cmd_get_field_from_json,
            .copy_field = NULL,
            .process_field = ten_raw_cmd_process_field,
        },
    [TEN_CMD_CLOSE_APP_FIELD_LAST] = {0},
};

static const size_t ten_cmd_close_app_fields_info_size =
    sizeof(ten_cmd_close_app_fields_info) /
    sizeof(ten_cmd_close_app_fields_info[0]);
