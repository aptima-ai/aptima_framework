//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension/extension_info/extension_info.h"

#include <stdlib.h>

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/common/loc.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension/msg_dest_info/msg_dest_info.h"
#include "include_internal/axis_runtime/msg_conversion/msg_conversion_context.h"
#include "axis_runtime/common/errno.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node_ptr.h"
#include "axis_utils/container/list_node_smart_ptr.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value.h"

axis_extension_info_t *axis_extension_info_create(void) {
  axis_extension_info_t *self =
      (axis_extension_info_t *)axis_MALLOC(sizeof(axis_extension_info_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_signature_set(&self->signature,
                    (axis_signature_t)axis_EXTENSION_INFO_SIGNATURE);
  axis_sanitizer_thread_check_init_with_current_thread(&self->thread_check);

  axis_string_init(&self->extension_addon_name);

  axis_loc_init_empty(&self->loc);

  self->property = axis_value_create_object_with_move(NULL);

  axis_list_init(&self->msg_conversion_contexts);

  axis_all_msg_type_dest_info_init(&self->msg_dest_info);

  return self;
}

bool axis_extension_info_is_desired_extension_group(
    axis_extension_info_t *self, const char *app_uri,
    const char *extension_group_name) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: The graph-related information of the extension remains
  // unchanged during the lifecycle of engine/graph, allowing safe
  // cross-thread access.
  axis_ASSERT(axis_extension_info_check_integrity(self, false),
             "Invalid use of extension_info %p.", self);

  axis_ASSERT(app_uri && extension_group_name, "Should not happen.");

  if (axis_string_is_equal_c_str(&self->loc.app_uri, app_uri) &&
      axis_string_is_equal_c_str(&self->loc.extension_group_name,
                                extension_group_name)) {
    return true;
  }
  return false;
}

static bool axis_extension_info_is_specified_extension(
    axis_extension_info_t *self, const char *app_uri, const char *graph_id,
    const char *extension_group_name, const char *extension_name) {
  axis_ASSERT(
      self && axis_extension_info_check_integrity(self, true) && extension_name,
      "Should not happen.");

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

  if (extension_name &&
      !axis_string_is_equal_c_str(&self->loc.extension_name, extension_name)) {
    return false;
  }

  return true;
}

static void axis_extension_info_destroy(axis_extension_info_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: In APTIMA, the destroy operation should only be allowed to be
  // invoked when there are no thread safety issues present.
  axis_ASSERT(axis_extension_info_check_integrity(self, false),
             "Invalid use of extension_info %p.", self);

  axis_sanitizer_thread_check_deinit(&self->thread_check);
  axis_signature_set(&self->signature, 0);

  axis_all_msg_type_dest_info_deinit(&self->msg_dest_info);

  axis_string_deinit(&self->extension_addon_name);

  axis_loc_deinit(&self->loc);

  if (self->property) {
    axis_value_destroy(self->property);
  }
  axis_list_clear(&self->msg_conversion_contexts);

  axis_FREE(self);
}

// 1. All extension_info will be stored in the `extensions_info`, only including
//    those defined in the `nodes` section. Any extension_info used in
//    `connections` must be declared in the `nodes`.
//
// 2. All extension_info in the `extensions_info` are unique, which is
//    identified by the `loc` field.
//
// 3. Each extension_info in the `extensions_info` is a shared_ptr, and if one
//    is used in the `dest` section, a weak_ptr will be created to reference it
//    to avoid there is a cycle in the graph.
//
// Parameters:
//
// - should_exist: the extension_info should be found in `extensions_info` if
//   true. If we are parsing the `nodes` section, it should be false. And if we
//   are parsing the `connections` section, it should be true.
axis_shared_ptr_t *get_extension_info_in_extensions_info(
    axis_list_t *extensions_info, const char *app_uri, const char *graph_id,
    const char *extension_group_name, const char *extension_addon_name,
    const char *extension_instance_name, bool should_exist, axis_error_t *err) {
  axis_ASSERT(extensions_info && extension_instance_name, "Should not happen.");

  if (!should_exist) {
    axis_ASSERT(
        extension_addon_name,
        "Expect to be a create request, the extension_addon_name is required.");
  }

  axis_extension_info_t *extension_info = NULL;

  // Find the corresponding extension_info according to the instance name of
  // extension_group and extension.
  axis_listnode_t *extension_info_node = axis_list_find_shared_ptr_custom_4(
      extensions_info, app_uri, graph_id, extension_group_name,
      extension_instance_name, axis_extension_info_is_specified_extension);

  if (extension_info_node) {
    extension_info = axis_shared_ptr_get_data(
        axis_smart_ptr_listnode_get(extension_info_node));
    axis_ASSERT(extension_info &&
                   axis_extension_info_check_integrity(extension_info, true),
               "Should not happen.");

    // The extension addon name should be equal if both specified.
    if (extension_addon_name && !axis_c_string_is_empty(extension_addon_name) &&
        !axis_string_is_equal_c_str(&extension_info->extension_addon_name,
                                   extension_addon_name)) {
      if (err) {
        axis_error_set(
            err, axis_ERRNO_INVALID_GRAPH,
            "extension '%s' is associated with different addon '%s', "
            "'%s'",
            extension_instance_name, extension_addon_name,
            axis_string_get_raw_str(&extension_info->extension_addon_name));
      } else {
        axis_ASSERT(
            0,
            "extension '%s' is associated with different addon '%s', "
            "'%s'",
            extension_instance_name, extension_addon_name,
            axis_string_get_raw_str(&extension_info->extension_addon_name));
      }
      return NULL;
    }

    if (!should_exist) {
      if (extension_info_node) {
        if (err) {
          axis_error_set(err, axis_ERRNO_INVALID_GRAPH,
                        "The extension_info is duplicated, extension_group: "
                        "%s, extension : %s.",
                        extension_group_name, extension_instance_name);
        } else {
          axis_ASSERT(0,
                     "The extension_info is duplicated, extension_group: %s, "
                     "extension: %s.",
                     extension_group_name, extension_instance_name);
        }

        return NULL;
      }
    }

    return axis_smart_ptr_listnode_get(extension_info_node);
  } else {
    if (should_exist) {
      if (err) {
        axis_error_set(err, axis_ERRNO_INVALID_GRAPH,
                      "The extension_info is not found, extension_group: %s, "
                      "extension: %s.",
                      extension_group_name, extension_instance_name);
      } else {
        axis_ASSERT(0,
                   "The extension_info is not found, extension_group: %s, "
                   "extension: %s.",
                   extension_group_name, extension_instance_name);
      }

      return NULL;
    }
  }

  axis_extension_info_t *self = axis_extension_info_create();
  axis_loc_set(&self->loc, app_uri, graph_id, extension_group_name,
              extension_instance_name);
  axis_string_set_formatted(&self->extension_addon_name, "%s",
                           extension_addon_name);

  axis_shared_ptr_t *shared_self =
      axis_shared_ptr_create(self, axis_extension_info_destroy);
  axis_shared_ptr_t *shared_self_ =
      axis_list_push_smart_ptr_back(extensions_info, shared_self);
  axis_shared_ptr_destroy(shared_self);

  return shared_self_;
}

static bool copy_msg_dest(axis_list_t *to_static_info,
                          axis_list_t *from_static_info,
                          axis_list_t *extensions_info, axis_error_t *err) {
  axis_ASSERT(to_static_info && extensions_info, "Should not happen.");

  axis_list_foreach (from_static_info, iter) {
    axis_shared_ptr_t *msg_dest_static_info =
        axis_smart_ptr_listnode_get(iter.node);

    axis_shared_ptr_t *new_msg_dest_static_info =
        axis_msg_dest_info_clone(msg_dest_static_info, extensions_info, err);
    if (!new_msg_dest_static_info) {
      return false;
    }

    axis_list_push_smart_ptr_back(to_static_info, new_msg_dest_static_info);

    axis_shared_ptr_destroy(new_msg_dest_static_info);
  }

  return true;
}

static axis_shared_ptr_t *axis_extension_info_clone_except_dest(
    axis_extension_info_t *self, axis_list_t *extensions_info, axis_error_t *err) {
  axis_ASSERT(extensions_info, "Should not happen.");

  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: The graph-related information of the extension remains
  // unchanged during the lifecycle of engine/graph, allowing safe
  // cross-thread access.
  axis_ASSERT(axis_extension_info_check_integrity(self, false),
             "Invalid use of extension_info %p.", self);

  axis_shared_ptr_t *new_dest = get_extension_info_in_extensions_info(
      extensions_info, axis_string_get_raw_str(&self->loc.app_uri),
      axis_string_get_raw_str(&self->loc.graph_id),
      axis_string_get_raw_str(&self->loc.extension_group_name),
      axis_string_get_raw_str(&self->extension_addon_name),
      axis_string_get_raw_str(&self->loc.extension_name),
      /* should_exist = */ false, err);
  axis_ASSERT(new_dest, "Should not happen.");

  axis_extension_info_t *new_extension_info = axis_shared_ptr_get_data(new_dest);
  axis_ASSERT(new_extension_info &&
                 axis_extension_info_check_integrity(new_extension_info, true),
             "Should not happen.");

  axis_value_object_merge_with_clone(new_extension_info->property,
                                    self->property);

  axis_list_foreach (&self->msg_conversion_contexts, iter) {
    axis_msg_conversion_context_t *msg_conversion =
        axis_ptr_listnode_get(iter.node);
    axis_ASSERT(msg_conversion &&
                   axis_msg_conversion_context_check_integrity(msg_conversion),
               "Should not happen.");

    bool rc = axis_msg_conversion_context_merge(
        &new_extension_info->msg_conversion_contexts, msg_conversion, err);
    axis_ASSERT(rc, "Should not happen.");
  }

  return new_dest;
}

static axis_shared_ptr_t *axis_extension_info_clone_dest(
    axis_extension_info_t *self, axis_list_t *extensions_info, axis_error_t *err) {
  axis_ASSERT(extensions_info, "Should not happen.");

  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: The graph-related information of the extension remains
  // unchanged during the lifecycle of engine/graph, allowing safe
  // cross-thread access.
  axis_ASSERT(axis_extension_info_check_integrity(self, false),
             "Invalid use of extension_info %p.", self);

  axis_shared_ptr_t *exist_dest = get_extension_info_in_extensions_info(
      extensions_info, axis_string_get_raw_str(&self->loc.app_uri),
      axis_string_get_raw_str(&self->loc.graph_id),
      axis_string_get_raw_str(&self->loc.extension_group_name),
      axis_string_get_raw_str(&self->extension_addon_name),
      axis_string_get_raw_str(&self->loc.extension_name),
      /* should_exist = */ true, err);
  axis_ASSERT(exist_dest, "Should not happen.");

  axis_extension_info_t *exist_extension_info =
      axis_shared_ptr_get_data(exist_dest);
  axis_ASSERT(exist_extension_info &&
                 axis_extension_info_check_integrity(exist_extension_info, true),
             "Should not happen.");

  if (!copy_msg_dest(&exist_extension_info->msg_dest_info.cmd,
                     &self->msg_dest_info.cmd, extensions_info, err)) {
    return NULL;
  }

  if (!copy_msg_dest(&exist_extension_info->msg_dest_info.data,
                     &self->msg_dest_info.data, extensions_info, err)) {
    return NULL;
  }

  if (!copy_msg_dest(&exist_extension_info->msg_dest_info.audio_frame,
                     &self->msg_dest_info.audio_frame, extensions_info, err)) {
    return NULL;
  }

  if (!copy_msg_dest(&exist_extension_info->msg_dest_info.video_frame,
                     &self->msg_dest_info.video_frame, extensions_info, err)) {
    return NULL;
  }

  if (!copy_msg_dest(&exist_extension_info->msg_dest_info.interface,
                     &self->msg_dest_info.interface, extensions_info, err)) {
    return NULL;
  }

  return exist_dest;
}

bool axis_extensions_info_clone(axis_list_t *from, axis_list_t *to,
                               axis_error_t *err) {
  axis_ASSERT(from && to, "Should not happen.");

  // `axis_extension_info_clone_except_dest()` will call
  // `get_extension_info_in_extensions_info()`. In
  // `get_extension_info_in_extensions_info()`, we need to determine if
  // `extension_info` exists in `extensions_info`. Therefore, we should first
  // clone the `nodes` and then proceed to clone the `connections`.
  axis_list_foreach (from, iter) {
    axis_extension_info_t *extension_info =
        axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(iter.node));
    if (!axis_extension_info_clone_except_dest(extension_info, to, err)) {
      return false;
    }
  }

  axis_list_foreach (from, iter) {
    axis_extension_info_t *extension_info =
        axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(iter.node));
    if (!axis_extension_info_clone_dest(extension_info, to, err)) {
      return false;
    }
  }

  return true;
}

