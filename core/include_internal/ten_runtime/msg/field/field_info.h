//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <stddef.h>

#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/msg/field/dest.h"
#include "include_internal/ten_runtime/msg/field/field.h"
#include "include_internal/ten_runtime/msg/field/name.h"
#include "include_internal/ten_runtime/msg/field/properties.h"
#include "include_internal/ten_runtime/msg/field/src.h"
#include "include_internal/ten_runtime/msg/field/type.h"

#ifdef __cplusplus
  #error \
      "This file contains C99 array designated initializer, and Visual Studio C++ compiler can only support up to C89 by default, so we enable this checking to prevent any wrong inclusion of this file."
#endif

typedef bool (*ten_msg_put_field_to_json_func_t)(ten_msg_t *self,
                                                 ten_json_t *json,
                                                 ten_error_t *err);
typedef bool (*ten_msg_get_field_from_json_func_t)(ten_msg_t *self,
                                                   ten_json_t *json,
                                                   ten_error_t *err);
typedef void (*ten_msg_copy_field_func_t)(ten_msg_t *self, ten_msg_t *src,
                                          ten_list_t *excluded_field_ids);

typedef struct ten_msg_field_info_t {
  const char *field_name;

  // @{
  // Because field_name might be repeated, such as fields with the same name at
  // different levels, it's not possible to uniquely specify a field using
  // field_name alone. Therefore, field_id is used as the unique ID for the
  // field.
  //
  // The type of 'field_id' is a signed type, because we use a negative value
  // (i.e., -1) to represents virtual fields (ex: TEN_CMD_BASE_FIELD_MSGHDR).
  int32_t field_id;
  // @}

  ten_msg_put_field_to_json_func_t put_field_to_json;
  ten_msg_get_field_from_json_func_t get_field_from_json;
  ten_msg_copy_field_func_t copy_field;
} ten_msg_field_info_t;

static const ten_msg_field_info_t ten_msg_fields_info[] = {
    [TEN_MSG_FIELD_TYPE] =
        {
            .field_name = TEN_STR_TYPE,
            .field_id = TEN_MSG_FIELD_TYPE,
            .put_field_to_json = ten_raw_msg_type_to_json,
            .get_field_from_json = ten_raw_msg_type_from_json,
            .copy_field = ten_raw_msg_type_copy,
        },
    [TEN_MSG_FIELD_NAME] =
        {
            .field_name = TEN_STR_NAME,
            .field_id = TEN_MSG_FIELD_NAME,
            .put_field_to_json = ten_raw_msg_name_to_json,
            .get_field_from_json = ten_raw_msg_name_from_json,
            .copy_field = ten_raw_msg_name_copy,
        },
    [TEN_MSG_FIELD_SRC] =
        {
            .field_name = TEN_STR_SRC,
            .field_id = TEN_MSG_FIELD_SRC,
            .put_field_to_json = ten_raw_msg_src_to_json,
            .get_field_from_json = ten_raw_msg_src_from_json,
            .copy_field = ten_raw_msg_src_copy,
        },
    [TEN_MSG_FIELD_DEST] =
        {
            .field_name = TEN_STR_DEST,
            .field_id = TEN_MSG_FIELD_DEST,
            .put_field_to_json = ten_raw_msg_dest_to_json,
            .get_field_from_json = ten_raw_msg_dest_from_json,
            .copy_field = ten_raw_msg_dest_copy,
        },
    [TEN_MSG_FIELD_PROPERTIES] =
        {
            .field_name = TEN_STR_PROPERTIES,
            .field_id = TEN_MSG_FIELD_PROPERTIES,
            .put_field_to_json = ten_raw_msg_properties_to_json,
            .get_field_from_json = ten_raw_msg_properties_from_json,
            .copy_field = ten_raw_msg_properties_copy,
        },
    [TEN_MSG_FIELD_LAST] = {0},
};

static const size_t ten_msg_fields_info_size =
    sizeof(ten_msg_fields_info) / sizeof(ten_msg_fields_info[0]);
