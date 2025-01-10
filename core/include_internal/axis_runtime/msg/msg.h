//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/common/loc.h"
#include "include_internal/axis_runtime/msg/loop_fields.h"
#include "axis_runtime/msg/msg.h"
#include "axis_utils/container/list.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value.h"

#define axis_MSG_SIGNATURE 0xA9FA53F77185F856U

typedef struct axis_app_t axis_app_t;
typedef struct axis_extension_t axis_extension_t;
typedef struct axis_error_t axis_error_t;
typedef struct axis_engine_t axis_engine_t;
typedef struct axis_extension_group_t axis_extension_group_t;
typedef struct axis_extension_thread_t axis_extension_thread_t;
typedef struct axis_extension_info_t axis_extension_info_t;
typedef struct axis_schema_store_t axis_schema_store_t;

static_assert(sizeof(axis_MSG_TYPE) == sizeof(uint32_t),
              "Incorrect axis_MSG_TYPE size, this would break (de)serialize.");

// - Only msg types have `to_json`/`from_json` functions.
//
//   - If the json contains the fields `aptima::type` and `aptima::name`, these fields
//     must uniquely map to one actual type of the msg instance; otherwise, an
//     error will be thrown.
//
//   - If the json contains the fields `aptima::type` and `aptima::name`, these fields
//     must uniquely map to the actual type of the msg instance; otherwise, an
//     error will be thrown.

typedef struct axis_msg_t {
  axis_signature_t signature;
  axis_sanitizer_thread_check_t thread_check;

  axis_MSG_TYPE type;

  // Each message has a "name", which is used for routing. In the graph, you can
  // specify different names to flow to different destination extensions. If a
  // message's name is empty, it can only flow to the destinations in the graph
  // that have not specified a name.
  axis_value_t name;  // string

  axis_loc_t src_loc;
  axis_list_t dest_loc;

  axis_value_t properties;  // object value.

  axis_list_t locked_res;
} axis_msg_t;

axis_RUNTIME_API bool axis_raw_msg_check_integrity(axis_msg_t *self);

