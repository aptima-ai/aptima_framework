//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "include_internal/ten_runtime/common/loc.h"
#include "include_internal/ten_runtime/path/common.h"
#include "include_internal/ten_runtime/path/path_table.h"
#include "ten_utils/lib/signature.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/lib/string.h"

#define TEN_PATH_SIGNATURE 0xC60A6AEBDC969A43U

typedef struct ten_path_group_t ten_path_group_t;
typedef struct ten_msg_conversion_t ten_msg_conversion_t;

typedef struct ten_path_t {
  ten_signature_t signature;
  ten_sanitizer_thread_check_t thread_check;

  ten_path_table_t *table;
  TEN_PATH_TYPE type;

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
  ten_string_t cmd_name;

  ten_string_t original_cmd_id;
  ten_string_t cmd_id;

  ten_loc_t src_loc;
  ten_loc_t dest_loc;

  ten_shared_ptr_t *group;  // a shared_ptr of ten_path_group_t

  // We will cache the returned cmd result here. If someone does not call
  // return_result() before Extension::onCmd is actually completed
  // (Ex: `await onCmd()` is done), then we can send the cmd result to the
  // previous node in graph automatically.
  ten_shared_ptr_t *cached_cmd_result;

  ten_msg_conversion_t *result_conversion;

  uint64_t expired_time_us;
} ten_path_t;

TEN_RUNTIME_PRIVATE_API bool ten_path_check_integrity(ten_path_t *self,
                                                      bool check_thread);

TEN_RUNTIME_PRIVATE_API void ten_path_init(
    ten_path_t *self, ten_path_table_t *table, TEN_PATH_TYPE type,
    const char *cmd_name, const char *parent_cmd_id, const char *cmd_id,
    ten_loc_t *src_loc, ten_loc_t *dest_loc);

TEN_RUNTIME_PRIVATE_API void ten_path_deinit(ten_path_t *self);

TEN_RUNTIME_PRIVATE_API void ten_path_set_result(ten_path_t *path,
                                                 ten_shared_ptr_t *cmd_result);

TEN_RUNTIME_PRIVATE_API void ten_path_set_expired_time(
    ten_path_t *path, uint64_t expired_time_us);

TEN_RUNTIME_PRIVATE_API ten_path_group_t *ten_path_get_group(ten_path_t *self);
