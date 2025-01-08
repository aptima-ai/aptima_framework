//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension_group/extension_group_info/extension_group_info.h"

#include <stdlib.h>

#include "include_internal/axis_runtime/common/loc.h"
#include "axis_runtime/common/errno.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value.h"

bool axis_extension_group_info_check_integrity(
    axis_extension_group_info_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_EXTENSION_GROUP_INFO_SIGNATURE) {
    return false;
  }

  return true;
}

axis_extension_group_info_t *axis_extension_group_info_create(void) {
  axis_extension_group_info_t *self = (axis_extension_group_info_t *)axis_MALLOC(
      sizeof(axis_extension_group_info_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_signature_set(&self->signature, axis_EXTENSION_GROUP_INFO_SIGNATURE);

  axis_string_init(&self->extension_group_addon_name);

  axis_loc_init_empty(&self->loc);
  self->property = axis_value_create_object_with_move(NULL);

  return self;
}

void axis_extension_group_info_destroy(axis_extension_group_info_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_group_info_check_integrity(self),
             "Invalid use of extension_info %p.", self);

  axis_signature_set(&self->signature, 0);

  axis_string_deinit(&self->extension_group_addon_name);

  axis_loc_deinit(&self->loc);

  if (self->property) {
    axis_value_destroy(self->property);
  }

  axis_FREE(self);
}

static bool axis_extension_group_info_is_specified_extension_group(
    axis_extension_group_info_t *self, const char *app_uri, const char *graph_id,
    const char *extension_group_name) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: The graph-related information of the extension group remains
  // unchanged during the lifecycle of engine/graph, allowing safe
  // cross-thread access.
  axis_ASSERT(axis_extension_group_info_check_integrity(self),
             "Invalid use of extension_group_info %p.", self);

  axis_ASSERT(extension_group_name, "Invalid argument.");

  if (app_uri && !axis_string_is_equal_c_str(&self->loc.app_uri, app_uri)) {
    return false;
  }

  if (graph_id && !axis_string_is_equal_c_str(&self->loc.graph_id, graph_id)) {
    return false;
  }

  if (extension_group_name &&
      !axis_string_is_equal_c_str(&self->loc.extension_group_name,
                                 extension_group_name)) {
    return false;
  }

  return true;
}

axis_extension_group_info_t *axis_extension_group_info_from_smart_ptr(
    axis_smart_ptr_t *extension_group_info_smart_ptr) {
  axis_ASSERT(extension_group_info_smart_ptr, "Invalid argument.");
  return axis_smart_ptr_get_data(extension_group_info_smart_ptr);
}

axis_shared_ptr_t *get_extension_group_info_in_extension_groups_info(
    axis_list_t *extension_groups_info, const char *app_uri,
    const char *graph_id, const char *extension_group_addon_name,
    const char *extension_group_instance_name, bool *new_one_created,
    axis_error_t *err) {
  axis_ASSERT(extension_groups_info, "Should not happen.");
  axis_ASSERT(
      extension_group_instance_name && strlen(extension_group_instance_name),
      "Invalid argument.");

  axis_extension_group_info_t *extension_group_info = NULL;

  // Find the corresponding extension_group_info according to the instance name
  // of extension_group only. Check if there are any other extension groups with
  // different extension_group settings. This step is to find if there are any
  // other extension groups with different extension_group addon name.
  axis_listnode_t *extension_group_info_node = axis_list_find_shared_ptr_custom_3(
      extension_groups_info, app_uri, graph_id, extension_group_instance_name,
      axis_extension_group_info_is_specified_extension_group);
  if (extension_group_info_node) {
    extension_group_info = axis_shared_ptr_get_data(
        axis_smart_ptr_listnode_get(extension_group_info_node));
    axis_ASSERT(extension_group_info && axis_extension_group_info_check_integrity(
                                           extension_group_info),
               "Should not happen.");

    if (new_one_created) {
      *new_one_created = false;
    }

    if (strlen(extension_group_addon_name) &&
        !axis_string_is_empty(
            &extension_group_info->extension_group_addon_name) &&
        !axis_string_is_equal_c_str(
            &extension_group_info->extension_group_addon_name,
            extension_group_addon_name)) {
      if (err) {
        axis_error_set(
            err, axis_ERRNO_INVALID_GRAPH,
            "extension group '%s' is associated with different addon '%s', "
            "'%s'",
            extension_group_instance_name, extension_group_addon_name,
            axis_string_get_raw_str(
                &extension_group_info->extension_group_addon_name));
      } else {
        axis_ASSERT(0,
                   "extension group '%s' is associated with different addon "
                   "'%s', '%s'",
                   extension_group_instance_name, extension_group_addon_name,
                   axis_string_get_raw_str(
                       &extension_group_info->extension_group_addon_name));
      }

      return NULL;
    }

    // If we know the extension group addon name now, add this data to the
    // extension_info.
    if (strlen(extension_group_addon_name) &&
        axis_string_is_empty(
            &extension_group_info->extension_group_addon_name)) {
      axis_string_set_formatted(
          &extension_group_info->extension_group_addon_name, "%s",
          extension_group_addon_name);
    }

    return axis_smart_ptr_listnode_get(extension_group_info_node);
  }

  axis_extension_group_info_t *self = axis_extension_group_info_create();

  axis_loc_set(&self->loc, app_uri, graph_id, extension_group_instance_name,
              NULL);

  // Add the extension group addon name if we know it now.
  if (strlen(extension_group_addon_name)) {
    axis_string_set_formatted(&self->extension_group_addon_name, "%s",
                             extension_group_addon_name);
  }

  axis_shared_ptr_t *shared_self =
      axis_shared_ptr_create(self, axis_extension_group_info_destroy);
  axis_shared_ptr_t *shared_self_ =
      axis_list_push_smart_ptr_back(extension_groups_info, shared_self);
  axis_shared_ptr_destroy(shared_self);

  if (new_one_created) {
    *new_one_created = true;
  }
  return shared_self_;
}

