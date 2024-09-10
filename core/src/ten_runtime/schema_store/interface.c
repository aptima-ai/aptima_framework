//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/schema_store/interface.h"

#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/schema_store/cmd.h"
#include "include_internal/ten_runtime/schema_store/msg.h"
#include "ten_runtime/common/errno.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_node_ptr.h"
#include "ten_utils/container/list_ptr.h"
#include "ten_utils/lib/signature.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/memory.h"
#include "ten_utils/value/value.h"
#include "ten_utils/value/value_is.h"
#include "ten_utils/value/value_object.h"

bool ten_interface_schema_check_integrity(ten_interface_schema_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  if (ten_signature_get(&self->signature) != TEN_INTERFACE_SCHEMA_SIGNATURE) {
    return false;
  }

  return true;
}

static void ten_interface_schema_init(ten_interface_schema_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  ten_signature_set(&self->signature, TEN_INTERFACE_SCHEMA_SIGNATURE);
  ten_string_init(&self->name);
  ten_list_init(&self->cmd);
  ten_list_init(&self->data);
  ten_list_init(&self->video_frame);
  ten_list_init(&self->audio_frame);
}

static void ten_interface_schema_deinit(ten_interface_schema_t *self) {
  TEN_ASSERT(self && ten_interface_schema_check_integrity(self),
             "Invalid argument.");

  ten_signature_set(&self->signature, 0);
  ten_string_deinit(&self->name);
  ten_list_clear(&self->cmd);
  ten_list_clear(&self->data);
  ten_list_clear(&self->video_frame);
  ten_list_clear(&self->audio_frame);
}

static void ten_interface_schema_parse_cmd_part(
    ten_interface_schema_t *self, ten_value_t *cmd_schemas_value) {
  TEN_ASSERT(self && ten_interface_schema_check_integrity(self),
             "Invalid argument.");
  TEN_ASSERT(cmd_schemas_value && ten_value_check_integrity(cmd_schemas_value),
             "Invalid argument.");

  if (!ten_value_is_array(cmd_schemas_value)) {
    TEN_ASSERT(0, "The cmd part should be an array.");
    return;
  }

  ten_value_array_foreach(cmd_schemas_value, iter) {
    ten_value_t *cmd_schema_value = ten_ptr_listnode_get(iter.node);
    if (!ten_value_is_object(cmd_schema_value)) {
      TEN_ASSERT(0, "The cmd schema should be an object.");
      return;
    }

    ten_cmd_schema_t *cmd_schema = ten_cmd_schema_create(cmd_schema_value);
    TEN_ASSERT(cmd_schema, "Failed to create cmd schema.");

    ten_list_push_ptr_back(
        &self->cmd, cmd_schema,
        (ten_ptr_listnode_destroy_func_t)ten_cmd_schema_destroy);
  }
}

static void ten_interface_schema_parse_msg_part(
    ten_list_t *container, ten_value_t *msg_schemas_value) {
  TEN_ASSERT(container && msg_schemas_value, "Invalid argument.");

  if (ten_value_is_array(msg_schemas_value)) {
    TEN_ASSERT(0, "The msg part should be array.");
    return;
  }

  ten_value_array_foreach(msg_schemas_value, iter) {
    ten_value_t *msg_schema_value = ten_ptr_listnode_get(iter.node);
    if (!ten_value_is_object(msg_schema_value)) {
      TEN_ASSERT(0, "The msg schema should be an object.");
      return;
    }

    ten_msg_schema_t *msg_schema = ten_msg_schema_create(msg_schema_value);
    TEN_ASSERT(msg_schema, "Failed to create msg schema.");

    ten_list_push_ptr_back(
        container, msg_schema,
        (ten_ptr_listnode_destroy_func_t)ten_msg_schema_destroy);
  }
}

