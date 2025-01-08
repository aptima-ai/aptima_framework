//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/container/list.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/string.h"

typedef struct axis_app_t axis_app_t;
typedef struct axis_engine_t axis_engine_t;

typedef struct axis_predefined_graph_info_t {
  axis_string_t name;

  bool auto_start;
  bool singleton;

  // axis_shared_ptr_t of axis_extension_info_t
  axis_list_t extensions_info;
  // axis_shared_ptr_t of axis_extension_group_info_t
  axis_list_t extension_groups_info;

  // 'singleton == true' means that there can only be one instance of this
  // predefined_graph in the entire process. In this case, the predefined graph
  // name is the same as the graph ID of that instance. Therefore, the 'engine'
  // field is only meaningful when the 'singleton' field is set to true.
  axis_engine_t *engine;
} axis_predefined_graph_info_t;

axis_RUNTIME_PRIVATE_API axis_predefined_graph_info_t *
axis_predefined_graph_info_create(void);

axis_RUNTIME_PRIVATE_API void axis_predefined_graph_info_destroy(
    axis_predefined_graph_info_t *self);

axis_RUNTIME_PRIVATE_API bool axis_app_start_auto_start_predefined_graph(
    axis_app_t *self, axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_app_start_predefined_graph(
    axis_app_t *self, axis_predefined_graph_info_t *predefined_graph_info,
    axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_predefined_graph_info_t *
axis_app_get_singleton_predefined_graph_info_by_name(axis_app_t *self,
                                                    const char *name);

axis_RUNTIME_PRIVATE_API axis_predefined_graph_info_t *
axis_predefined_graph_infos_get_singleton_by_name(
    axis_list_t *predefined_graph_infos, const char *name);

axis_RUNTIME_PRIVATE_API axis_engine_t *
axis_app_get_singleton_predefined_graph_engine_by_name(axis_app_t *self,
                                                      const char *name);

axis_RUNTIME_PRIVATE_API bool axis_app_get_predefined_graphs_from_property(
    axis_app_t *self);
