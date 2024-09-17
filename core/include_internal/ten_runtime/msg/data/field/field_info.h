//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <stddef.h>

#include "include_internal/ten_runtime/msg/data/data.h"
#include "include_internal/ten_runtime/msg/data/field/field.h"
#include "include_internal/ten_runtime/msg/field/field_info.h"
#include "include_internal/ten_runtime/msg/msg.h"

#ifdef __cplusplus
  #error \
      "This file contains C99 array designated initializer, and Visual Studio C++ compiler can only support up to C89 by default, so we enable this checking to prevent any wrong inclusion of this file."
#endif

static const ten_msg_field_info_t ten_data_fields_info[] = {
    [TEN_DATA_FIELD_MSGHDR] =
        {
            .field_name = NULL,
            .field_id = -1,
            .put_field_to_json = ten_raw_msg_put_field_to_json,
            .get_field_from_json = ten_raw_msg_get_field_from_json,
            .copy_field = ten_raw_msg_copy_field,
        },
    [TEN_DATA_FIELD_BUF] =
        {
            .field_name = NULL,
            .field_id =
                TEN_MSG_FIELD_LAST + TEN_DATA_FIELD_BUF - TEN_DATA_FIELD_BUF,

            // It is not possible to get/put the binary content of a memory
            // buffer from/into JSON (unless you use base64). If needed, clients
            // must use explicit get/set properties from/to the buffer; clients
            // cannot use JSON for this.
            .put_field_to_json = NULL,
            .get_field_from_json = NULL,

            .copy_field = ten_raw_data_buf_copy,
        },
    [TEN_DATA_FIELD_LAST] = {0},
};

static const size_t ten_data_fields_info_size =
    sizeof(ten_data_fields_info) / sizeof(ten_data_fields_info[0]);