bool axis_extension_info_check_integrity(axis_extension_info_t *self,
                                        bool check_thread) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_EXTENSION_INFO_SIGNATURE) {
    return false;
  }

  if (check_thread &&
      !axis_sanitizer_thread_check_do_check(&self->thread_check)) {
    return false;
  }

  return true;
}

void axis_extension_info_translate_localhost_to_app_uri(
    axis_extension_info_t *self, const char *uri) {
  axis_ASSERT(self && axis_extension_info_check_integrity(self, true) && uri,
             "Should not happen.");

  if (axis_string_is_equal_c_str(&self->loc.app_uri, axis_STR_LOCALHOST) ||
      axis_string_is_empty(&self->loc.app_uri)) {
    axis_string_init_from_c_str(&self->loc.app_uri, uri, strlen(uri));
  }
}

axis_extension_info_t *axis_extension_info_from_smart_ptr(
    axis_smart_ptr_t *extension_info_smart_ptr) {
  axis_ASSERT(extension_info_smart_ptr, "Invalid argument.");
  return axis_smart_ptr_get_data(extension_info_smart_ptr);
}

static void axis_extension_info_fill_app_uri(axis_extension_info_t *self,
                                            const char *app_uri) {
  axis_ASSERT(self && axis_extension_info_check_integrity(self, true),
             "Invalid argument.");
  axis_ASSERT(app_uri, "Should not happen.");
  axis_ASSERT(!axis_loc_is_empty(&self->loc), "Should not happen.");

  // Fill the app uri of the extension_info if it is empty.
  if (axis_string_is_empty(&self->loc.app_uri)) {
    axis_string_set_formatted(&self->loc.app_uri, "%s", app_uri);
  }

  // Fill the app uri of each item in the msg_conversions_list if it is empty.
  axis_list_foreach (&self->msg_conversion_contexts, iter) {
    axis_msg_conversion_context_t *conversion_iter =
        axis_ptr_listnode_get(iter.node);
    axis_ASSERT(conversion_iter &&
                   axis_msg_conversion_context_check_integrity(conversion_iter),
               "Should not happen.");

    if (axis_string_is_empty(&conversion_iter->src_loc.app_uri)) {
      axis_string_set_formatted(&conversion_iter->src_loc.app_uri, "%s",
                               app_uri);
    }
  }
}

