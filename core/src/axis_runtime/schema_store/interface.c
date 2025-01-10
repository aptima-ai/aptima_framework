//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/schema_store/interface.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/schema_store/cmd.h"
#include "include_internal/axis_runtime/schema_store/msg.h"
#include "axis_runtime/common/errno.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node_ptr.h"
#include "axis_utils/container/list_ptr.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_is.h"
#include "axis_utils/value/value_object.h"

bool axis_interface_schema_check_integrity(axis_interface_schema_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (axis_signature_get(&self->signature) != axis_INTERFACE_SCHEMA_SIGNATURE) {
    return false;
  }

  return true;
}

static void axis_interface_schema_init(axis_interface_schema_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_signature_set(&self->signature, axis_INTERFACE_SCHEMA_SIGNATURE);
  axis_string_init(&self->name);
  axis_list_init(&self->cmd);
  axis_list_init(&self->data);
  axis_list_init(&self->video_frame);
  axis_list_init(&self->audio_frame);
}

static void axis_interface_schema_deinit(axis_interface_schema_t *self) {
  axis_ASSERT(self && axis_interface_schema_check_integrity(self),
             "Invalid argument.");

  axis_signature_set(&self->signature, 0);
  axis_string_deinit(&self->name);
  axis_list_clear(&self->cmd);
  axis_list_clear(&self->data);
  axis_list_clear(&self->video_frame);
  axis_list_clear(&self->audio_frame);
}

static void axis_interface_schema_parse_cmd_part(
    axis_interface_schema_t *self, axis_value_t *cmd_schemas_value) {
  axis_ASSERT(self && axis_interface_schema_check_integrity(self),
             "Invalid argument.");
  axis_ASSERT(cmd_schemas_value && axis_value_check_integrity(cmd_schemas_value),
             "Invalid argument.");

  if (!axis_value_is_array(cmd_schemas_value)) {
    axis_ASSERT(0, "The cmd part should be an array.");
    return;
  }

  axis_value_array_foreach(cmd_schemas_value, iter) {
    axis_value_t *cmd_schema_value = axis_ptr_listnode_get(iter.node);
    if (!axis_value_is_object(cmd_schema_value)) {
      axis_ASSERT(0, "The cmd schema should be an object.");
      return;
    }

    axis_cmd_schema_t *cmd_schema = axis_cmd_schema_create(cmd_schema_value);
    axis_ASSERT(cmd_schema, "Failed to create cmd schema.");

    axis_list_push_ptr_back(
        &self->cmd, cmd_schema,
        (axis_ptr_listnode_destroy_func_t)axis_cmd_schema_destroy);
  }
}

static void axis_interface_schema_parse_msg_part(
    axis_list_t *container, axis_value_t *msg_schemas_value) {
  axis_ASSERT(container && msg_schemas_value, "Invalid argument.");

  if (axis_value_is_array(msg_schemas_value)) {
    axis_ASSERT(0, "The msg part should be array.");
    return;
  }

  axis_value_array_foreach(msg_schemas_value, iter) {
    axis_value_t *msg_schema_value = axis_ptr_listnode_get(iter.node);
    if (!axis_value_is_object(msg_schema_value)) {
      axis_ASSERT(0, "The msg schema should be an object.");
      return;
    }

    axis_msg_schema_t *msg_schema = axis_msg_schema_create(msg_schema_value);
    axis_ASSERT(msg_schema, "Failed to create msg schema.");

    axis_list_push_ptr_back(
        container, msg_schema,
        (axis_ptr_listnode_destroy_func_t)axis_msg_schema_destroy);
  }
}

