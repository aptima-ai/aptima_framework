//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_runtime/msg/msg.h"
#include "axis_utils/container/hash_table.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/value/value.h"

#define axis_SCHEMA_STORE_SIGNATURE 0x0FD9B508D67169A4U

typedef struct axis_schema_t axis_schema_t;
typedef struct axis_msg_schema_t axis_msg_schema_t;

typedef struct axis_schema_store_t {
  axis_signature_t signature;

  // The schema definitions are as follows:
  //
  // "api": {                    <== This section will be passed to
  //                                 `axis_schema_store_init`.
  //   "property": {
  //     "prop_a": {
  //       "type": "string"
  //     },
  //     "prop_b": {
  //       "type": "uint8"
  //     }
  //   }
  // }
  //
  // The type of property schema is always axis_schema_object_t, refer to
  // `axis_schemas_parse_schema_object_for_property`.
  axis_schema_t *property;

  // Key is the cmd name, the type of value is `axis_cmd_schema_t`.
  axis_hashtable_t cmd_in;
  axis_hashtable_t cmd_out;

  // Key is the msg name, the type of value is `axis_msg_schema_t`.
  axis_hashtable_t data_in;
  axis_hashtable_t data_out;
  axis_hashtable_t video_frame_in;
  axis_hashtable_t video_frame_out;
  axis_hashtable_t audio_frame_in;
  axis_hashtable_t audio_frame_out;

  // Key is the interface name, the type of value is `axis_interface_schema_t`.
  // Since this is a schema store, the interface_in/out here must be the
  // interface definitions after expanding the `$ref`.
  axis_hashtable_t interface_in;
  axis_hashtable_t interface_out;
} axis_schema_store_t;

axis_RUNTIME_PRIVATE_API bool axis_schema_store_check_integrity(
    axis_schema_store_t *self);

axis_RUNTIME_API void axis_schema_store_init(axis_schema_store_t *self);

axis_RUNTIME_API bool axis_schema_store_set_schema_definition(
    axis_schema_store_t *self, axis_value_t *schema_def, axis_error_t *err);

/**
 * @param schema_def The definition defined in the `api` section.
 */
axis_RUNTIME_API bool axis_schema_store_set_interface_schema_definition(
    axis_schema_store_t *self, axis_value_t *schema_def, const char *base_dir,
    axis_error_t *err);

axis_RUNTIME_API void axis_schema_store_deinit(axis_schema_store_t *self);

/**
 * @param props_value The property must be an object.
 */
axis_RUNTIME_API bool axis_schema_store_validate_properties(
    axis_schema_store_t *self, axis_value_t *props_value, axis_error_t *err);

/**
 * @param prop_name The property name must not be NULL or empty.
 */
axis_RUNTIME_API bool axis_schema_store_validate_property_kv(
    axis_schema_store_t *self, const char *prop_name, axis_value_t *prop_value,
    axis_error_t *err);

/**
 * @param prop_name The property name must not be NULL or empty.
 */
axis_RUNTIME_API bool axis_schema_store_adjust_property_kv(
    axis_schema_store_t *self, const char *prop_name, axis_value_t *prop_value,
    axis_error_t *err);

/**
 * @param props_value The property must be an object.
 */
axis_RUNTIME_API bool axis_schema_store_adjust_properties(
    axis_schema_store_t *self, axis_value_t *props_value, axis_error_t *err);

axis_RUNTIME_API axis_msg_schema_t *axis_schema_store_get_msg_schema(
    axis_schema_store_t *self, axis_MSG_TYPE msg_type, const char *msg_name,
    bool is_msg_out);

/**
 * @brief Retrieve all msg names belonging to the interface.
 *
 * @return Whether the interface is found.
 */
axis_RUNTIME_PRIVATE_API bool
axis_schema_store_get_all_msg_names_in_interface_out(axis_schema_store_t *self,
                                                    axis_MSG_TYPE msg_type,
                                                    const char *interface_name,
                                                    axis_list_t *msg_names);
