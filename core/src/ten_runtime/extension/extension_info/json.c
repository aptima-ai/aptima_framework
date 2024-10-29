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

// NOLINTNEXTLINE(misc-no-recursion)
static bool parse_msg_dest_json(ten_json_t *json, ten_list_t *extensions_info,
                                ten_list_t *dests,
                                ten_extension_info_t *src_extension_info) {
  TEN_ASSERT(ten_json_is_array(json) && extensions_info, "Should not happen.");

  size_t i = 0;
  ten_json_t *msg_json = NULL;
  ten_json_array_foreach(json, i, msg_json) {
    TEN_ASSERT(ten_json_is_object(msg_json), "Should not happen.");

    ten_shared_ptr_t *msg_dest = ten_msg_dest_info_from_json(
        msg_json, extensions_info, src_extension_info);
    if (!msg_dest) {
      return false;
    }

    ten_list_push_smart_ptr_back(dests, msg_dest);
    ten_shared_ptr_destroy(msg_dest);
  }

  return true;
}

static bool parse_msg_conversions_json(ten_json_t *msg_conversions_json,
                                       const char *msg_name,
                                       ten_extension_info_t *src_extension_info,
                                       ten_list_t *msg_conversions,
                                       ten_error_t *err) {
  TEN_ASSERT(msg_name, "Should not happen.");

  ten_msg_conversion_context_t *msg_conversion =
      ten_msg_conversion_context_from_json(msg_conversions_json,
                                           src_extension_info, msg_name, err);
  TEN_ASSERT(msg_conversion &&
                 ten_msg_conversion_context_check_integrity(msg_conversion),
             "Should not happen.");

  return ten_msg_conversion_context_merge(msg_conversions, msg_conversion, err);
}

ten_shared_ptr_t *ten_extension_info_nodes_from_json(
    ten_json_t *json, ten_list_t *extensions_info, ten_error_t *err) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");
  TEN_ASSERT(extensions_info && ten_list_check_integrity(extensions_info),
             "Should not happen.");

  const char *app_uri = ten_json_object_peek_string(json, TEN_STR_APP);
  TEN_ASSERT(app_uri, "Should not happen.");
  if (!app_uri) {
    return NULL;
  }

  const char *graph_id = ten_json_object_peek_string(json, TEN_STR_GRAPH);

  const char *extension_group_name =
      ten_json_object_peek_string(json, TEN_STR_EXTENSION_GROUP);
  TEN_ASSERT(extension_group_name, "Should not happen.");
  if (!extension_group_name) {
    return NULL;
  }

  const char *addon_name = ten_json_object_peek_string(json, TEN_STR_ADDON);
  const char *instance_name = ten_json_object_peek_string(json, TEN_STR_NAME);
  TEN_ASSERT(instance_name, "Should not happen.");
  if (!instance_name) {
    return NULL;
  }

  ten_shared_ptr_t *self = get_extension_info_in_extensions_info(
      extensions_info, app_uri, graph_id, extension_group_name, addon_name,
      instance_name, NULL, err);
  if (!self) {
    return NULL;
  }

  ten_extension_info_t *extension_info = ten_shared_ptr_get_data(self);
  TEN_ASSERT(ten_extension_info_check_integrity(extension_info, true),
             "Should not happen.");

  // Parse 'prop'
  ten_json_t *props_json = ten_json_object_peek(json, TEN_STR_PROPERTY);
  if (props_json) {
    if (!ten_json_is_object(props_json)) {
      // Indicates an error.
      TEN_ASSERT(0,
                 "Failed to parse 'prop' in 'start_graph' command, it's not an "
                 "object.");
      return NULL;
    }

    ten_value_object_merge_with_json(extension_info->property, props_json);
  }

  return self;
}