static void axis_interface_schema_set_definition(
    axis_interface_schema_t *self, axis_value_t *interface_schema_def) {
  axis_ASSERT(self && axis_interface_schema_check_integrity(self),
             "Invalid argument.");
  axis_ASSERT(
      interface_schema_def && axis_value_check_integrity(interface_schema_def),
      "Invalid argument.");

  if (!axis_value_is_object(interface_schema_def)) {
    axis_ASSERT(0, "The interface schema should be an object.");
    return;
  }

  const char *name =
      axis_value_object_peek_string(interface_schema_def, axis_STR_NAME);
  axis_string_set_formatted(&self->name, "%s", name);

  axis_value_t *cmd_schemas_value =
      axis_value_object_peek(interface_schema_def, axis_STR_CMD);
  if (cmd_schemas_value) {
    axis_interface_schema_parse_cmd_part(self, cmd_schemas_value);
  }

  axis_value_t *data_schemas_value =
      axis_value_object_peek(interface_schema_def, axis_STR_DATA);
  if (data_schemas_value) {
    axis_interface_schema_parse_msg_part(&self->data, data_schemas_value);
  }

  axis_value_t *video_frame_schemas_value =
      axis_value_object_peek(interface_schema_def, axis_STR_VIDEO_FRAME);
  if (video_frame_schemas_value) {
    axis_interface_schema_parse_msg_part(&self->video_frame,
                                        video_frame_schemas_value);
  }

  axis_value_t *audio_frame_schemas_value =
      axis_value_object_peek(interface_schema_def, axis_STR_AUDIO_FRAME);
  if (audio_frame_schemas_value) {
    axis_interface_schema_parse_msg_part(&self->audio_frame,
                                        audio_frame_schemas_value);
  }
}

axis_interface_schema_t *axis_interface_schema_create(
    axis_value_t *interface_schema_def) {
  axis_ASSERT(
      interface_schema_def && axis_value_check_integrity(interface_schema_def),
      "Invalid argument.");

  axis_interface_schema_t *self = axis_MALLOC(sizeof(axis_interface_schema_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_interface_schema_init(self);
  axis_interface_schema_set_definition(self, interface_schema_def);

  return self;
}

void axis_interface_schema_destroy(axis_interface_schema_t *self) {
  axis_ASSERT(self && axis_interface_schema_check_integrity(self),
             "Invalid argument.");

  axis_interface_schema_deinit(self);
  axis_FREE(self);
}

bool axis_interface_schema_merge_into_msg_schema(axis_interface_schema_t *self,
                                                axis_MSG_TYPE msg_type,
                                                axis_hashtable_t *msg_schema_map,
                                                axis_error_t *err) {
  axis_ASSERT(self && axis_interface_schema_check_integrity(self),
             "Invalid argument.");

  axis_list_t *msg_schemas_in_interface = NULL;
  switch (msg_type) {
    case axis_MSG_TYPE_CMD:
      msg_schemas_in_interface = &self->cmd;
      break;

    case axis_MSG_TYPE_DATA:
      msg_schemas_in_interface = &self->data;
      break;

    case axis_MSG_TYPE_VIDEO_FRAME:
      msg_schemas_in_interface = &self->video_frame;
      break;

    case axis_MSG_TYPE_AUDIO_FRAME:
      msg_schemas_in_interface = &self->audio_frame;
      break;

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  if (!msg_schemas_in_interface) {
    return true;
  }

  axis_list_foreach (msg_schemas_in_interface, iter) {
    axis_msg_schema_t *msg_schema_in_interface = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(msg_schema_in_interface, "Should not happen.");

    if (axis_hashtable_find_string(
            msg_schema_map,
            axis_msg_schema_get_msg_name(msg_schema_in_interface))) {
      // There should not be duplicate schemas.
      axis_error_set(err, axis_ERRNO_GENERIC, "Schema for %s is duplicated.",
                    axis_msg_schema_get_msg_name(msg_schema_in_interface));
      return false;
    }

    axis_hashtable_add_string(
        msg_schema_map, &msg_schema_in_interface->hh_in_map,
        axis_msg_schema_get_msg_name(msg_schema_in_interface), NULL);
  }

  return true;
}
