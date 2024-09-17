//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdbool.h>

#include "ten_runtime/msg/msg.h"
#include "ten_utils/container/hash_handle.h"
#include "ten_utils/container/list.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/signature.h"
#include "ten_utils/value/value.h"

#define TEN_INTERFACE_SCHEMA_SIGNATURE 0xAC3AF7CE5FFC1048U

typedef struct ten_interface_schema_t {
  ten_signature_t signature;

  ten_string_t name;
  ten_hashhandle_t hh_in_map;

  ten_list_t cmd;          // Type of value is ten_cmd_schema_t*.
  ten_list_t data;         // Type of value is ten_msg_schema_t*.
  ten_list_t video_frame;  // Type of value is ten_msg_schema_t*.
  ten_list_t audio_frame;  // Type of value is ten_msg_schema_t*.
} ten_interface_schema_t;

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
TEN_RUNTIME_PRIVATE_API ten_value_t *ten_interface_schema_info_resolve(
    ten_value_t *interface_schema_def, const char *base_dir, ten_error_t *err);

TEN_RUNTIME_PRIVATE_API bool ten_interface_schema_check_integrity(
    ten_interface_schema_t *self);

TEN_RUNTIME_PRIVATE_API ten_interface_schema_t *ten_interface_schema_create(
    ten_value_t *interface_schema_def);

TEN_RUNTIME_PRIVATE_API void ten_interface_schema_destroy(
    ten_interface_schema_t *self);

TEN_RUNTIME_PRIVATE_API bool ten_interface_schema_merge_into_msg_schema(
    ten_interface_schema_t *self, TEN_MSG_TYPE msg_type,
    ten_hashtable_t *msg_schema_map, ten_error_t *err);
