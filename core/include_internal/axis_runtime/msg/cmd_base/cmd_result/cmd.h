//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/msg/cmd_base/cmd_base.h"
#include "axis_runtime/common/status_code.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/macro/mark.h"

#define axis_CMD_STATUS_SIGNATURE 0x9EAF798217CDEC8DU

typedef struct axis_msg_t axis_msg_t;
typedef struct axis_schema_store_t axis_schema_store_t;

typedef struct axis_cmd_result_t {
  axis_cmd_base_t cmd_base_hdr;

  axis_signature_t signature;

  axis_value_t original_cmd_type;  // int32 (axis_MSG_TYPE)
  axis_value_t original_cmd_name;  // string

  axis_value_t status_code;  // int32 (axis_STATUS_CODE)

  axis_value_t is_final;      // bool
  axis_value_t is_completed;  // bool
} axis_cmd_result_t;

axis_RUNTIME_PRIVATE_API bool axis_raw_cmd_result_validate_schema(
    axis_msg_t *msg, axis_schema_store_t *schema_store, bool is_msg_out,
    axis_error_t *err);

/**
 * @brief The schema definition of the `status` cmd is defined within the schema
 * of the `cmd`, ex:
 *
 *   "api": {
 *     "cmd_in": [{
 *       "name": "hello",
 *       "result": {
 *         "property": {}
 *       }
 *     }]
 *   }
 *
 * In other words, the schema of the `status` cmd is indexed by the original cmd
 * name. When we want to validate the cmd result, we must get the original cmd
 * name from the path first, and the original cmd name has to be stored in the
 * cmd result for using later. As the msg name of the cmd result is always a
 * fixed value (i.e., aptima::result), we can borrow the storage of the msg name to
 * store the original cmd name, and then resume the msg name after the schema
 * validation.
 */
axis_RUNTIME_PRIVATE_API void axis_cmd_result_set_original_cmd_name(
    axis_shared_ptr_t *self, const char *original_cmd_name);

axis_RUNTIME_PRIVATE_API void axis_raw_cmd_result_set_status_code(
    axis_cmd_result_t *self, axis_STATUS_CODE status_code);

axis_RUNTIME_API void axis_cmd_result_set_status_code(
    axis_shared_ptr_t *self, axis_STATUS_CODE status_code);

axis_RUNTIME_PRIVATE_API void axis_raw_cmd_result_set_original_cmd_type(
    axis_cmd_result_t *self, axis_MSG_TYPE type);

axis_RUNTIME_PRIVATE_API axis_MSG_TYPE
axis_raw_cmd_result_get_original_cmd_type(axis_cmd_result_t *self);

axis_RUNTIME_PRIVATE_API void axis_cmd_result_set_original_cmd_type(
    axis_shared_ptr_t *self, axis_MSG_TYPE type);

axis_RUNTIME_PRIVATE_API axis_MSG_TYPE
axis_cmd_result_get_original_cmd_type(axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API axis_msg_t *axis_raw_cmd_result_as_msg_clone(
    axis_msg_t *self, axis_UNUSED axis_list_t *excluded_field_ids);

axis_RUNTIME_PRIVATE_API void axis_raw_cmd_result_destroy(axis_cmd_result_t *self);

axis_RUNTIME_PRIVATE_API axis_STATUS_CODE
axis_raw_cmd_result_get_status_code(axis_cmd_result_t *self);

axis_RUNTIME_API axis_json_t *axis_cmd_result_to_json(axis_shared_ptr_t *self,
                                                   axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_raw_cmd_result_is_final(axis_cmd_result_t *self,
                                                         axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_raw_cmd_result_is_completed(
    axis_cmd_result_t *self, axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_raw_cmd_result_set_final(
    axis_cmd_result_t *self, bool is_final, axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_raw_cmd_result_set_completed(
    axis_cmd_result_t *self, bool is_completed, axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_raw_cmd_result_loop_all_fields(
    axis_msg_t *self, axis_raw_msg_process_one_field_func_t cb, void *user_data,
    axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_cmd_result_set_completed(
    axis_shared_ptr_t *self, bool is_completed, axis_error_t *err);
