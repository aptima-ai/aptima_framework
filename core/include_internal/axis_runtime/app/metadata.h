//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>
#include <stddef.h>

#include "axis_utils/lib/error.h"

typedef struct axis_app_t axis_app_t;
typedef struct axis_list_t axis_list_t;
typedef struct axis_string_t axis_string_t;
typedef struct axis_value_t axis_value_t;

typedef bool (*axis_app_axis_namespace_prop_init_from_value_func_t)(
    axis_app_t *self, axis_value_t *value);

typedef struct axis_app_axis_namespace_prop_info_t {
  const char *name;
  axis_app_axis_namespace_prop_init_from_value_func_t init_from_value;
} axis_app_axis_namespace_prop_info_t;

axis_RUNTIME_PRIVATE_API void axis_app_handle_metadata(axis_app_t *self);

axis_RUNTIME_PRIVATE_API bool axis_app_handle_axis_namespace_properties(
    axis_app_t *self);

axis_RUNTIME_PRIVATE_API bool
axis_app_get_predefined_graph_extensions_and_groups_info_by_name(
    axis_app_t *self, const char *name, axis_list_t *extensions_info,
    axis_list_t *extension_groups_info, axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_app_init_one_event_loop_per_engine(
    axis_app_t *self, axis_value_t *value);

axis_RUNTIME_PRIVATE_API axis_value_t *axis_app_get_axis_namespace_properties(
    axis_app_t *self);

axis_RUNTIME_PRIVATE_API bool axis_app_init_long_running_mode(axis_app_t *self,
                                                            axis_value_t *value);

axis_RUNTIME_PRIVATE_API bool axis_app_init_uri(axis_app_t *self,
                                              axis_value_t *value);

axis_RUNTIME_PRIVATE_API bool axis_app_init_log_level(axis_app_t *self,
                                                    axis_value_t *value);

axis_RUNTIME_PRIVATE_API bool axis_app_init_log_file(axis_app_t *self,
                                                   axis_value_t *value);
