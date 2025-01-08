//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <stddef.h>

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/msg/field/dest.h"
#include "include_internal/axis_runtime/msg/field/field.h"
#include "include_internal/axis_runtime/msg/field/name.h"
#include "include_internal/axis_runtime/msg/field/properties.h"
#include "include_internal/axis_runtime/msg/field/src.h"
#include "include_internal/axis_runtime/msg/field/type.h"
#include "include_internal/axis_runtime/msg/loop_fields.h"

#ifdef __cplusplus
#error \
    "This file contains C99 array designated initializer, and Visual Studio C++ compiler can only support up to C89 by default, so we enable this checking to prevent any wrong inclusion of this file."
#endif

typedef void (*axis_msg_copy_field_func_t)(axis_msg_t *self, axis_msg_t *src,
                                          axis_list_t *excluded_field_ids);
typedef bool (*axis_msg_process_field_func_t)(
    axis_msg_t *self, axis_raw_msg_process_one_field_func_t cb, void *user_data,
    axis_error_t *err);

typedef struct axis_msg_field_info_t {
  const char *field_name;

  // @{
  // Because field_name might be repeated, such as fields with the same name at
  // different levels, it's not possible to uniquely specify a field using
  // field_name alone. Therefore, field_id is used as the unique ID for the
  // field.
  //
  // The type of 'field_id' is a signed type, because we use a negative value
  // (i.e., -1) to represents virtual fields (ex: axis_CMD_BASE_FIELD_MSGHDR).
  int32_t field_id;
  // @}

  axis_msg_copy_field_func_t copy_field;
  axis_msg_process_field_func_t process_field;
} axis_msg_field_info_t;

static const axis_msg_field_info_t axis_msg_fields_info[] = {
    [axis_MSG_FIELD_TYPE] =
        {
            .field_name = axis_STR_TYPE,
            .field_id = axis_MSG_FIELD_TYPE,
            .copy_field = axis_raw_msg_type_copy,
            .process_field = axis_raw_msg_type_process,
        },
    [axis_MSG_FIELD_NAME] =
        {
            .field_name = axis_STR_NAME,
            .field_id = axis_MSG_FIELD_NAME,
            .copy_field = axis_raw_msg_name_copy,
            .process_field = axis_raw_msg_name_process,
        },
    [axis_MSG_FIELD_SRC] =
        {
            .field_name = axis_STR_SRC,
            .field_id = axis_MSG_FIELD_SRC,
            .copy_field = axis_raw_msg_src_copy,
            .process_field = axis_raw_msg_src_process,
        },
    [axis_MSG_FIELD_DEST] =
        {
            .field_name = axis_STR_DEST,
            .field_id = axis_MSG_FIELD_DEST,
            .copy_field = axis_raw_msg_dest_copy,
            .process_field = axis_raw_msg_dest_process,
        },
    [axis_MSG_FIELD_PROPERTIES] =
        {
            .field_name = axis_STR_PROPERTIES,
            .field_id = axis_MSG_FIELD_PROPERTIES,
            .copy_field = axis_raw_msg_properties_copy,
            .process_field = axis_raw_msg_properties_process,
        },
    [axis_MSG_FIELD_LAST] = {0},
};

static const size_t axis_msg_fields_info_size =
    sizeof(axis_msg_fields_info) / sizeof(axis_msg_fields_info[0]);
