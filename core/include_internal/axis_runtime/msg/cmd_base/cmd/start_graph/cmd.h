//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/msg/cmd_base/cmd/cmd.h"

typedef struct axis_cmd_start_graph_t {
  axis_cmd_t cmd_hdr;

  axis_value_t long_running_mode;  // bool

  // There are two methods to start a graph:
  // 1) one is by explicitly specifying the graph's content through
  //    extension_groups_info and extensions_info.
  // 2) The other is by specifying only a predefined graph name, and then
  //    finding the corresponding graph definition from the app's predefined
  //    graph database.

  // The name of the specified predefined graph.
  axis_value_t predefined_graph_name;  // string

  // axis_shared_ptr_t of axis_extension_group_info_t
  axis_list_t extension_groups_info;

  // axis_shared_ptr_t of axis_extension_info_t
  axis_list_t extensions_info;
} axis_cmd_start_graph_t;

axis_RUNTIME_PRIVATE_API void
axis_cmd_start_graph_collect_all_immediate_connectable_apps(
    axis_shared_ptr_t *self, axis_app_t *app, axis_list_t *next);

axis_RUNTIME_PRIVATE_API axis_list_t *axis_raw_cmd_start_graph_get_extensions_info(
    axis_cmd_start_graph_t *self);

axis_RUNTIME_PRIVATE_API axis_list_t *axis_cmd_start_graph_get_extensions_info(
    axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API axis_list_t *
axis_raw_cmd_start_graph_get_extension_groups_info(axis_cmd_start_graph_t *self);

axis_RUNTIME_PRIVATE_API axis_list_t *
axis_cmd_start_graph_get_extension_groups_info(axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API void
axis_cmd_start_graph_add_missing_extension_group_node(axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API bool axis_raw_cmd_start_graph_get_long_running_mode(
    axis_cmd_start_graph_t *self);

axis_RUNTIME_PRIVATE_API bool axis_cmd_start_graph_get_long_running_mode(
    axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API axis_string_t *
axis_raw_cmd_start_graph_get_predefined_graph_name(axis_cmd_start_graph_t *self);

axis_RUNTIME_PRIVATE_API axis_string_t *
axis_cmd_start_graph_get_predefined_graph_name(axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API axis_list_t
axis_cmd_start_graph_get_requested_extension_names(axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API void axis_cmd_start_graph_fill_loc_info(
    axis_shared_ptr_t *self, const char *app_uri, const char *graph_id);

axis_RUNTIME_PRIVATE_API bool axis_raw_cmd_start_graph_init_from_json(
    axis_cmd_start_graph_t *self, axis_json_t *json, axis_error_t *err);

axis_RUNTIME_PRIVATE_API void axis_raw_cmd_start_graph_as_msg_destroy(
    axis_msg_t *self);

axis_RUNTIME_PRIVATE_API axis_list_t
axis_cmd_start_graph_get_extension_addon_and_instance_name_pairs_of_specified_extension_group(
    axis_shared_ptr_t *self, const char *app_uri, const char *graph_id,
    const char *extension_group_name);

axis_RUNTIME_PRIVATE_API axis_json_t *axis_raw_cmd_start_graph_to_json(
    axis_msg_t *self, axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_cmd_start_graph_t *axis_raw_cmd_start_graph_create(
    void);

axis_RUNTIME_PRIVATE_API axis_msg_t *axis_raw_cmd_start_graph_as_msg_clone(
    axis_msg_t *self, axis_list_t *excluded_field_ids);

axis_RUNTIME_PRIVATE_API bool axis_raw_cmd_start_graph_loop_all_fields(
    axis_msg_t *self, axis_raw_msg_process_one_field_func_t cb, void *user_data,
    axis_error_t *err);
