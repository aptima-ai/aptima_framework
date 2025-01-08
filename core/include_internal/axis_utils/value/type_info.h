//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stddef.h>

#include "include_internal/axis_utils/value/constant_str.h"
#include "axis_utils/value/type.h"

#ifdef __cplusplus
#error \
    "This file contains C99 array designated initializer, and Visual Studio C++ compiler can only support up to C89 by default, so we enable this checking to prevent any wrong inclusion of this file."
#endif

#define axis_IS_INTEGER_TYPE(t)                                               \
  ((t) == axis_TYPE_INT8 || (t) == axis_TYPE_UINT8 || (t) == axis_TYPE_INT16 || \
   (t) == axis_TYPE_UINT16 || (t) == axis_TYPE_INT32 ||                        \
   (t) == axis_TYPE_UINT32 || (t) == axis_TYPE_INT64 || (t) == axis_TYPE_UINT64)

#define axis_IS_FLOAT_TYPE(t) \
  ((t) == axis_TYPE_FLOAT32 || (t) == axis_TYPE_FLOAT64)

typedef struct axis_type_info_t {
  const char *name;
} axis_type_info_t;

static const axis_type_info_t axis_types_info[] = {
    [axis_TYPE_INVALID] =
        {
            .name = NULL,
        },
    [axis_TYPE_NULL] =
        {
            .name = axis_STR_NULL,
        },
    [axis_TYPE_BOOL] =
        {
            .name = axis_STR_BOOL,
        },
    [axis_TYPE_INT8] =
        {
            .name = axis_STR_INT8,
        },
    [axis_TYPE_INT16] =
        {
            .name = axis_STR_INT16,
        },
    [axis_TYPE_INT32] =
        {
            .name = axis_STR_INT32,
        },
    [axis_TYPE_INT64] =
        {
            .name = axis_STR_INT64,
        },
    [axis_TYPE_UINT8] =
        {
            .name = axis_STR_UINT8,
        },
    [axis_TYPE_UINT16] =
        {
            .name = axis_STR_UINT16,
        },
    [axis_TYPE_UINT32] =
        {
            .name = axis_STR_UINT32,
        },
    [axis_TYPE_UINT64] =
        {
            .name = axis_STR_UINT64,
        },
    [axis_TYPE_FLOAT32] =
        {
            .name = axis_STR_FLOAT32,
        },
    [axis_TYPE_FLOAT64] =
        {
            .name = axis_STR_FLOAT64,
        },
    [axis_TYPE_ARRAY] =
        {
            .name = axis_STR_ARRAY,
        },
    [axis_TYPE_OBJECT] =
        {
            .name = axis_STR_OBJECT,
        },
    [axis_TYPE_PTR] =
        {
            .name = axis_STR_PTR,
        },
    [axis_TYPE_STRING] =
        {
            .name = axis_STR_STRING,
        },
    [axis_TYPE_BUF] =
        {
            .name = axis_STR_BUF,
        },
};

static const size_t axis_types_info_size =
    sizeof(axis_types_info) / sizeof(axis_types_info[0]);
