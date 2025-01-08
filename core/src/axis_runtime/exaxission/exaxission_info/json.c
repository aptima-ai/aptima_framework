//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension/extension_info/json.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/extension/extension_info/extension_info.h"
#include "include_internal/axis_runtime/extension/msg_dest_info/json.h"
#include "include_internal/axis_runtime/extension/msg_dest_info/msg_dest_info.h"
#include "include_internal/axis_runtime/msg_conversion/msg_conversion_context.h"
#include "axis_utils/container/list.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"

static axis_json_t *pack_msg_dest(axis_extension_info_t *self,
                                 axis_list_t *msg_dests, axis_error_t *err) {
  axis_json_t *msg_json = axis_json_create_array();
  axis_ASSERT(msg_json, "Should not happen.");

  axis_list_foreach (msg_dests, iter) {
    axis_msg_dest_info_t *msg_dest =
        axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(iter.node));

    axis_json_t *msg_dest_json = axis_msg_dest_info_to_json(msg_dest, self, err);
    if (!msg_dest_json) {
      axis_json_destroy(msg_json);
      return NULL;
    }

    axis_json_array_append_new(msg_json, msg_dest_json);
  }

  return msg_json;
}

axis_json_t *axis_extension_info_node_to_json(axis_extension_info_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: The graph-related information of the extension remains
  // unchanged during the lifecycle of engine/graph, allowing safe
  // cross-thread access.
  axis_ASSERT(axis_extension_info_check_integrity(self, false),
             "Should not happen.");

  axis_json_t *info = axis_json_create_object();
  axis_ASSERT(info, "Should not happen.");

  axis_json_t *type = axis_json_create_string(axis_STR_EXTENSION);
  axis_ASSERT(type, "Should not happen.");
  axis_json_object_set_new(info, axis_STR_TYPE, type);

  axis_json_t *name =
      axis_json_create_string(axis_string_get_raw_str(&self->loc.extension_name));
  axis_ASSERT(name, "Should not happen.");
  axis_json_object_set_new(info, axis_STR_NAME, name);

  axis_json_t *addon = axis_json_create_string(
      axis_string_get_raw_str(&self->extension_addon_name));
  axis_ASSERT(addon, "Should not happen.");
  axis_json_object_set_new(info, axis_STR_ADDON, addon);

  axis_json_t *extension_group_name = axis_json_create_string(
      axis_string_get_raw_str(&self->loc.extension_group_name));
  axis_ASSERT(extension_group_name, "Should not happen.");
  axis_json_object_set_new(info, axis_STR_EXTENSION_GROUP, extension_group_name);

  axis_json_t *graph_id =
      axis_json_create_string(axis_string_get_raw_str(&self->loc.graph_id));
  axis_ASSERT(graph_id, "Should not happen.");
  axis_json_object_set_new(info, axis_STR_GRAPH, graph_id);

  axis_json_t *app_uri =
      axis_json_create_string(axis_string_get_raw_str(&self->loc.app_uri));
  axis_ASSERT(app_uri, "Should not happen.");
  axis_json_object_set_new(info, axis_STR_APP, app_uri);

  if (self->property) {
    axis_json_object_set_new(info, axis_STR_PROPERTY,
                            axis_value_to_json(self->property));
  }

  return info;
}

bool axis_extension_info_connections_to_json(axis_extension_info_t *self,
                                            axis_json_t **json,
                                            axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: The graph-related information of the extension remains
  // unchanged during the lifecycle of engine/graph, allowing safe
  // cross-thread access.
  axis_ASSERT(axis_extension_info_check_integrity(self, false),
             "Should not happen.");
  axis_ASSERT(json, "Invalid argument.");

  if (axis_list_is_empty(&self->msg_dest_info.cmd) &&
      axis_list_is_empty(&self->msg_dest_info.data) &&
      axis_list_is_empty(&self->msg_dest_info.video_frame) &&
      axis_list_is_empty(&self->msg_dest_info.audio_frame) &&
      axis_list_is_empty(&self->msg_dest_info.interface) &&
      axis_list_is_empty(&self->msg_conversion_contexts)) {
    *json = NULL;
    return true;
  }

  axis_json_t *info = axis_json_create_object();
  axis_ASSERT(info, "Should not happen.");

  axis_json_t *app_uri =
      axis_json_create_string(axis_string_get_raw_str(&self->loc.app_uri));
  axis_ASSERT(app_uri, "Should not happen.");
  axis_json_object_set_new(info, axis_STR_APP, app_uri);

  axis_json_t *graph_id =
      axis_json_create_string(axis_string_get_raw_str(&self->loc.graph_id));
  axis_ASSERT(graph_id, "Should not happen.");
  axis_json_object_set_new(info, axis_STR_GRAPH, graph_id);

  axis_json_t *extension_group_json = axis_json_create_string(
      axis_string_get_raw_str(&self->loc.extension_group_name));
  axis_ASSERT(extension_group_json, "Should not happen.");
  axis_json_object_set_new(info, axis_STR_EXTENSION_GROUP, extension_group_json);

  axis_json_t *extension_json =
      axis_json_create_string(axis_string_get_raw_str(&self->loc.extension_name));
  axis_ASSERT(extension_json, "Should not happen.");
  axis_json_object_set_new(info, axis_STR_EXTENSION, extension_json);

  if (!axis_list_is_empty(&self->msg_dest_info.cmd)) {
    axis_json_t *cmd_dest_json =
        pack_msg_dest(self, &self->msg_dest_info.cmd, err);
    if (!cmd_dest_json) {
      axis_json_destroy(info);
      return false;
    }

    axis_json_object_set_new(info, axis_STR_CMD, cmd_dest_json);
  }

  if (!axis_list_is_empty(&self->msg_dest_info.data)) {
    axis_json_t *data_dest_json =
        pack_msg_dest(self, &self->msg_dest_info.data, err);
    if (!data_dest_json) {
      axis_json_destroy(info);
      return false;
    }

    axis_json_object_set_new(info, axis_STR_DATA, data_dest_json);
  }

  if (!axis_list_is_empty(&self->msg_dest_info.video_frame)) {
    axis_json_t *video_frame_dest_json =
        pack_msg_dest(self, &self->msg_dest_info.video_frame, err);
    if (!video_frame_dest_json) {
      axis_json_destroy(info);
      return false;
    }

    axis_json_object_set_new(info, axis_STR_VIDEO_FRAME, video_frame_dest_json);
  }

  if (!axis_list_is_empty(&self->msg_dest_info.audio_frame)) {
    axis_json_t *audio_frame_dest_json =
        pack_msg_dest(self, &self->msg_dest_info.audio_frame, err);
    if (!audio_frame_dest_json) {
      axis_json_destroy(info);
      return false;
    }

    axis_json_object_set_new(info, axis_STR_AUDIO_FRAME, audio_frame_dest_json);
  }

  if (!axis_list_is_empty(&self->msg_dest_info.interface)) {
    axis_json_t *interface_dest_json =
        pack_msg_dest(self, &self->msg_dest_info.interface, err);
    if (!interface_dest_json) {
      axis_json_destroy(info);
      return false;
    }

    axis_json_object_set_new(info, axis_STR_INTERFACE, interface_dest_json);
  }

  *json = info;
  return true;
}
