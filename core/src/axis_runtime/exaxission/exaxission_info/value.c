//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension/extension_info/value.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/extension/extension_info/extension_info.h"
#include "include_internal/axis_runtime/extension/extension_info/json.h"
#include "include_internal/axis_runtime/extension/msg_dest_info/msg_dest_info.h"
#include "include_internal/axis_runtime/extension/msg_dest_info/value.h"
#include "include_internal/axis_runtime/msg_conversion/msg_conversion_context.h"
#include "axis_runtime/common/errno.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_merge.h"
#include "axis_utils/value/value_object.h"

// Parse the following snippet.
//
// ------------------------
// "name": "...",
// "dest": [{
//   "app": "...",
//   "extension_group": "...",
//   "extension": "...",
//   "msg_conversion": {
//   }
// }]
// ------------------------
static bool parse_msg_dest_value(axis_value_t *value,
                                 axis_list_t *extensions_info,
                                 axis_list_t *static_dests,
                                 axis_extension_info_t *src_extension_info,
                                 axis_error_t *err) {
  axis_ASSERT(value && axis_value_is_array(value) && extensions_info,
             "Should not happen.");
  axis_ASSERT(static_dests, "Invalid argument.");
  axis_ASSERT(src_extension_info,
             "src_extension must be specified in this case.");

  axis_value_array_foreach(value, iter) {
    axis_value_t *item_value = axis_ptr_listnode_get(iter.node);
    if (!axis_value_is_object(item_value)) {
      return false;
    }

    axis_shared_ptr_t *msg_dest = axis_msg_dest_info_from_value(
        item_value, extensions_info, src_extension_info, err);
    if (!msg_dest) {
      return false;
    }

    axis_list_push_smart_ptr_back(static_dests, msg_dest);
    axis_shared_ptr_destroy(msg_dest);
  }

  return true;
}

static bool parse_msg_conversions_value(
    axis_value_t *value, axis_extension_info_t *src_extension_info,
    const char *msg_name, axis_list_t *msg_conversions, axis_error_t *err) {
  axis_ASSERT(src_extension_info, "Should not happen.");
  axis_ASSERT(msg_name, "Should not happen.");

  axis_msg_conversion_context_t *msg_conversion =
      axis_msg_conversion_context_from_value(value, src_extension_info, msg_name,
                                            err);
  axis_ASSERT(msg_conversion &&
                 axis_msg_conversion_context_check_integrity(msg_conversion),
             "Should not happen.");
  if (!msg_conversion) {
    return false;
  }

  return axis_msg_conversion_context_merge(msg_conversions, msg_conversion, err);
}

axis_shared_ptr_t *axis_extension_info_node_from_value(
    axis_value_t *value, axis_list_t *extensions_info, axis_error_t *err) {
  axis_ASSERT(value && extensions_info, "Invalid argument.");

  const char *app_uri = axis_value_object_peek_string(value, axis_STR_APP);
  const char *graph_id = axis_value_object_peek_string(value, axis_STR_GRAPH);
  const char *extension_group_name =
      axis_value_object_peek_string(value, axis_STR_EXTENSION_GROUP);
  const char *addon_name = axis_value_object_peek_string(value, axis_STR_ADDON);
  const char *instance_name = axis_value_object_peek_string(value, axis_STR_NAME);

  axis_shared_ptr_t *self = get_extension_info_in_extensions_info(
      extensions_info, app_uri, graph_id, extension_group_name, addon_name,
      instance_name, false, err);
  if (!self) {
    return NULL;
  }

  axis_extension_info_t *extension_info = axis_shared_ptr_get_data(self);
  axis_ASSERT(axis_extension_info_check_integrity(extension_info, true),
             "Should not happen.");

  // Parse 'prop'
  axis_value_t *props_value = axis_value_object_peek(value, axis_STR_PROPERTY);
  if (props_value) {
    if (!axis_value_is_object(props_value)) {
      if (err) {
        axis_error_set(err, axis_ERRNO_GENERIC,
                      "The `property` in graph node should be an object.");
      } else {
        axis_ASSERT(0, "The `property` in graph node should be an object.");
      }

      return NULL;
    }

    axis_value_object_merge_with_clone(extension_info->property, props_value);
  }

  return self;
}

