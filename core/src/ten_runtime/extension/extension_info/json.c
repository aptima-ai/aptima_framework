//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/extension/extension_info/json.h"

#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/extension/extension_info/extension_info.h"
#include "include_internal/ten_runtime/extension/msg_dest_info/json.h"
#include "include_internal/ten_runtime/extension/msg_dest_info/msg_dest_info.h"
#include "include_internal/ten_runtime/msg_conversion/msg_conversion_context.h"
#include "ten_utils/container/list.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"

static ten_json_t *pack_msg_dest(ten_extension_info_t *self,
                                 ten_list_t *msg_dests, ten_error_t *err) {
  ten_json_t *msg_json = ten_json_create_array();
  TEN_ASSERT(msg_json, "Should not happen.");

  ten_list_foreach (msg_dests, iter) {
    ten_msg_dest_info_t *msg_dest =
        ten_shared_ptr_get_data(ten_smart_ptr_listnode_get(iter.node));

    ten_json_t *msg_dest_json = ten_msg_dest_info_to_json(msg_dest, self, err);
    if (!msg_dest_json) {
      ten_json_destroy(msg_json);
      return NULL;
    }

    ten_json_array_append_new(msg_json, msg_dest_json);
  }

  return msg_json;
}

ten_json_t *ten_extension_info_node_to_json(ten_extension_info_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: The graph-related information of the extension remains
  // unchanged during the lifecycle of engine/graph, allowing safe
  // cross-thread access.
  TEN_ASSERT(ten_extension_info_check_integrity(self, false),
             "Should not happen.");

  ten_json_t *info = ten_json_create_object();
  TEN_ASSERT(info, "Should not happen.");

  ten_json_t *type = ten_json_create_string(TEN_STR_EXTENSION);
  TEN_ASSERT(type, "Should not happen.");
  ten_json_object_set_new(info, TEN_STR_TYPE, type);

  ten_json_t *name =
      ten_json_create_string(ten_string_get_raw_str(&self->loc.extension_name));
  TEN_ASSERT(name, "Should not happen.");
  ten_json_object_set_new(info, TEN_STR_NAME, name);

  ten_json_t *addon = ten_json_create_string(
      ten_string_get_raw_str(&self->extension_addon_name));
  TEN_ASSERT(addon, "Should not happen.");
  ten_json_object_set_new(info, TEN_STR_ADDON, addon);

  ten_json_t *extension_group_name = ten_json_create_string(
      ten_string_get_raw_str(&self->loc.extension_group_name));
  TEN_ASSERT(extension_group_name, "Should not happen.");
  ten_json_object_set_new(info, TEN_STR_EXTENSION_GROUP, extension_group_name);

  ten_json_t *graph_id =
      ten_json_create_string(ten_string_get_raw_str(&self->loc.graph_id));
  TEN_ASSERT(graph_id, "Should not happen.");
  ten_json_object_set_new(info, TEN_STR_GRAPH, graph_id);

  ten_json_t *app_uri =
      ten_json_create_string(ten_string_get_raw_str(&self->loc.app_uri));
  TEN_ASSERT(app_uri, "Should not happen.");
  ten_json_object_set_new(info, TEN_STR_APP, app_uri);

  if (self->property) {
    ten_json_object_set_new(info, TEN_STR_PROPERTY,
                            ten_value_to_json(self->property));
  }

  return info;
}

bool ten_extension_info_connections_to_json(ten_extension_info_t *self,
                                            ten_json_t **json,
                                            ten_error_t *err) {
  TEN_ASSERT(self, "Invalid argument.");
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: The graph-related information of the extension remains
  // unchanged during the lifecycle of engine/graph, allowing safe
  // cross-thread access.
  TEN_ASSERT(ten_extension_info_check_integrity(self, false),
             "Should not happen.");
  TEN_ASSERT(json, "Invalid argument.");

  if (ten_list_is_empty(&self->msg_dest_info.cmd) &&
      ten_list_is_empty(&self->msg_dest_info.data) &&
      ten_list_is_empty(&self->msg_dest_info.video_frame) &&
      ten_list_is_empty(&self->msg_dest_info.audio_frame) &&
      ten_list_is_empty(&self->msg_dest_info.interface) &&
      ten_list_is_empty(&self->msg_conversion_contexts)) {
    *json = NULL;
    return true;
  }

  ten_json_t *info = ten_json_create_object();
  TEN_ASSERT(info, "Should not happen.");

  ten_json_t *app_uri =
      ten_json_create_string(ten_string_get_raw_str(&self->loc.app_uri));
  TEN_ASSERT(app_uri, "Should not happen.");
  ten_json_object_set_new(info, TEN_STR_APP, app_uri);

  ten_json_t *graph_id =
      ten_json_create_string(ten_string_get_raw_str(&self->loc.graph_id));
  TEN_ASSERT(graph_id, "Should not happen.");
  ten_json_object_set_new(info, TEN_STR_GRAPH, graph_id);

  ten_json_t *extension_group_json = ten_json_create_string(
      ten_string_get_raw_str(&self->loc.extension_group_name));
  TEN_ASSERT(extension_group_json, "Should not happen.");
  ten_json_object_set_new(info, TEN_STR_EXTENSION_GROUP, extension_group_json);

  ten_json_t *extension_json =
      ten_json_create_string(ten_string_get_raw_str(&self->loc.extension_name));
  TEN_ASSERT(extension_json, "Should not happen.");
  ten_json_object_set_new(info, TEN_STR_EXTENSION, extension_json);

  if (!ten_list_is_empty(&self->msg_dest_info.cmd)) {
    ten_json_t *cmd_dest_json =
        pack_msg_dest(self, &self->msg_dest_info.cmd, err);
    if (!cmd_dest_json) {
      ten_json_destroy(info);
      return false;
    }

    ten_json_object_set_new(info, TEN_STR_CMD, cmd_dest_json);
  }

  if (!ten_list_is_empty(&self->msg_dest_info.data)) {
    ten_json_t *data_dest_json =
        pack_msg_dest(self, &self->msg_dest_info.data, err);
    if (!data_dest_json) {
      ten_json_destroy(info);
      return false;
    }

    ten_json_object_set_new(info, TEN_STR_DATA, data_dest_json);
  }

  if (!ten_list_is_empty(&self->msg_dest_info.video_frame)) {
    ten_json_t *video_frame_dest_json =
        pack_msg_dest(self, &self->msg_dest_info.video_frame, err);
    if (!video_frame_dest_json) {
      ten_json_destroy(info);
      return false;
    }

    ten_json_object_set_new(info, TEN_STR_VIDEO_FRAME, video_frame_dest_json);
  }

  if (!ten_list_is_empty(&self->msg_dest_info.audio_frame)) {
    ten_json_t *audio_frame_dest_json =
        pack_msg_dest(self, &self->msg_dest_info.audio_frame, err);
    if (!audio_frame_dest_json) {
      ten_json_destroy(info);
      return false;
    }

    ten_json_object_set_new(info, TEN_STR_AUDIO_FRAME, audio_frame_dest_json);
  }

  if (!ten_list_is_empty(&self->msg_dest_info.interface)) {
    ten_json_t *interface_dest_json =
        pack_msg_dest(self, &self->msg_dest_info.interface, err);
    if (!interface_dest_json) {
      ten_json_destroy(info);
      return false;
    }

    ten_json_object_set_new(info, TEN_STR_INTERFACE, interface_dest_json);
  }

  *json = info;
  return true;
}
