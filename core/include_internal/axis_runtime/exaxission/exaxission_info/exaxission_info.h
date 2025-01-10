//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/common/loc.h"
#include "include_internal/axis_runtime/extension/msg_dest_info/all_msg_type_dest_info.h"
#include "axis_utils/container/list.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"

#define axis_EXTENSION_INFO_SIGNATURE 0xE313C401D4C0C3C2U

typedef struct axis_property_t axis_property_t;
typedef struct axis_error_t axis_error_t;

// Regardless of whether the extension is in the current process, all extensions
// in a graph will have an extension_info to store the static information of
// that extension.
typedef struct axis_extension_info_t {
  axis_signature_t signature;
  axis_sanitizer_thread_check_t thread_check;

  axis_string_t extension_addon_name;
  axis_loc_t loc;

  // The extension_info of the destination extension for each type of message.
  axis_all_msg_type_dest_info_t msg_dest_info;

  // The definition of properties in the graph related to the current extension.
  axis_value_t *property;

  axis_list_t msg_conversion_contexts;  // axis_msg_conversion_context_t
} axis_extension_info_t;

axis_RUNTIME_PRIVATE_API axis_extension_info_t *axis_extension_info_create(void);

axis_RUNTIME_PRIVATE_API bool axis_extensions_info_clone(axis_list_t *from,
                                                       axis_list_t *to,
                                                       axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_extension_info_check_integrity(
    axis_extension_info_t *self, bool check_thread);

axis_RUNTIME_PRIVATE_API void axis_extension_info_translate_localhost_to_app_uri(
    axis_extension_info_t *self, const char *app_uri);

axis_RUNTIME_PRIVATE_API bool axis_extension_info_is_desired_extension_group(
    axis_extension_info_t *self, const char *app_uri,
    const char *extension_group_instance_name);

axis_RUNTIME_PRIVATE_API axis_shared_ptr_t *get_extension_info_in_extensions_info(
    axis_list_t *extensions_info, const char *app_uri, const char *graph_id,
    const char *extension_group_name, const char *extension_addon_name,
    const char *extension_instance_name, bool should_exist, axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_extension_info_t *axis_extension_info_from_smart_ptr(
    axis_smart_ptr_t *smart_ptr);

axis_RUNTIME_PRIVATE_API void axis_extensions_info_fill_app_uri(
    axis_list_t *extensions_info, const char *app_uri);

axis_RUNTIME_PRIVATE_API void axis_extensions_info_fill_loc_info(
    axis_list_t *extensions_info, const char *app_uri, const char *graph_id);