ten_shared_ptr_t *ten_extension_info_parse_connection_src_part_from_json(
    ten_json_t *json, ten_list_t *extensions_info, ten_error_t *err) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");
  TEN_ASSERT(extensions_info && ten_list_check_integrity(extensions_info),
             "Should not happen.");

  const char *app_uri = ten_json_object_peek_string(json, TEN_STR_APP);
  const char *graph_id = ten_json_object_peek_string(json, TEN_STR_GRAPH);

  const char *extension_group_name =
      ten_json_object_peek_string(json, TEN_STR_EXTENSION_GROUP);
  if (extension_group_name == NULL) {
    TEN_ASSERT(0, "Should not happen.");
    return NULL;
  }

  const char *extension_name =
      ten_json_object_peek_string(json, TEN_STR_EXTENSION);
  if (extension_name == NULL) {
    TEN_ASSERT(0, "Should not happen.");
    return NULL;
  }

  ten_shared_ptr_t *self = get_extension_info_in_extensions_info(
      extensions_info, app_uri, graph_id, extension_group_name, NULL,
      extension_name, NULL, err);
  if (!self) {
    return NULL;
  }

  ten_extension_info_t *extension_info = ten_shared_ptr_get_data(self);
  TEN_ASSERT(ten_extension_info_check_integrity(extension_info, true),
             "Should not happen.");

  // Parse 'cmd'
  ten_json_t *cmds_json = ten_json_object_peek(json, TEN_STR_CMD);
  if (cmds_json) {
    if (!parse_msg_dest_json(cmds_json, extensions_info,
                             &extension_info->msg_dest_info.cmd,
                             extension_info)) {
      return NULL;
    }
  }

  // Parse 'data'
  ten_json_t *data_json = ten_json_object_peek(json, TEN_STR_DATA);
  if (data_json) {
    if (!parse_msg_dest_json(data_json, extensions_info,
                             &extension_info->msg_dest_info.data,
                             extension_info)) {
      return NULL;
    }
  }

  // Parse 'video_frame'
  ten_json_t *video_frame_json =
      ten_json_object_peek(json, TEN_STR_VIDEO_FRAME);
  if (video_frame_json) {
    if (!parse_msg_dest_json(video_frame_json, extensions_info,
                             &extension_info->msg_dest_info.video_frame,
                             extension_info)) {
      return NULL;
    }
  }

  // Parse 'audio_frame'
  ten_json_t *audio_frame_json =
      ten_json_object_peek(json, TEN_STR_AUDIO_FRAME);
  if (audio_frame_json) {
    if (!parse_msg_dest_json(audio_frame_json, extensions_info,
                             &extension_info->msg_dest_info.audio_frame,
                             extension_info)) {
      return NULL;
    }
  }

  // Parse 'interface'
  ten_json_t *interface_json = ten_json_object_peek(json, TEN_STR_INTERFACE);
  if (interface_json) {
    if (!parse_msg_dest_json(interface_json, extensions_info,
                             &extension_info->msg_dest_info.interface,
                             extension_info)) {
      return NULL;
    }
  }

  return self;
}

ten_shared_ptr_t *ten_extension_info_parse_connection_dest_part_from_json(
    ten_json_t *json, ten_list_t *extensions_info,
    ten_extension_info_t *src_extension_info, const char *origin_cmd_name,
    ten_error_t *err) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");
  TEN_ASSERT(extensions_info && ten_list_check_integrity(extensions_info),
             "Should not happen.");

  const char *app_uri = ten_json_object_peek_string(json, TEN_STR_APP);
  const char *graph_id = ten_json_object_peek_string(json, TEN_STR_GRAPH);

  const char *extension_group_name =
      ten_json_object_peek_string(json, TEN_STR_EXTENSION_GROUP);
  if (extension_group_name == NULL) {
    TEN_ASSERT(0, "Should not happen.");
    return NULL;
  }

  const char *extension_name =
      ten_json_object_peek_string(json, TEN_STR_EXTENSION);
  if (extension_name == NULL) {
    TEN_ASSERT(0, "Should not happen.");
    return NULL;
  }

  ten_shared_ptr_t *self = get_extension_info_in_extensions_info(
      extensions_info, app_uri, graph_id, extension_group_name, NULL,
      extension_name, NULL, err);
  if (!self) {
    return NULL;
  }

  ten_extension_info_t *extension_info = ten_shared_ptr_get_data(self);
  TEN_ASSERT(ten_extension_info_check_integrity(extension_info, true),
             "Should not happen.");

  // Parse 'msg_conversions'
  ten_json_t *msg_conversions_json =
      ten_json_object_peek(json, TEN_STR_MSG_CONVERSION);
  if (msg_conversions_json) {
    if (!parse_msg_conversions_json(
            msg_conversions_json, origin_cmd_name, src_extension_info,
            &extension_info->msg_conversion_contexts, err)) {
      TEN_ASSERT(0, "Should not happen.");
      return NULL;
    }
  }

  return self;
}
