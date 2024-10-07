//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/extension/extension_info/value.h"

#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/extension/extension_info/extension_info.h"
#include "include_internal/ten_runtime/extension/extension_info/json.h"
#include "include_internal/ten_runtime/extension/msg_dest_info/value.h"
#include "include_internal/ten_runtime/msg_conversion/msg_conversion/msg_conversion.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/value/value_merge.h"
#include "ten_utils/value/value_object.h"

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
static bool parse_msg_dest_value(ten_value_t *value,
                                 ten_list_t *extensions_info,
                                 ten_list_t *static_dests,
                                 ten_extension_info_t *src_extension_info,
                                 ten_error_t *err) {
  TEN_ASSERT(value && ten_value_is_array(value) && extensions_info,
             "Should not happen.");
  TEN_ASSERT(static_dests, "Invalid argument.");
  TEN_ASSERT(src_extension_info,
             "src_extension must be specified in this case.");

  ten_value_array_foreach(value, iter) {
    ten_value_t *item_value = ten_ptr_listnode_get(iter.node);
    if (!ten_value_is_object(item_value)) {
      return false;
    }

    ten_shared_ptr_t *msg_dest = ten_msg_dest_info_from_value(
        item_value, extensions_info, src_extension_info, err);
    if (!msg_dest) {
      return false;
    }

    ten_list_push_smart_ptr_back(static_dests, msg_dest);
    ten_shared_ptr_destroy(msg_dest);
  }

  return true;
}

static bool parse_msg_conversions_value(
    ten_value_t *value, ten_extension_info_t *src_extension_info,
    const char *msg_name, ten_list_t *msg_conversions, ten_error_t *err) {
  TEN_ASSERT(src_extension_info, "Should not happen.");
  TEN_ASSERT(msg_name, "Should not happen.");

  ten_msg_conversion_t *msg_conversion =
      ten_msg_conversion_from_value(value, src_extension_info, msg_name, err);
  TEN_ASSERT(
      msg_conversion && ten_msg_conversion_check_integrity(msg_conversion),
      "Should not happen.");
  if (!msg_conversion) {
    return false;
  }

  return ten_msg_conversion_merge(msg_conversions, msg_conversion, err);
}

ten_shared_ptr_t *ten_extension_info_node_from_value(
    ten_value_t *value, ten_list_t *extensions_info, ten_error_t *err) {
  TEN_ASSERT(value && extensions_info, "Should not happen.");

  const char *app_uri = ten_value_object_peek_string(value, TEN_STR_APP);
  const char *graph_id = ten_value_object_peek_string(value, TEN_STR_GRAPH);
  const char *extension_group_name =
      ten_value_object_peek_string(value, TEN_STR_EXTENSION_GROUP);
  const char *addon_name = ten_value_object_peek_string(value, TEN_STR_ADDON);
  const char *instance_name = ten_value_object_peek_string(value, TEN_STR_NAME);

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
  ten_value_t *props_value = ten_value_object_peek(value, TEN_STR_PROPERTY);
  if (props_value) {
    if (!ten_value_is_object(props_value)) {
      // Indicates an error.
      TEN_ASSERT(0,
                 "Failed to parse 'prop' in 'start_graph' command, it's not an "
                 "object.");
      return NULL;
    }

    ten_value_object_merge_with_clone(extension_info->property, props_value);
  }

  return self;
}

ten_shared_ptr_t *ten_extension_info_parse_connection_src_part_from_value(
    ten_value_t *value, ten_list_t *extensions_info, ten_error_t *err) {
  TEN_ASSERT(value && extensions_info, "Should not happen.");

  const char *app_uri = ten_value_object_peek_string(value, TEN_STR_APP);
  const char *graph_id = ten_value_object_peek_string(value, TEN_STR_GRAPH);

  const char *extension_group_name =
      ten_value_object_peek_string(value, TEN_STR_EXTENSION_GROUP);
  if (!extension_group_name) {
    TEN_ASSERT(0, "Should not happen.");
    return NULL;
  }

  const char *extension_name =
      ten_value_object_peek_string(value, TEN_STR_EXTENSION);
  if (!extension_name) {
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
  ten_value_t *cmds_value = ten_value_object_peek(value, TEN_STR_CMD);
  if (cmds_value) {
    if (!parse_msg_dest_value(cmds_value, extensions_info,
                              &extension_info->msg_dest_info.cmd,
                              extension_info, err)) {
      return NULL;
    }
  }

  // Parse 'data'
  ten_value_t *data_value = ten_value_object_peek(value, TEN_STR_DATA);
  if (data_value) {
    if (!parse_msg_dest_value(data_value, extensions_info,
                              &extension_info->msg_dest_info.data,
                              extension_info, err)) {
      TEN_ASSERT(0, "Should not happen.");
      return NULL;
    }
  }

  // Parse 'video_frame'
  ten_value_t *video_frame_value =
      ten_value_object_peek(value, TEN_STR_VIDEO_FRAME);
  if (video_frame_value) {
    if (!parse_msg_dest_value(video_frame_value, extensions_info,
                              &extension_info->msg_dest_info.video_frame,
                              extension_info, err)) {
      TEN_ASSERT(0, "Should not happen.");
      return NULL;
    }
  }

  // Parse 'audio_frame'
  ten_value_t *audio_frame_value =
      ten_value_object_peek(value, TEN_STR_AUDIO_FRAME);
  if (audio_frame_value) {
    if (!parse_msg_dest_value(audio_frame_value, extensions_info,
                              &extension_info->msg_dest_info.audio_frame,
                              extension_info, err)) {
      TEN_ASSERT(0, "Should not happen.");
      return NULL;
    }
  }

  // Parse 'interface'.
  ten_value_t *interface_value =
      ten_value_object_peek(value, TEN_STR_INTERFACE);
  if (interface_value) {
    if (!parse_msg_dest_value(interface_value, extensions_info,
                              &extension_info->msg_dest_info.interface,
                              extension_info, err)) {
      TEN_ASSERT(0, "Should not happen.");
      return NULL;
    }
  }

  return self;
}

ten_shared_ptr_t *ten_extension_info_parse_connection_dest_part_from_value(
    ten_value_t *value, ten_list_t *extensions_info,
    ten_extension_info_t *src_extension_info, const char *origin_cmd_name,
    ten_error_t *err) {
  TEN_ASSERT(value && extensions_info, "Should not happen.");

  const char *app_uri = ten_value_object_peek_string(value, TEN_STR_APP);
  const char *graph_id = ten_value_object_peek_string(value, TEN_STR_GRAPH);

  const char *extension_group_name =
      ten_value_object_peek_string(value, TEN_STR_EXTENSION_GROUP);
  if (!extension_group_name) {
    TEN_ASSERT(0, "Should not happen.");
    return NULL;
  }

  const char *extension_name =
      ten_value_object_peek_string(value, TEN_STR_EXTENSION);
  if (!extension_name) {
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
  ten_value_t *msg_conversions_value =
      ten_value_object_peek(value, TEN_STR_MSG_CONVERSION);
  if (msg_conversions_value) {
    if (!parse_msg_conversions_value(msg_conversions_value, src_extension_info,
                                     origin_cmd_name,
                                     &extension_info->msg_conversions, err)) {
      TEN_ASSERT(0, "Should not happen.");
      return NULL;
    }
  }

  return self;
}