axis_RUNTIME_API bool axis_msg_check_integrity(axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API void axis_raw_msg_init(axis_msg_t *self,
                                              axis_MSG_TYPE type);

axis_RUNTIME_PRIVATE_API void axis_raw_msg_deinit(axis_msg_t *self);

axis_RUNTIME_PRIVATE_API void axis_raw_msg_copy_field(
    axis_msg_t *self, axis_msg_t *src, axis_list_t *excluded_field_ids);

axis_RUNTIME_PRIVATE_API void axis_raw_msg_set_src_to_loc(axis_msg_t *self,
                                                        axis_loc_t *loc);

axis_RUNTIME_PRIVATE_API void axis_msg_set_src_to_loc(axis_shared_ptr_t *self,
                                                    axis_loc_t *loc);

axis_RUNTIME_PRIVATE_API void axis_msg_set_src_to_engine(axis_shared_ptr_t *self,
                                                       axis_engine_t *engine);

axis_RUNTIME_PRIVATE_API void axis_msg_set_src_to_extension(
    axis_shared_ptr_t *self, axis_extension_t *extension);

axis_RUNTIME_PRIVATE_API void axis_msg_set_src_to_extension_group(
    axis_shared_ptr_t *self, axis_extension_group_t *extension_group);

axis_RUNTIME_PRIVATE_API void axis_msg_clear_and_set_dest_from_msg_src(
    axis_shared_ptr_t *self, axis_shared_ptr_t *cmd);

axis_RUNTIME_PRIVATE_API void axis_raw_msg_add_dest(
    axis_msg_t *self, const char *app_uri, const char *graph_id,
    const char *extension_group_name, const char *extension_name);

axis_RUNTIME_PRIVATE_API void axis_raw_msg_clear_dest(axis_msg_t *self);

axis_RUNTIME_PRIVATE_API bool axis_msg_src_is_empty(axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API const char *axis_msg_get_src_graph_id(
    axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API const char *axis_msg_get_first_dest_uri(
    axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API const char *axis_raw_msg_get_first_dest_uri(
    axis_msg_t *self);

axis_RUNTIME_PRIVATE_API axis_loc_t *axis_raw_msg_get_src_loc(axis_msg_t *self);

axis_RUNTIME_PRIVATE_API axis_loc_t *axis_raw_msg_get_first_dest_loc(
    axis_msg_t *self);

axis_RUNTIME_PRIVATE_API void axis_msg_set_src(axis_shared_ptr_t *self,
                                             const char *app_uri,
                                             const char *graph_id,
                                             const char *extension_group_name,
                                             const char *extension_name);

axis_RUNTIME_PRIVATE_API void axis_msg_set_src_uri(axis_shared_ptr_t *self,
                                                 const char *app_uri);

axis_RUNTIME_PRIVATE_API bool axis_msg_src_uri_is_empty(axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API bool axis_msg_src_graph_id_is_empty(
    axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API void axis_msg_set_src_uri_if_empty(
    axis_shared_ptr_t *self, const char *app_uri);

axis_RUNTIME_PRIVATE_API void axis_msg_set_src_engine_if_unspecified(
    axis_shared_ptr_t *self, axis_engine_t *engine);

axis_RUNTIME_PRIVATE_API size_t axis_raw_msg_get_dest_cnt(axis_msg_t *self);

axis_RUNTIME_PRIVATE_API void axis_raw_msg_clear_and_set_dest_to_loc(
    axis_msg_t *self, axis_loc_t *loc);

axis_RUNTIME_PRIVATE_API void axis_msg_set_src_to_app(axis_shared_ptr_t *self,
                                                    axis_app_t *app);

axis_RUNTIME_PRIVATE_API bool axis_msg_type_to_handle_when_closing(
    axis_shared_ptr_t *msg);

axis_RUNTIME_PRIVATE_API const char *axis_msg_type_to_string(axis_MSG_TYPE type);

axis_RUNTIME_PRIVATE_API const char *axis_raw_msg_get_type_string(
    axis_msg_t *self);

axis_RUNTIME_PRIVATE_API void axis_msg_clear_and_set_dest_from_extension_info(
    axis_shared_ptr_t *self, axis_extension_info_t *extension_info);

axis_RUNTIME_PRIVATE_API void axis_msg_correct_dest(axis_shared_ptr_t *msg,
                                                  axis_engine_t *engine);

inline bool axis_raw_msg_is_cmd_and_result(axis_msg_t *self) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  switch (self->type) {
    case axis_MSG_TYPE_CMD_CLOSE_APP:
    case axis_MSG_TYPE_CMD_STOP_GRAPH:
    case axis_MSG_TYPE_CMD_START_GRAPH:
    case axis_MSG_TYPE_CMD_TIMER:
    case axis_MSG_TYPE_CMD_TIMEOUT:
    case axis_MSG_TYPE_CMD:
    case axis_MSG_TYPE_CMD_RESULT:
      return true;

    case axis_MSG_TYPE_DATA:
    case axis_MSG_TYPE_VIDEO_FRAME:
    case axis_MSG_TYPE_AUDIO_FRAME:
      return false;

    default:
      axis_ASSERT(0, "Invalid message type %d", self->type);
      return false;
  }
}

inline bool axis_raw_msg_is_cmd(axis_msg_t *self) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  switch (self->type) {
    case axis_MSG_TYPE_CMD_CLOSE_APP:
    case axis_MSG_TYPE_CMD_STOP_GRAPH:
    case axis_MSG_TYPE_CMD_START_GRAPH:
    case axis_MSG_TYPE_CMD_TIMER:
    case axis_MSG_TYPE_CMD_TIMEOUT:
    case axis_MSG_TYPE_CMD:
      return true;

    case axis_MSG_TYPE_CMD_RESULT:
    case axis_MSG_TYPE_DATA:
    case axis_MSG_TYPE_VIDEO_FRAME:
    case axis_MSG_TYPE_AUDIO_FRAME:
      return false;

    default:
      axis_ASSERT(0, "Invalid message type %d", self->type);
      return false;
  }
}

inline bool axis_raw_msg_is_cmd_result(axis_msg_t *self) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  switch (self->type) {
    case axis_MSG_TYPE_CMD_RESULT:
      return true;

    case axis_MSG_TYPE_CMD_CLOSE_APP:
    case axis_MSG_TYPE_CMD_STOP_GRAPH:
    case axis_MSG_TYPE_CMD_START_GRAPH:
    case axis_MSG_TYPE_CMD_TIMER:
    case axis_MSG_TYPE_CMD_TIMEOUT:
    case axis_MSG_TYPE_CMD:
    case axis_MSG_TYPE_DATA:
    case axis_MSG_TYPE_VIDEO_FRAME:
    case axis_MSG_TYPE_AUDIO_FRAME:
      return false;

    default:
      axis_ASSERT(0, "Invalid message type %d", self->type);
      return false;
  }
}

axis_RUNTIME_PRIVATE_API bool axis_msg_has_locked_res(axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API void axis_msg_clear_and_set_dest_to_loc(
    axis_shared_ptr_t *self, axis_loc_t *loc);

axis_RUNTIME_PRIVATE_API axis_MSG_TYPE axis_msg_type_from_type_and_name_string(
    const char *type_str, const char *name_str);

axis_RUNTIME_PRIVATE_API const char *axis_msg_get_type_string(
    axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API axis_MSG_TYPE
axis_msg_type_from_type_string(const char *type_str);

axis_RUNTIME_PRIVATE_API axis_MSG_TYPE
axis_msg_type_from_unique_name_string(const char *name_str);

// Debug only.
axis_RUNTIME_PRIVATE_API bool axis_raw_msg_dump(axis_msg_t *msg, axis_error_t *err,
                                              const char *fmt, ...);

axis_RUNTIME_API bool axis_msg_dump(axis_shared_ptr_t *msg, axis_error_t *err,
                                  const char *fmt, ...);

axis_RUNTIME_PRIVATE_API bool axis_raw_msg_validate_schema(
    axis_msg_t *self, axis_schema_store_t *schema_store, bool is_msg_out,
    axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_msg_validate_schema(
    axis_shared_ptr_t *self, axis_schema_store_t *schema_store, bool is_msg_out,
    axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool
axis_raw_msg_get_one_field_from_json_include_internal_field(
    axis_msg_t *self, axis_msg_field_process_data_t *field, void *user_data,
    axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_raw_msg_put_one_field_to_json(
    axis_msg_t *self, axis_msg_field_process_data_t *field, void *user_data,
    axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_json_t *axis_msg_to_json_include_internal_field(
    axis_shared_ptr_t *self, axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_raw_msg_process_field(
    axis_msg_t *self, axis_raw_msg_process_one_field_func_t cb, void *user_data,
    axis_error_t *err);

axis_RUNTIME_PRIVATE_API const char *axis_msg_get_src_app_uri(
    axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API axis_loc_t *axis_msg_get_src_loc(axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API axis_loc_t *axis_msg_get_first_dest_loc(
    axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API axis_list_t *axis_msg_get_dest(axis_shared_ptr_t *self);

axis_RUNTIME_API size_t axis_msg_get_dest_cnt(axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API void axis_msg_clear_dest(axis_shared_ptr_t *self);

axis_RUNTIME_API axis_shared_ptr_t *axis_msg_create_from_msg_type(
    axis_MSG_TYPE msg_type);

axis_RUNTIME_API void axis_raw_msg_destroy(axis_msg_t *self);

axis_RUNTIME_PRIVATE_API bool axis_raw_msg_set_name(axis_msg_t *self,
                                                  const char *msg_name,
                                                  axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_raw_msg_set_name_with_len(axis_msg_t *self,
                                                           const char *msg_name,
                                                           size_t msg_name_len,
                                                           axis_error_t *err);

/**
 * @brief Set the 'graph_id' in the dest loc to the specified value.
 */
axis_RUNTIME_PRIVATE_API void
axis_msg_set_dest_engine_if_unspecified_or_predefined_graph_name(
    axis_shared_ptr_t *self, axis_engine_t *target_engine,
    axis_list_t *predefined_graph_infos);

axis_RUNTIME_PRIVATE_API bool axis_msg_set_name_with_len(axis_shared_ptr_t *self,
                                                       const char *msg_name,
                                                       size_t msg_name_len,
                                                       axis_error_t *err);

inline axis_MSG_TYPE axis_raw_msg_get_type(axis_msg_t *self) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");
  return self->type;
}

inline axis_msg_t *axis_msg_get_raw_msg(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  return (axis_msg_t *)axis_shared_ptr_get_data(self);
}

inline bool axis_msg_is_cmd_and_result(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  return axis_raw_msg_is_cmd_and_result(axis_msg_get_raw_msg(self));
}

inline bool axis_msg_is_cmd(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  return axis_raw_msg_is_cmd(axis_msg_get_raw_msg(self));
}

inline bool axis_msg_is_cmd_result(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  return axis_raw_msg_is_cmd_result(axis_msg_get_raw_msg(self));
}