axis_shared_ptr_t *axis_extension_group_info_clone(
    axis_extension_group_info_t *self, axis_list_t *extension_groups_info) {
  axis_ASSERT(extension_groups_info, "Should not happen.");

  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: The graph-related information of the extension remains
  // unchanged during the lifecycle of engine/graph, allowing safe
  // cross-thread access.
  axis_ASSERT(axis_extension_group_info_check_integrity(self),
             "Invalid use of extension_group_info %p.", self);

  bool new_extension_group_info_created = false;
  axis_shared_ptr_t *new_dest =
      get_extension_group_info_in_extension_groups_info(
          extension_groups_info, axis_string_get_raw_str(&self->loc.app_uri),
          axis_string_get_raw_str(&self->loc.graph_id),
          axis_string_get_raw_str(&self->extension_group_addon_name),
          axis_string_get_raw_str(&self->loc.extension_group_name),
          &new_extension_group_info_created, NULL);

  return new_dest;
}

static void axis_extension_group_info_fill_app_uri(
    axis_extension_group_info_t *self, const char *app_uri) {
  axis_ASSERT(self && axis_extension_group_info_check_integrity(self),
             "Invalid argument.");
  axis_ASSERT(app_uri, "Should not happen.");
  axis_ASSERT(!axis_loc_is_empty(&self->loc), "Should not happen.");

  // Fill the app uri of the extension_info if it is empty.
  if (axis_string_is_empty(&self->loc.app_uri)) {
    axis_string_set_formatted(&self->loc.app_uri, "%s", app_uri);
  }
}

void axis_extension_groups_info_fill_app_uri(axis_list_t *extension_groups_info,
                                            const char *app_uri) {
  axis_list_foreach (extension_groups_info, iter) {
    axis_extension_group_info_t *extension_group_info =
        axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(iter.node));
    axis_ASSERT(extension_group_info && axis_extension_group_info_check_integrity(
                                           extension_group_info),
               "Invalid argument.");

    axis_extension_group_info_fill_app_uri(extension_group_info, app_uri);
  }
}

static void axis_extension_group_info_fill_graph_id(
    axis_extension_group_info_t *self, const char *graph_id) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_group_info_check_integrity(self),
             "Invalid use of extension_group_info %p.", self);

  axis_string_set_formatted(&self->loc.graph_id, "%s", graph_id);
}

void axis_extension_groups_info_fill_graph_id(axis_list_t *extension_groups_info,
                                             const char *graph_id) {
  axis_list_foreach (extension_groups_info, iter) {
    axis_extension_group_info_t *extension_group_info =
        axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(iter.node));
    axis_ASSERT(extension_group_info && axis_extension_group_info_check_integrity(
                                           extension_group_info),
               "Invalid argument.");

    axis_extension_group_info_fill_graph_id(extension_group_info, graph_id);
  }
}
