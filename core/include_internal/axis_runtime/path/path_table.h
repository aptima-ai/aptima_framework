//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/path/common.h"
#include "axis_utils/container/list.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/sanitizer/thread_check.h"

#define axis_PATH_TABLE_SIGNATURE 0xB7C015B5FD691797U

typedef struct axis_loc_t axis_loc_t;
typedef struct axis_path_group_t axis_path_group_t;
typedef struct axis_extension_t axis_extension_t;
typedef struct axis_engine_t axis_engine_t;
typedef struct axis_path_t axis_path_t;
typedef struct axis_path_out_t axis_path_out_t;
typedef struct axis_path_in_t axis_path_in_t;
typedef struct axis_msg_conversion_t axis_msg_conversion_t;

typedef enum axis_PATH_TABLE_ATTACH_TO {
  axis_PATH_TABLE_ATTACH_TO_INVALID,

  axis_PATH_TABLE_ATTACH_TO_EXTENSION,
  axis_PATH_TABLE_ATTACH_TO_ENGINE,
} axis_PATH_TABLE_ATTACH_TO;

typedef struct axis_path_table_t {
  axis_signature_t signature;
  axis_sanitizer_thread_check_t thread_check;

  axis_PATH_TABLE_ATTACH_TO attach_to;
  union {
    axis_extension_t *extension;
    axis_engine_t *engine;
  } attached_target;

  axis_list_t in_paths;   // axis_path_in_t
  axis_list_t out_paths;  // axis_path_out_t
} axis_path_table_t;

axis_RUNTIME_PRIVATE_API axis_path_table_t *axis_path_table_create(
    axis_PATH_TABLE_ATTACH_TO attach_to, void *attached_target);

axis_RUNTIME_PRIVATE_API void axis_path_table_destroy(axis_path_table_t *self);

axis_RUNTIME_PRIVATE_API void axis_path_table_check_empty(axis_path_table_t *self);

axis_RUNTIME_PRIVATE_API axis_path_in_t *axis_path_table_add_in_path(
    axis_path_table_t *self, axis_shared_ptr_t *cmd,
    axis_msg_conversion_t *result_conversion);

axis_RUNTIME_PRIVATE_API axis_path_out_t *axis_path_table_add_out_path(
    axis_path_table_t *self, axis_shared_ptr_t *cmd);

axis_RUNTIME_PRIVATE_API axis_listnode_t *axis_path_table_find_path_from_cmd_id(
    axis_path_table_t *self, axis_PATH_TYPE type, const char *cmd_id);

axis_RUNTIME_PRIVATE_API axis_shared_ptr_t *
axis_path_table_determine_actual_cmd_result(axis_path_table_t *self,
                                           axis_PATH_TYPE path_type,
                                           axis_path_t *path, bool remove_path);

axis_RUNTIME_PRIVATE_API axis_path_t *axis_path_table_set_result(
    axis_path_table_t *self, axis_PATH_TYPE path_type,
    axis_shared_ptr_t *cmd_result);

axis_RUNTIME_PRIVATE_API axis_string_t *axis_path_table_get_graph_id(
    axis_path_table_t *self);
