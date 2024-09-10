//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stddef.h>

#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd_result/field/field.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd_result/field/original_cmd_type.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd_result/field/status_code.h"
#include "include_internal/ten_runtime/msg/field/field_info.h"

#ifdef __cplusplus
  #error \
      "This file contains C99 array designated initializer, and Visual Studio C++ compiler can only support up to C89 by default, so we enable this checking to prevent any wrong inclusion of this file."
#endif

static const ten_msg_field_info_t ten_cmd_result_fields_info[] = {
    [TEN_CMD_STATUS_FIELD_CMD_BASE_HDR] =
        {
            .field_name = NULL,
            .put_field_to_json = ten_raw_cmd_base_put_field_to_json,
            .get_field_from_json = ten_raw_cmd_base_get_field_from_json,
            .copy_field = ten_raw_cmd_base_copy_field,
        },
    [TEN_CMD_STATUS_FIELD_ORIGINAL_CMD_TYPE] =
        {
            .field_name = TEN_STR_ORIGINAL_CMD_TYPE,
            .put_field_to_json = ten_cmd_result_put_original_cmd_type_to_json,
            .get_field_from_json =
                ten_cmd_result_get_original_cmd_type_from_json,
            .copy_field = ten_cmd_result_copy_original_cmd_type,
        },
    [TEN_CMD_STATUS_FIELD_STATUS_CODE] =
        {
            .field_name = TEN_STR_STATUS_CODE,
            .put_field_to_json = ten_cmd_result_put_status_code_to_json,
            .get_field_from_json = ten_cmd_result_get_status_code_from_json,
            .copy_field = ten_cmd_result_copy_status_code,
        },
    [TEN_CMD_STATUS_FIELD_LAST] = {0},
};

static const size_t ten_cmd_result_fields_info_size =
    sizeof(ten_cmd_result_fields_info) / sizeof(ten_cmd_result_fields_info[0]);
