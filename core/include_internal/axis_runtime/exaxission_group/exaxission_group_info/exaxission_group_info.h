//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/common/loc.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/string.h"

#define axis_EXTENSION_GROUP_INFO_SIGNATURE 0xBC5114352AFF63AEU

typedef struct axis_property_t axis_property_t;
typedef struct axis_error_t axis_error_t;

// Regardless of whether the extension group is in the current process, all
// extension groups in a graph will have an extension_group_info to store the
// static information of that extension group.
typedef struct axis_extension_group_info_t {
  axis_signature_t signature;

  axis_string_t extension_group_addon_name;
  axis_loc_t loc;

  // The definition of properties in the graph related to the current extension
  // group.
  axis_value_t *property;
} axis_extension_group_info_t;

axis_RUNTIME_PRIVATE_API bool axis_extension_group_info_check_integrity(
    axis_extension_group_info_t *self);

axis_RUNTIME_PRIVATE_API axis_extension_group_info_t *
axis_extension_group_info_create(void);

axis_RUNTIME_PRIVATE_API void axis_extension_group_info_destroy(
    axis_extension_group_info_t *self);

axis_RUNTIME_PRIVATE_API axis_extension_group_info_t *
axis_extension_group_info_from_smart_ptr(
    axis_smart_ptr_t *extension_group_info_smart_ptr);

axis_RUNTIME_PRIVATE_API axis_shared_ptr_t *
get_extension_group_info_in_extension_groups_info(
    axis_list_t *extension_groups_info, const char *app_uri,
    const char *graph_id, const char *extension_group_addon_name,
    const char *extension_group_instance_name, bool *new_one_created,
    axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_shared_ptr_t *axis_extension_group_info_clone(
    axis_extension_group_info_t *self, axis_list_t *extension_groups_info);

axis_RUNTIME_PRIVATE_API void axis_extension_groups_info_fill_app_uri(
    axis_list_t *extension_groups_info, const char *app_uri);

axis_RUNTIME_PRIVATE_API void axis_extension_groups_info_fill_graph_id(
    axis_list_t *extension_groups_info, const char *graph_id);