// Fill the app uri of each extension_info in the extensions_info.
void axis_extensions_info_fill_app_uri(axis_list_t *extensions_info,
                                      const char *app_uri) {
  axis_list_foreach (extensions_info, iter) {
    axis_extension_info_t *extension_info =
        axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(iter.node));
    axis_ASSERT(extension_info &&
                   axis_extension_info_check_integrity(extension_info, true),
               "Invalid argument.");

    axis_extension_info_fill_app_uri(extension_info, app_uri);
  }
}

static void axis_extension_info_fill_loc_info(axis_extension_info_t *self,
                                             const char *app_uri,
                                             const char *graph_id) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: The graph-related information of the extension remains
  // unchanged during the lifecycle of engine/graph, allowing safe
  // cross-thread access.
  axis_ASSERT(axis_extension_info_check_integrity(self, false),
             "Invalid use of extension_info %p.", self);

  if (axis_string_is_empty(&self->loc.graph_id)) {
    axis_string_set_formatted(&self->loc.graph_id, "%s", graph_id);
  }

  if (axis_string_is_empty(&self->loc.app_uri) ||
      axis_string_is_equal_c_str(&self->loc.app_uri, axis_STR_LOCALHOST)) {
    axis_string_set_formatted(&self->loc.app_uri, app_uri);
  }

  // Fill the app uri of each item in the msg_conversions_list if it is empty.
  axis_list_foreach (&self->msg_conversion_contexts, iter) {
    axis_msg_conversion_context_t *conversion_iter =
        axis_ptr_listnode_get(iter.node);
    axis_ASSERT(conversion_iter &&
                   axis_msg_conversion_context_check_integrity(conversion_iter),
               "Should not happen.");

    if (axis_string_is_empty(&conversion_iter->src_loc.graph_id)) {
      axis_string_set_formatted(&conversion_iter->src_loc.graph_id, "%s",
                               graph_id);
    }

    if (axis_string_is_empty(&self->loc.app_uri) ||
        axis_string_is_equal_c_str(&self->loc.app_uri, axis_STR_LOCALHOST)) {
      axis_string_set_formatted(&self->loc.app_uri, app_uri);
    }
  }
}

