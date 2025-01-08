//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_runtime/axis_env/internal/send.h"
#include "axis_utils/lib/smart_ptr.h"

#define axis_CMD_BASE_SIGNATURE 0x0DF810096247FFD5U

typedef struct axis_connection_t axis_connection_t;

// Every command struct should starts with this.
typedef struct axis_cmd_base_t {
  axis_msg_t msg_hdr;

  axis_signature_t signature;

  // If the command is cloned from another command, this field is used to create
  // the relationship between these 2 commands.
  axis_string_t parent_cmd_id;

  axis_value_t cmd_id;  // string. This is used in TEN runtime internally.
  axis_value_t seq_id;  // string. This is used in TEN client.

  // The origin where the command is originated.
  //
  // This is kind of a cache to enable us not to loop all the remotes to find
  // the correct one.
  //
  // If any remote of an engine is closed, then it will trigger the closing of
  // the engine, and no cmds could be processed any further. So we don't need
  // to use sharedptr to wrap the following variable, because when a command
  // is being processed, the origin must be alive.
  axis_connection_t *original_connection;

  axis_env_msg_result_handler_func_t result_handler;
  void *result_handler_data;
} axis_cmd_base_t;

axis_RUNTIME_API bool axis_raw_cmd_base_check_integrity(axis_cmd_base_t *self);

axis_RUNTIME_API bool axis_cmd_base_check_integrity(axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API const char *axis_cmd_base_gen_new_cmd_id_forcibly(
    axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API const char *axis_raw_cmd_base_gen_new_cmd_id_forcibly(
    axis_cmd_base_t *self);

axis_RUNTIME_PRIVATE_API axis_cmd_base_t *axis_cmd_base_get_raw_cmd_base(
    axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API const char *axis_cmd_base_get_seq_id(
    axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API void axis_cmd_base_set_seq_id(axis_shared_ptr_t *self,
                                                     const char *seq_id);

axis_RUNTIME_PRIVATE_API const char *axis_cmd_base_get_cmd_id(
    axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API void axis_cmd_base_set_cmd_id(axis_shared_ptr_t *self,
                                                     const char *cmd_id);

axis_RUNTIME_PRIVATE_API void axis_raw_cmd_base_init(axis_cmd_base_t *self,
                                                   axis_MSG_TYPE type);

axis_RUNTIME_PRIVATE_API void axis_raw_cmd_base_deinit(axis_cmd_base_t *self);

axis_RUNTIME_PRIVATE_API void axis_raw_cmd_base_copy_field(
    axis_msg_t *self, axis_msg_t *src, axis_list_t *excluded_field_ids);

axis_RUNTIME_PRIVATE_API bool axis_raw_cmd_base_process_field(
    axis_msg_t *self, axis_raw_msg_process_one_field_func_t cb, void *user_data,
    axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_string_t *axis_cmd_base_gen_cmd_id_if_empty(
    axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API void axis_raw_cmd_base_set_cmd_id(axis_cmd_base_t *self,
                                                         const char *cmd_id);

axis_RUNTIME_PRIVATE_API axis_string_t *axis_raw_cmd_base_get_cmd_id(
    axis_cmd_base_t *self);

axis_RUNTIME_PRIVATE_API void axis_cmd_base_save_cmd_id_to_parent_cmd_id(
    axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API void axis_raw_cmd_base_save_cmd_id_to_parent_cmd_id(
    axis_cmd_base_t *self);

axis_RUNTIME_PRIVATE_API void axis_raw_cmd_base_set_seq_id(axis_cmd_base_t *self,
                                                         const char *seq_id);

axis_RUNTIME_PRIVATE_API bool axis_cmd_base_cmd_id_is_empty(
    axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API axis_connection_t *axis_cmd_base_get_original_connection(
    axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API void axis_cmd_base_set_original_connection(
    axis_shared_ptr_t *self, axis_connection_t *connection);

axis_RUNTIME_PRIVATE_API const char *axis_cmd_base_get_parent_cmd_id(
    axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API void axis_cmd_base_reset_parent_cmd_id(
    axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API axis_string_t *axis_raw_cmd_base_get_seq_id(
    axis_cmd_base_t *self);

axis_RUNTIME_PRIVATE_API void axis_cmd_base_set_result_handler(
    axis_shared_ptr_t *self, axis_env_msg_result_handler_func_t result_handler,
    void *result_handler_data);

/**
 * @brief Whether this cmd comes from the client outside of TEN world, e.g.:
 * browsers.
 */
axis_RUNTIME_PRIVATE_API bool axis_cmd_base_comes_from_client_outside(
    axis_shared_ptr_t *self);
