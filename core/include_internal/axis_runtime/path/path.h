//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/common/loc.h"
#include "include_internal/axis_runtime/path/common.h"
#include "include_internal/axis_runtime/path/path_table.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"

#define axis_PATH_SIGNATURE 0xC60A6AEBDC969A43U

typedef struct axis_path_group_t axis_path_group_t;
typedef struct axis_msg_conversion_t axis_msg_conversion_t;

typedef struct axis_path_t {
  axis_signature_t signature;
  axis_sanitizer_thread_check_t thread_check;

  axis_path_table_t *table;
  axis_PATH_TYPE type;

  // This field is only useful for the path of the cmd result and is used to
  // store the command name of the command corresponding to the cmd result.
  // This is because some information from the cmd result can only be
  // obtained when the original command is known.
  //
  // Ex: The schema of the `status` cmd is defined within the corresponding
  // `cmd`, ex:
  //
  // "api": {
  //   "cmd_in": [
  //     {
  //       "name": "hello",
  //       "result": {
  //         "property": {}
  //       }
  //     }
  //   ]
  // }
  //
  // We need to use the cmd name to find the schema of the `status` cmd.
  axis_string_t cmd_name;

  axis_string_t original_cmd_id;
  axis_string_t cmd_id;

  axis_loc_t src_loc;
  axis_loc_t dest_loc;

  axis_shared_ptr_t *group;  // a shared_ptr of axis_path_group_t

  // We will cache the returned cmd result here. If someone does not call
  // return_result() before Extension::onCmd is actually completed
  // (Ex: `await onCmd()` is done), then we can send the cmd result to the
  // previous node in graph automatically.
  axis_shared_ptr_t *cached_cmd_result;

  axis_msg_conversion_t *result_conversion;

  uint64_t expired_time_us;
} axis_path_t;

axis_RUNTIME_PRIVATE_API bool axis_path_check_integrity(axis_path_t *self,
                                                      bool check_thread);

axis_RUNTIME_PRIVATE_API void axis_path_init(
    axis_path_t *self, axis_path_table_t *table, axis_PATH_TYPE type,
    const char *cmd_name, const char *parent_cmd_id, const char *cmd_id,
    axis_loc_t *src_loc, axis_loc_t *dest_loc);

axis_RUNTIME_PRIVATE_API void axis_path_deinit(axis_path_t *self);

axis_RUNTIME_PRIVATE_API void axis_path_set_result(axis_path_t *path,
                                                 axis_shared_ptr_t *cmd_result);

axis_RUNTIME_PRIVATE_API void axis_path_set_expired_time(
    axis_path_t *path, uint64_t expired_time_us);

axis_RUNTIME_PRIVATE_API axis_path_group_t *axis_path_get_group(axis_path_t *self);