void axis_extensions_info_fill_loc_info(axis_list_t *extensions_info,
                                       const char *app_uri,
                                       const char *graph_id) {
  axis_list_foreach (extensions_info, iter) {
    axis_extension_info_t *extension_info =
        axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(iter.node));
    // axis_NOLINTNEXTLINE(thread-check)
    // thread-check: The graph-related information of the extension remains
    // unchanged during the lifecycle of engine/graph, allowing safe
    // cross-thread access.
    axis_ASSERT(extension_info &&
                   axis_extension_info_check_integrity(extension_info, false),
               "Invalid argument.");

    axis_extension_info_fill_loc_info(extension_info, app_uri, graph_id);
  }

  // Check if the extension_info in the `dest` section is correct.
  axis_list_foreach (extensions_info, iter) {
    axis_extension_info_t *extension_info =
        axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(iter.node));

    axis_list_foreach (&extension_info->msg_dest_info.cmd, iter_cmd) {
      axis_msg_dest_info_t *dest_info =
          axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(iter_cmd.node));
      axis_list_foreach (&dest_info->dest, dest_iter) {
        axis_extension_info_t *dest_extension_info =
            axis_smart_ptr_get_data(axis_smart_ptr_listnode_get(dest_iter.node));
        if (axis_string_is_empty(&dest_extension_info->loc.app_uri)) {
          axis_ASSERT(0, "extension_info->loc.app_uri should not be empty.");
          return;
        }

        if (axis_string_is_equal_c_str(&dest_extension_info->loc.app_uri,
                                      axis_STR_LOCALHOST)) {
          if (!axis_string_is_equal_c_str(&dest_extension_info->loc.app_uri,
                                         app_uri)) {
            axis_ASSERT(0,
                       "extension_info->loc.app_uri should not be localhost.");
            return;
          }
        }

        if (axis_string_is_empty(&dest_extension_info->loc.graph_id)) {
          axis_ASSERT(0, "extension_info->loc.graph_id should not be empty.");
          return;
        }
      }
    }
  }
}