axis_shared_ptr_t *axis_extension_info_parse_connection_src_part_from_value(
    axis_value_t *value, axis_list_t *extensions_info, axis_error_t *err) {
  axis_ASSERT(value && extensions_info, "Invalid argument.");

  const char *app_uri = axis_value_object_peek_string(value, axis_STR_APP);
  const char *graph_id = axis_value_object_peek_string(value, axis_STR_GRAPH);

  const char *extension_name =
      axis_value_object_peek_string(value, axis_STR_EXTENSION);
  if (!extension_name || axis_c_string_is_empty(extension_name)) {
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_GRAPH,
                    "The extension in connection is required.");
    } else {
      axis_ASSERT(0, "The extension in connection is required.");
    }

    return NULL;
  }

  axis_shared_ptr_t *self = get_extension_info_in_extensions_info(
      extensions_info, app_uri, graph_id, NULL, NULL, extension_name, true,
      err);
  if (!self) {
    return NULL;
  }

  axis_extension_info_t *extension_info = axis_shared_ptr_get_data(self);
  axis_ASSERT(axis_extension_info_check_integrity(extension_info, true),
             "Should not happen.");

  // Parse 'cmd'
  axis_value_t *cmds_value = axis_value_object_peek(value, axis_STR_CMD);
  if (cmds_value) {
    if (!parse_msg_dest_value(cmds_value, extensions_info,
                              &extension_info->msg_dest_info.cmd,
                              extension_info, err)) {
      return NULL;
    }
  }

  // Parse 'data'
  axis_value_t *data_value = axis_value_object_peek(value, axis_STR_DATA);
  if (data_value) {
    if (!parse_msg_dest_value(data_value, extensions_info,
                              &extension_info->msg_dest_info.data,
                              extension_info, err)) {
      return NULL;
    }
  }

  // Parse 'video_frame'
  axis_value_t *video_frame_value =
      axis_value_object_peek(value, axis_STR_VIDEO_FRAME);
  if (video_frame_value) {
    if (!parse_msg_dest_value(video_frame_value, extensions_info,
                              &extension_info->msg_dest_info.video_frame,
                              extension_info, err)) {
      return NULL;
    }
  }

  // Parse 'audio_frame'
  axis_value_t *audio_frame_value =
      axis_value_object_peek(value, axis_STR_AUDIO_FRAME);
  if (audio_frame_value) {
    if (!parse_msg_dest_value(audio_frame_value, extensions_info,
                              &extension_info->msg_dest_info.audio_frame,
                              extension_info, err)) {
      return NULL;
    }
  }

  // Parse 'interface'.
  axis_value_t *interface_value =
      axis_value_object_peek(value, axis_STR_INTERFACE);
  if (interface_value) {
    if (!parse_msg_dest_value(interface_value, extensions_info,
                              &extension_info->msg_dest_info.interface,
                              extension_info, err)) {
      return NULL;
    }
  }

  return self;
}

axis_shared_ptr_t *axis_extension_info_parse_connection_dest_part_from_value(
    axis_value_t *value, axis_list_t *extensions_info,
    axis_extension_info_t *src_extension_info, const char *origin_cmd_name,
    axis_error_t *err) {
  axis_ASSERT(value && extensions_info, "Should not happen.");

  const char *app_uri = axis_value_object_peek_string(value, axis_STR_APP);
  const char *graph_id = axis_value_object_peek_string(value, axis_STR_GRAPH);

  const char *extension_name =
      axis_value_object_peek_string(value, axis_STR_EXTENSION);
  if (!extension_name || axis_c_string_is_empty(extension_name)) {
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_GRAPH,
                    "The extension in connection is required.");
    } else {
      axis_ASSERT(0, "The extension in connection is required.");
    }
    return NULL;
  }

  axis_shared_ptr_t *self = get_extension_info_in_extensions_info(
      extensions_info, app_uri, graph_id, NULL, NULL, extension_name, true,
      err);
  if (!self) {
    return NULL;
  }

  axis_extension_info_t *extension_info = axis_shared_ptr_get_data(self);
  axis_ASSERT(axis_extension_info_check_integrity(extension_info, true),
             "Should not happen.");

  // Parse 'msg_conversions'
  axis_value_t *msg_conversions_value =
      axis_value_object_peek(value, axis_STR_MSG_CONVERSION);
  if (msg_conversions_value) {
    if (!parse_msg_conversions_value(
            msg_conversions_value, src_extension_info, origin_cmd_name,
            &extension_info->msg_conversion_contexts, err)) {
      axis_ASSERT(0, "Should not happen.");
      return NULL;
    }
  }

  return self;
}