static void ten_interface_schema_set_definition(
    ten_interface_schema_t *self, ten_value_t *interface_schema_def) {
  TEN_ASSERT(self && ten_interface_schema_check_integrity(self),
             "Invalid argument.");
  TEN_ASSERT(
      interface_schema_def && ten_value_check_integrity(interface_schema_def),
      "Invalid argument.");

  if (!ten_value_is_object(interface_schema_def)) {
    TEN_ASSERT(0, "The interface schema should be an object.");
    return;
  }

  const char *name =
      ten_value_object_peek_string(interface_schema_def, TEN_STR_NAME);
  ten_string_set_formatted(&self->name, "%s", name);

  ten_value_t *cmd_schemas_value =
      ten_value_object_peek(interface_schema_def, TEN_STR_CMD);
  if (cmd_schemas_value) {
    ten_interface_schema_parse_cmd_part(self, cmd_schemas_value);
  }

  ten_value_t *data_schemas_value =
      ten_value_object_peek(interface_schema_def, TEN_STR_DATA);
  if (data_schemas_value) {
    ten_interface_schema_parse_msg_part(&self->data, data_schemas_value);
  }

  ten_value_t *video_frame_schemas_value =
      ten_value_object_peek(interface_schema_def, TEN_STR_VIDEO_FRAME);
  if (video_frame_schemas_value) {
    ten_interface_schema_parse_msg_part(&self->video_frame,
                                        video_frame_schemas_value);
  }

  ten_value_t *audio_frame_schemas_value =
      ten_value_object_peek(interface_schema_def, TEN_STR_AUDIO_FRAME);
  if (audio_frame_schemas_value) {
    ten_interface_schema_parse_msg_part(&self->audio_frame,
                                        audio_frame_schemas_value);
  }
}

ten_interface_schema_t *ten_interface_schema_create(
    ten_value_t *interface_schema_def) {
  TEN_ASSERT(
      interface_schema_def && ten_value_check_integrity(interface_schema_def),
      "Invalid argument.");

  ten_interface_schema_t *self = TEN_MALLOC(sizeof(ten_interface_schema_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  ten_interface_schema_init(self);
  ten_interface_schema_set_definition(self, interface_schema_def);

  return self;
}

void ten_interface_schema_destroy(ten_interface_schema_t *self) {
  TEN_ASSERT(self && ten_interface_schema_check_integrity(self),
             "Invalid argument.");

  ten_interface_schema_deinit(self);
  TEN_FREE(self);
}

bool ten_interface_schema_merge_into_msg_schema(ten_interface_schema_t *self,
                                                TEN_MSG_TYPE msg_type,
                                                ten_hashtable_t *msg_schema_map,
                                                ten_error_t *err) {
  TEN_ASSERT(self && ten_interface_schema_check_integrity(self),
             "Invalid argument.");

  ten_list_t *msg_schemas_in_interface = NULL;
  switch (msg_type) {
    case TEN_MSG_TYPE_CMD:
      msg_schemas_in_interface = &self->cmd;
      break;

    case TEN_MSG_TYPE_DATA:
      msg_schemas_in_interface = &self->data;
      break;

    case TEN_MSG_TYPE_VIDEO_FRAME:
      msg_schemas_in_interface = &self->video_frame;
      break;

    case TEN_MSG_TYPE_AUDIO_FRAME:
      msg_schemas_in_interface = &self->audio_frame;
      break;

    default:
      TEN_ASSERT(0, "Should not happen.");
      break;
  }

  if (!msg_schemas_in_interface) {
    return true;
  }

  ten_list_foreach (msg_schemas_in_interface, iter) {
    ten_msg_schema_t *msg_schema_in_interface = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(msg_schema_in_interface, "Should not happen.");

    if (ten_hashtable_find_string(
            msg_schema_map,
            ten_msg_schema_get_msg_name(msg_schema_in_interface))) {
      // There should not be duplicate schemas.
      ten_error_set(err, TEN_ERRNO_GENERIC, "Schema for %s is duplicated.",
                    ten_msg_schema_get_msg_name(msg_schema_in_interface));
      return false;
    }

    ten_hashtable_add_string(
        msg_schema_map, &msg_schema_in_interface->hh_in_map,
        ten_msg_schema_get_msg_name(msg_schema_in_interface), NULL);
  }

  return true;
}
