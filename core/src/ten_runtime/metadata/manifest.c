//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/metadata/manifest.h"

#include "ten_utils/log/log.h"

void ten_manifest_dependencies_get_dependencies_type_and_name(
    ten_value_t *manifest_dependencies, ten_list_t *extension_list,
    ten_list_t *extension_group_list, ten_list_t *protocol_list) {
  TEN_ASSERT(manifest_dependencies,
             "Invalid argument: manifest_dependencies is NULL.");
  TEN_ASSERT(ten_value_check_integrity(manifest_dependencies),
             "Invalid manifest_dependencies value.");
  // Ensure that "dependencies" is an array.
  TEN_ASSERT(ten_value_is_array(manifest_dependencies),
             "The 'dependencies' field should be an array.");
  TEN_ASSERT(extension_list, "Invalid argument: extension_list is NULL.");
  TEN_ASSERT(extension_group_list,
             "Invalid argument: extension_group_list is NULL.");
  TEN_ASSERT(protocol_list, "Invalid argument: protocol_list is NULL.");

  // Iterate over each dependency in the array.
  ten_value_array_foreach(manifest_dependencies, iter) {
    ten_value_t *dep = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(dep, "Dependency is NULL.");
    TEN_ASSERT(ten_value_check_integrity(dep), "Invalid dependency value.");
    TEN_ASSERT(ten_value_is_object(dep),
               "Each dependency should be an object.");

    // Get the "type" field of the dependency.
    ten_value_t *type_value = ten_value_object_peek(dep, "type");
    TEN_ASSERT(type_value, "Dependency missing 'type' field.");
    TEN_ASSERT(ten_value_is_string(type_value),
               "The 'type' field should be a string.");
    ten_string_t *type_str = ten_value_peek_string(type_value);

    // Get the "name" field of the dependency.
    ten_value_t *name_value = ten_value_object_peek(dep, "name");
    TEN_ASSERT(name_value, "Dependency missing 'name' field.");
    TEN_ASSERT(ten_value_is_string(name_value),
               "The 'name' field should be a string.");
    ten_string_t *name_str = ten_value_peek_string(name_value);
    const char *name_cstr = ten_string_get_raw_str(name_str);

    // Add the dependency name to the appropriate list based on its type.
    if (ten_string_is_equal_c_str(type_str, "extension")) {
      TEN_LOGI("Collect extension dependency: %s", name_cstr);
      ten_list_push_str_back(extension_list, name_cstr);
    } else if (ten_string_is_equal_c_str(type_str, "extension_group")) {
      TEN_LOGI("Collect extension_group dependency: %s", name_cstr);
      ten_list_push_str_back(extension_group_list, name_cstr);
    } else if (ten_string_is_equal_c_str(type_str, "protocol")) {
      TEN_LOGI("Collect protocol dependency: %s", name_cstr);
      ten_list_push_str_back(protocol_list, name_cstr);
    }
  }
}

void ten_manifest_get_dependencies_type_and_name(
    ten_value_t *manifest, ten_list_t *extension_list,
    ten_list_t *extension_group_list, ten_list_t *protocol_list) {
  TEN_ASSERT(manifest, "Invalid argument: manifest is NULL.");
  TEN_ASSERT(extension_list, "Invalid argument: extension_list is NULL.");
  TEN_ASSERT(extension_group_list,
             "Invalid argument: extension_group_list is NULL.");
  TEN_ASSERT(protocol_list, "Invalid argument: protocol_list is NULL.");
  TEN_ASSERT(ten_value_check_integrity(manifest), "Invalid manifest value.");
  TEN_ASSERT(ten_value_is_object(manifest), "Manifest should be an object.");

  // Retrieve the "dependencies" field from the manifest.
  ten_value_t *dependencies = ten_value_object_peek(manifest, "dependencies");
  if (!dependencies) {
    // No dependencies found; nothing to do.
    return;
  }

  ten_manifest_dependencies_get_dependencies_type_and_name(
      dependencies, extension_list, extension_group_list, protocol_list);
}
