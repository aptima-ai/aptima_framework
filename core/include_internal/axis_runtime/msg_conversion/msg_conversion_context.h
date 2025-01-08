//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/common/loc.h"
#include "axis_utils/container/list.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/smart_ptr.h"

#define axis_MSG_CONVERSIONS_SIGNATURE 0x00C0F3A0F42BE9E9U

typedef struct axis_extension_info_t axis_extension_info_t;
typedef struct axis_extension_t axis_extension_t;
typedef struct axis_msg_and_result_conversion_t axis_msg_and_result_conversion_t;

// {
//   "type": "per_property",
//   "rules": [{
//     "path": "...",
//     "conversion_mode": "from_original",
//     "original_path": "..."
//   },{
//     "path": "...",
//     "conversion_mode": "fixed_value",
//     "value": "..."
//   }]
// }
typedef struct axis_msg_conversion_context_t {
  axis_signature_t signature;

  axis_loc_t src_loc;
  axis_string_t msg_name;

  axis_msg_and_result_conversion_t *msg_and_result_conversion;
} axis_msg_conversion_context_t;

axis_RUNTIME_PRIVATE_API bool axis_msg_conversion_context_check_integrity(
    axis_msg_conversion_context_t *self);

axis_RUNTIME_PRIVATE_API axis_msg_conversion_context_t *
axis_msg_conversion_context_create(const char *msg_name);

axis_RUNTIME_PRIVATE_API void axis_msg_conversion_context_destroy(
    axis_msg_conversion_context_t *self);

axis_RUNTIME_PRIVATE_API bool axis_msg_conversion_context_merge(
    axis_list_t *msg_conversion_contexts,
    axis_msg_conversion_context_t *new_msg_conversion_context, axis_error_t *err);

/**
 * @param result [out] The type of each item is
 * 'axis_msg_and_its_result_conversion_t'
 */
axis_RUNTIME_PRIVATE_API bool axis_extension_convert_msg(axis_extension_t *self,
                                                       axis_shared_ptr_t *msg,
                                                       axis_list_t *result,
                                                       axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_msg_conversion_context_t *
axis_msg_conversion_context_from_json(axis_json_t *json,
                                     axis_extension_info_t *src_extension_info,
                                     const char *cmd_name, axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_msg_conversion_context_t *
axis_msg_conversion_context_from_value(axis_value_t *value,
                                      axis_extension_info_t *src_extension_info,
                                      const char *cmd_name, axis_error_t *err);