axis_value_t *axis_extension_info_node_to_value(axis_extension_info_t *self,
                                              axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: The graph-related information of the extension remains
  // unchanged during the lifecycle of engine/graph, allowing safe cross-thread
  // access.
  axis_ASSERT(axis_extension_info_check_integrity(self, false),
             "Should not happen.");

  // Convert the extension info into axis_value_t, which is an object-type value
  // and the snippet is as follows:
  //
  // ------------------------
  // {
  //   "type": "extension",
  //   "name": "...",
  //   "addon": "...",
  //   "extension_group": "...",
  //   "graph": "...",
  //   "app": "...",
  //   "property": {
  //     ...
  //   }
  // }
  // ------------------------
  axis_list_t kv_list = axis_LIST_INIT_VAL;

  axis_value_t *type_value = axis_value_create_string(axis_STR_EXTENSION);
  axis_ASSERT(type_value, "Should not happen.");
  axis_list_push_ptr_back(&kv_list,
                         axis_value_kv_create(axis_STR_TYPE, type_value),
                         (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);

  axis_value_t *name_value = axis_value_create_string(
      axis_string_get_raw_str(&self->loc.extension_name));
  axis_ASSERT(name_value, "Should not happen.");
  axis_list_push_ptr_back(&kv_list,
                         axis_value_kv_create(axis_STR_NAME, name_value),
                         (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);

  axis_value_t *addon_value = axis_value_create_string(
      axis_string_get_raw_str(&self->extension_addon_name));
  axis_ASSERT(addon_value, "Should not happen.");
  axis_list_push_ptr_back(&kv_list,
                         axis_value_kv_create(axis_STR_ADDON, addon_value),
                         (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);

  axis_value_t *extension_group_name_value = axis_value_create_string(
      axis_string_get_raw_str(&self->loc.extension_group_name));
  axis_ASSERT(extension_group_name_value, "Should not happen.");
  axis_list_push_ptr_back(
      &kv_list,
      axis_value_kv_create(axis_STR_EXTENSION_GROUP, extension_group_name_value),
      (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);

  axis_value_t *graph_id_value =
      axis_value_create_string(axis_string_get_raw_str(&self->loc.graph_id));
  axis_ASSERT(graph_id_value, "Should not happen.");
  axis_list_push_ptr_back(&kv_list,
                         axis_value_kv_create(axis_STR_GRAPH, graph_id_value),
                         (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);

  axis_value_t *app_uri_value =
      axis_value_create_string(axis_string_get_raw_str(&self->loc.app_uri));
  axis_ASSERT(app_uri_value, "Should not happen.");
  axis_list_push_ptr_back(&kv_list,
                         axis_value_kv_create(axis_STR_APP, app_uri_value),
                         (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);

  if (self->property) {
    axis_list_push_ptr_back(
        &kv_list, axis_value_kv_create(axis_STR_PROPERTY, self->property),
        (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy_key_only);
  }

  axis_value_t *result = axis_value_create_object_with_move(&kv_list);
  axis_ASSERT(result, "Should not happen.");

  axis_list_clear(&kv_list);

  return result;
}

static axis_value_t *pack_msg_dest(axis_extension_info_t *self,
                                  axis_list_t *msg_dests, axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: The graph-related information of the extension remains
  // unchanged during the lifecycle of engine/graph, allowing safe
  // cross-thread access.
  axis_ASSERT(axis_extension_info_check_integrity(self, false),
             "Should not happen.");

  axis_list_t dest_list = axis_LIST_INIT_VAL;

  axis_list_foreach (msg_dests, iter) {
    axis_msg_dest_info_t *msg_dest =
        axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(iter.node));

    axis_value_t *msg_dest_value =
        axis_msg_dest_info_to_value(msg_dest, self, err);
    if (!msg_dest_value) {
      return NULL;
    }

    axis_list_push_ptr_back(&dest_list, msg_dest_value,
                           (axis_ptr_listnode_destroy_func_t)axis_value_destroy);
  }

  axis_value_t *result = axis_value_create_array_with_move(&dest_list);
  axis_ASSERT(result, "Should not happen.");

  axis_list_clear(&dest_list);

  return result;
}

axis_value_t *axis_extension_info_connection_to_value(axis_extension_info_t *self,
                                                    axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: The graph-related information of the extension remains
  // unchanged during the lifecycle of engine/graph, allowing safe
  // cross-thread access.
  axis_ASSERT(axis_extension_info_check_integrity(self, false),
             "Should not happen.");

  if (axis_list_is_empty(&self->msg_dest_info.cmd) &&
      axis_list_is_empty(&self->msg_dest_info.data) &&
      axis_list_is_empty(&self->msg_dest_info.video_frame) &&
      axis_list_is_empty(&self->msg_dest_info.audio_frame) &&
      axis_list_is_empty(&self->msg_dest_info.interface) &&
      axis_list_is_empty(&self->msg_conversion_contexts)) {
    return NULL;
  }

  // Convert the extension info connections into axis_value_t, which is an
  // object-type value and the snippet is as follows:
  //
  // ------------------------
  // {
  //   "app": "...",
  //   "graph": "...",
  //   "extension_group": "...",
  //   "extension": "...",
  //   "cmd": [
  //     ...
  //   ],
  //   "data": [
  //     ...
  //   ],
  //   "video_frame": [
  //     ...
  //   ],
  //   "audio_frame": [
  //     ...
  //   ],
  //   "interface": [
  //     ...
  //   ]
  // }
  axis_list_t kv_list = axis_LIST_INIT_VAL;

  axis_value_t *app_uri_value =
      axis_value_create_string(axis_string_get_raw_str(&self->loc.app_uri));
  axis_ASSERT(app_uri_value, "Should not happen.");
  axis_list_push_ptr_back(&kv_list,
                         axis_value_kv_create(axis_STR_APP, app_uri_value),
                         (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);

  axis_value_t *graph_id_value =
      axis_value_create_string(axis_string_get_raw_str(&self->loc.graph_id));
  axis_ASSERT(graph_id_value, "Should not happen.");
  axis_list_push_ptr_back(&kv_list,
                         axis_value_kv_create(axis_STR_GRAPH, graph_id_value),
                         (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);

  axis_value_t *extension_group_value = axis_value_create_string(
      axis_string_get_raw_str(&self->loc.extension_group_name));
  axis_ASSERT(extension_group_value, "Should not happen.");
  axis_list_push_ptr_back(
      &kv_list,
      axis_value_kv_create(axis_STR_EXTENSION_GROUP, extension_group_value),
      (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);

  axis_value_t *extension_value = axis_value_create_string(
      axis_string_get_raw_str(&self->loc.extension_name));
  axis_ASSERT(extension_value, "Should not happen.");
  axis_list_push_ptr_back(
      &kv_list, axis_value_kv_create(axis_STR_EXTENSION, extension_value),
      (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);

  // Parse 'cmd'
  if (!axis_list_is_empty(&self->msg_dest_info.cmd)) {
    axis_value_t *cmd_dest_value =
        pack_msg_dest(self, &self->msg_dest_info.cmd, err);
    if (!cmd_dest_value) {
      return NULL;
    }

    axis_list_push_ptr_back(
        &kv_list, axis_value_kv_create(axis_STR_CMD, cmd_dest_value),
        (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);
  }

  // Parse 'data'
  if (!axis_list_is_empty(&self->msg_dest_info.data)) {
    axis_value_t *data_dest_value =
        pack_msg_dest(self, &self->msg_dest_info.data, err);
    if (!data_dest_value) {
      return NULL;
    }

    axis_list_push_ptr_back(
        &kv_list, axis_value_kv_create(axis_STR_DATA, data_dest_value),
        (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);
  }

  // Parse 'video_frame'
  if (!axis_list_is_empty(&self->msg_dest_info.video_frame)) {
    axis_value_t *video_frame_dest_value =
        pack_msg_dest(self, &self->msg_dest_info.video_frame, err);
    if (!video_frame_dest_value) {
      return NULL;
    }

    axis_list_push_ptr_back(
        &kv_list,
        axis_value_kv_create(axis_STR_VIDEO_FRAME, video_frame_dest_value),
        (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);
  }

  // Parse 'audio_frame'
  if (!axis_list_is_empty(&self->msg_dest_info.audio_frame)) {
    axis_value_t *audio_frame_dest_value =
        pack_msg_dest(self, &self->msg_dest_info.audio_frame, err);
    if (!audio_frame_dest_value) {
      return NULL;
    }

    axis_list_push_ptr_back(
        &kv_list,
        axis_value_kv_create(axis_STR_AUDIO_FRAME, audio_frame_dest_value),
        (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);
  }

  // Parse 'interface'
  if (!axis_list_is_empty(&self->msg_dest_info.interface)) {
    axis_value_t *interface_dest_value =
        pack_msg_dest(self, &self->msg_dest_info.interface, err);
    if (!interface_dest_value) {
      return NULL;
    }

    axis_list_push_ptr_back(
        &kv_list, axis_value_kv_create(axis_STR_INTERFACE, interface_dest_value),
        (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);
  }

  axis_value_t *result = axis_value_create_object_with_move(&kv_list);
  axis_ASSERT(result, "Should not happen.");

  axis_list_clear(&kv_list);

  return result;
}
