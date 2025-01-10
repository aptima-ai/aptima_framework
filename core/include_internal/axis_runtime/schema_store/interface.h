//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "axis_runtime/msg/msg.h"
#include "axis_utils/container/hash_handle.h"
#include "axis_utils/container/list.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/value/value.h"

#define axis_INTERFACE_SCHEMA_SIGNATURE 0xAC3AF7CE5FFC1048U

typedef struct axis_interface_schema_t {
  axis_signature_t signature;

  axis_string_t name;
  axis_hashhandle_t hh_in_map;

  axis_list_t cmd;          // Type of value is axis_cmd_schema_t*.
  axis_list_t data;         // Type of value is axis_msg_schema_t*.
  axis_list_t video_frame;  // Type of value is axis_msg_schema_t*.
  axis_list_t audio_frame;  // Type of value is axis_msg_schema_t*.
} axis_interface_schema_t;

/**
 * @brief Resolve the interface schema content, replace the referenced schema
 * with its content.
 *
 * @param interface_schema_def The content of the interface, i.e., the value of
 * the `interface_in` or `interface_out` field in manifest.json.
 *
 * @param base_dir The base directory of the extension. It can not be empty only
 * if there is a local file referenced schema.
 *
 * @return The resolved schema definition.
 */
axis_RUNTIME_PRIVATE_API axis_value_t *axis_interface_schema_info_resolve(
    axis_value_t *interface_schema_def, const char *base_dir, axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_interface_schema_check_integrity(
    axis_interface_schema_t *self);

axis_RUNTIME_PRIVATE_API axis_interface_schema_t *axis_interface_schema_create(
    axis_value_t *interface_schema_def);

axis_RUNTIME_PRIVATE_API void axis_interface_schema_destroy(
    axis_interface_schema_t *self);

axis_RUNTIME_PRIVATE_API bool axis_interface_schema_merge_into_msg_schema(
    axis_interface_schema_t *self, axis_MSG_TYPE msg_type,
    axis_hashtable_t *msg_schema_map, axis_error_t *err);
