//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/schema_store/store.h"

#include <stddef.h>
#include <string.h>

#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_runtime/schema_store/cmd.h"
#include "include_internal/ten_runtime/schema_store/interface.h"
#include "include_internal/ten_runtime/schema_store/msg.h"
#include "include_internal/ten_runtime/schema_store/property.h"
#include "include_internal/ten_utils/schema/constant_str.h"
#include "include_internal/ten_utils/schema/schema.h"
#include "include_internal/ten_utils/schema/types/schema_object.h"
#include "ten_runtime/common/errno.h"
#include "ten_runtime/msg/msg.h"
#include "ten_utils/container/hash_table.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_ptr.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/field.h"
#include "ten_utils/value/value.h"
#include "ten_utils/value/value_is.h"
#include "ten_utils/value/value_kv.h"
#include "ten_utils/value/value_object.h"

bool ten_schema_store_check_integrity(ten_schema_store_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  if (ten_signature_get(&self->signature) != TEN_SCHEMA_STORE_SIGNATURE) {
    return false;
  }

  return true;
}

// Parse 'cmd_in' and 'cmd_out'.
//
// "cmd_in": [
// ]
static void ten_schemas_parse_cmd_part(ten_hashtable_t *cmd_schema_map,
                                       ten_value_t *cmds_schema_value) {
  TEN_ASSERT(cmd_schema_map, "Invalid argument.");
  TEN_ASSERT(cmds_schema_value && ten_value_check_integrity(cmds_schema_value),
             "Invalid argument.");

  if (!ten_value_is_array(cmds_schema_value)) {
    TEN_ASSERT(0, "The schema should be an array.");
    return;
  }

  ten_value_array_foreach(cmds_schema_value, iter) {
    ten_value_t *cmd_schema_value = ten_ptr_listnode_get(iter.node);

    ten_cmd_schema_t *cmd_schema = ten_cmd_schema_create(cmd_schema_value);
    if (!cmd_schema) {
      TEN_ASSERT(0, "Failed to create schema for cmd.");
      return;
    }

    ten_hashtable_add_string(
        cmd_schema_map, &cmd_schema->hdr.hh_in_map,
        ten_string_get_raw_str(ten_cmd_schema_get_cmd_name(cmd_schema)),
        ten_cmd_schema_destroy);
  }
}

// "data_in": [
// ]
static void ten_schemas_parse_msg_part(ten_hashtable_t *msg_schema_map,
                                       ten_value_t *msgs_schema_value) {
  TEN_ASSERT(msg_schema_map, "Invalid argument.");
  TEN_ASSERT(msgs_schema_value && ten_value_check_integrity(msgs_schema_value),
             "Invalid argument.");

  if (!ten_value_is_array(msgs_schema_value)) {
    TEN_ASSERT(0, "The schema should be an array.");
    return;
  }

  ten_value_array_foreach(msgs_schema_value, iter) {
    ten_value_t *msg_schema_value = ten_ptr_listnode_get(iter.node);

    ten_msg_schema_t *msg_schema = ten_msg_schema_create(msg_schema_value);
    if (!msg_schema) {
      TEN_ASSERT(0, "Failed to create schema for msg.");
      return;
    }

    ten_hashtable_add_string(msg_schema_map, &msg_schema->hh_in_map,
                             ten_string_get_raw_str(&msg_schema->msg_name),
                             ten_msg_schema_destroy);
  }
}

void ten_schema_store_init(ten_schema_store_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  ten_signature_set(&self->signature, TEN_SCHEMA_STORE_SIGNATURE);

  self->property = NULL;
  ten_hashtable_init(&self->cmd_in, offsetof(ten_msg_schema_t, hh_in_map));
  ten_hashtable_init(&self->cmd_out, offsetof(ten_msg_schema_t, hh_in_map));
  ten_hashtable_init(&self->data_in, offsetof(ten_msg_schema_t, hh_in_map));
  ten_hashtable_init(&self->data_out, offsetof(ten_msg_schema_t, hh_in_map));
  ten_hashtable_init(&self->video_frame_in,
                     offsetof(ten_msg_schema_t, hh_in_map));
  ten_hashtable_init(&self->video_frame_out,
                     offsetof(ten_msg_schema_t, hh_in_map));
  ten_hashtable_init(&self->audio_frame_in,
                     offsetof(ten_msg_schema_t, hh_in_map));
  ten_hashtable_init(&self->audio_frame_out,
                     offsetof(ten_msg_schema_t, hh_in_map));
  ten_hashtable_init(&self->interface_in,
                     offsetof(ten_interface_schema_t, hh_in_map));
  ten_hashtable_init(&self->interface_out,
                     offsetof(ten_interface_schema_t, hh_in_map));
}

// The schema definition is as follows:
//
// {
//   "property": {},
//   "cmd_in": [],
//   "cmd_out": [],
//   "data_in": [],
//   "data_out": [],
//   "video_frame_in": [],
//   "video_frame_out": [],
//   "audio_frame_in": [],
//   "audio_frame_out": [],
//   "interface_in": [],
//   "interface_out": []
// }
//
// The `interface_in` and `interface_out` will not be parsed here, as they are
// only used in extensions. The base directory of the addon is needed to resolve
// the full definition of the interface schema, we will parse them alone.
bool ten_schema_store_set_schema_definition(ten_schema_store_t *self,
                                            ten_value_t *schema_def,
                                            ten_error_t *err) {
  TEN_ASSERT(self && ten_schema_store_check_integrity(self),
             "Invalid argument.");
  TEN_ASSERT(ten_value_check_integrity(schema_def), "Invalid argument.");
  TEN_ASSERT(err && ten_error_check_integrity(err), "Invalid argument.");

  if (!ten_value_is_object(schema_def)) {
    ten_error_set(err, TEN_ERRNO_GENERIC, "The schema should be an object.");
    return false;
  }

  // App/Extension property does not support `required` keyword.
  ten_value_t *required_schema_value =
      ten_value_object_peek(schema_def, TEN_SCHEMA_KEYWORD_STR_REQUIRED);
  if (required_schema_value) {
    ten_error_set(
        err, TEN_ERRNO_GENERIC,
        "The schema keyword [required] is only supported in the msg schema.");
    return false;
  }

  ten_value_t *props_schema_value =
      ten_value_object_peek(schema_def, TEN_STR_PROPERTY);
  if (props_schema_value) {
    if (!ten_value_is_object(props_schema_value)) {
      ten_error_set(err, TEN_ERRNO_GENERIC,
                    "The schema [property] should be an object.");
      return false;
    }

    self->property = ten_schemas_parse_schema_object_for_property(schema_def);
  }

  ten_value_t *cmds_in_schema_value =
      ten_value_object_peek(schema_def, TEN_STR_CMD_IN);
  if (cmds_in_schema_value) {
    ten_schemas_parse_cmd_part(&self->cmd_in, cmds_in_schema_value);
  }

  ten_value_t *cmds_out_schema_value =
      ten_value_object_peek(schema_def, TEN_STR_CMD_OUT);
  if (cmds_out_schema_value) {
    ten_schemas_parse_cmd_part(&self->cmd_out, cmds_out_schema_value);
  }

  ten_value_t *datas_in_schema_value =
      ten_value_object_peek(schema_def, TEN_STR_DATA_IN);
  if (datas_in_schema_value) {
    ten_schemas_parse_msg_part(&self->data_in, datas_in_schema_value);
  }

  ten_value_t *datas_out_schema_value =
      ten_value_object_peek(schema_def, TEN_STR_DATA_OUT);
  if (datas_out_schema_value) {
    ten_schemas_parse_msg_part(&self->data_out, datas_out_schema_value);
  }

  ten_value_t *video_frames_in_schema_value =
      ten_value_object_peek(schema_def, TEN_STR_VIDEO_FRAME_IN);
  if (video_frames_in_schema_value) {
    ten_schemas_parse_msg_part(&self->video_frame_in,
                               video_frames_in_schema_value);
  }

  ten_value_t *video_frames_out_schema_value =
      ten_value_object_peek(schema_def, TEN_STR_VIDEO_FRAME_OUT);
  if (video_frames_out_schema_value) {
    ten_schemas_parse_msg_part(&self->video_frame_out,
                               video_frames_out_schema_value);
  }

  ten_value_t *audio_frames_in_schema_value =
      ten_value_object_peek(schema_def, TEN_STR_AUDIO_FRAME_IN);
  if (audio_frames_in_schema_value) {
    ten_schemas_parse_msg_part(&self->audio_frame_in,
                               audio_frames_in_schema_value);
  }

  ten_value_t *audio_frames_out_schema_value =
      ten_value_object_peek(schema_def, TEN_STR_AUDIO_FRAME_OUT);
  if (audio_frames_out_schema_value) {
    ten_schemas_parse_msg_part(&self->audio_frame_out,
                               audio_frames_out_schema_value);
  }

  return true;
}

static void ten_schemas_parse_interface_part(
    ten_hashtable_t *interface_schema_map, ten_value_t *interface_schema_value,
    const char *base_dir) {
  TEN_ASSERT(interface_schema_map, "Invalid argument.");
  TEN_ASSERT(interface_schema_value &&
                 ten_value_check_integrity(interface_schema_value),
             "Invalid argument.");
  TEN_ASSERT(base_dir, "Invalid argument.");

  if (!ten_value_is_array(interface_schema_value)) {
    TEN_ASSERT(0, "The schema should be an array.");
    return;
  }

  ten_error_t err;
  ten_error_init(&err);

  do {
    ten_value_t *resolved_interface_schemas = ten_interface_schema_info_resolve(
        interface_schema_value, base_dir, &err);
    if (!resolved_interface_schemas) {
      TEN_LOGW("Failed to resolve interface schema, %s.",
               ten_error_errmsg(&err));
      break;
    }

    ten_value_array_foreach(resolved_interface_schemas, iter) {
      ten_value_t *resolved_interface_schema = ten_ptr_listnode_get(iter.node);

      ten_interface_schema_t *interface_schema =
          ten_interface_schema_create(resolved_interface_schema);
      if (!interface_schema) {
        TEN_ASSERT(0, "Failed to create schema for interface.");
        break;
      }

      ten_hashtable_add_string(interface_schema_map,
                               &interface_schema->hh_in_map,
                               ten_string_get_raw_str(&interface_schema->name),
                               ten_interface_schema_destroy);
    }

    ten_value_destroy(resolved_interface_schemas);
  } while (0);

  ten_error_deinit(&err);
}

static bool ten_schema_store_merge_interface_schemas_into_msg_schemas(
    ten_schema_store_t *self, bool is_msg_out,
    ten_hashtable_t *interface_schemas, ten_error_t *err) {
  TEN_ASSERT(self && ten_schema_store_check_integrity(self),
             "Invalid argument.");

  ten_hashtable_t *cmd_schemas_map =
      is_msg_out ? &self->cmd_out : &self->cmd_in;
  ten_hashtable_t *data_schemas_map =
      is_msg_out ? &self->data_out : &self->data_in;
  ten_hashtable_t *video_frame_schemas_map =
      is_msg_out ? &self->video_frame_out : &self->video_frame_in;
  ten_hashtable_t *audio_frame_schemas_map =
      is_msg_out ? &self->audio_frame_out : &self->audio_frame_in;

  ten_hashtable_foreach(interface_schemas, iter) {
    ten_hashhandle_t *hh = iter.node;
    ten_interface_schema_t *interface_schema =
        CONTAINER_OF_FROM_OFFSET(hh, interface_schemas->hh_offset);
    TEN_ASSERT(interface_schema &&
                   ten_interface_schema_check_integrity(interface_schema),
               "Invalid argument.");

    if (!ten_interface_schema_merge_into_msg_schema(
            interface_schema, TEN_MSG_TYPE_CMD, cmd_schemas_map, err)) {
      return false;
    }

    if (!ten_interface_schema_merge_into_msg_schema(
            interface_schema, TEN_MSG_TYPE_DATA, data_schemas_map, err)) {
      return false;
    }

    if (!ten_interface_schema_merge_into_msg_schema(
            interface_schema, TEN_MSG_TYPE_VIDEO_FRAME, video_frame_schemas_map,
            err)) {
      return false;
    }

    if (!ten_interface_schema_merge_into_msg_schema(
            interface_schema, TEN_MSG_TYPE_AUDIO_FRAME, audio_frame_schemas_map,
            err)) {
      return false;
    }
  }

  return true;
}

bool ten_schema_store_set_interface_schema_definition(ten_schema_store_t *self,
                                                      ten_value_t *schema_def,
                                                      const char *base_dir,
                                                      ten_error_t *err) {
  TEN_ASSERT(self && ten_schema_store_check_integrity(self),
             "Invalid argument.");
  TEN_ASSERT(schema_def && ten_value_check_integrity(schema_def),
             "Invalid argument.");
  TEN_ASSERT(err && ten_error_check_integrity(err), "Invalid argument.");
  TEN_ASSERT(base_dir, "Invalid argument.");

  if (!ten_value_is_object(schema_def)) {
    ten_error_set(err, TEN_ERRNO_GENERIC,
                  "The interface schema should be an object.");
    return false;
  }

  ten_value_t *interface_in =
      ten_value_object_peek(schema_def, TEN_STR_INTERFACE_IN);
  if (interface_in) {
    ten_schemas_parse_interface_part(&self->interface_in, interface_in,
                                     base_dir);

    if (!ten_schema_store_merge_interface_schemas_into_msg_schemas(
            self, false, &self->interface_in, err)) {
      return false;
    }
  }

  ten_value_t *interface_out =
      ten_value_object_peek(schema_def, TEN_STR_INTERFACE_OUT);
  if (interface_out) {
    ten_schemas_parse_interface_part(&self->interface_out, interface_out,
                                     base_dir);

    if (!ten_schema_store_merge_interface_schemas_into_msg_schemas(
            self, true, &self->interface_out, err)) {
      return false;
    }
  }

  return true;
}

void ten_schema_store_deinit(ten_schema_store_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  ten_signature_set(&self->signature, 0);
  if (self->property) {
    ten_schema_destroy(self->property);
    self->property = NULL;
  }

  ten_hashtable_clear(&self->cmd_in);
  ten_hashtable_clear(&self->cmd_out);
  ten_hashtable_clear(&self->data_in);
  ten_hashtable_clear(&self->data_out);
  ten_hashtable_clear(&self->video_frame_in);
  ten_hashtable_clear(&self->video_frame_out);
  ten_hashtable_clear(&self->audio_frame_in);
  ten_hashtable_clear(&self->audio_frame_out);
  ten_hashtable_clear(&self->interface_in);
  ten_hashtable_clear(&self->interface_out);
}

// {
//   "foo": 3,
//   "bar": "hello",
//   ...
// }
bool ten_schema_store_validate_properties(ten_schema_store_t *self,
                                          ten_value_t *props_value,
                                          ten_error_t *err) {
  TEN_ASSERT(self && ten_schema_store_check_integrity(self),
             "Invalid argument.");
  TEN_ASSERT(props_value && ten_value_check_integrity(props_value),
             "Invalid argument.");
  TEN_ASSERT(err && ten_error_check_integrity(err), "Invalid argument.");

  if (!self->property) {
    // No `property` schema is defined, which is permitted in TEN runtime.
    return true;
  }

  return ten_schema_validate_value(self->property, props_value, err);
}

bool ten_schema_store_validate_property_kv(ten_schema_store_t *self,
                                           const char *prop_name,
                                           ten_value_t *prop_value,
                                           ten_error_t *err) {
  TEN_ASSERT(self && ten_schema_store_check_integrity(self),
             "Invalid argument.");
  TEN_ASSERT(prop_name && strlen(prop_name), "Invalid argument.");
  TEN_ASSERT(prop_value && ten_value_check_integrity(prop_value),
             "Invalid argument.");

  if (!self->property) {
    // No `property` schema is defined, which is permitted in TEN runtime.
    return true;
  }

  ten_schema_t *prop_schema =
      ten_schema_object_peek_property_schema(self->property, prop_name);
  if (!prop_schema) {
    return true;
  }

  return ten_schema_validate_value(prop_schema, prop_value, err);
}

bool ten_schema_store_adjust_property_kv(ten_schema_store_t *self,
                                         const char *prop_name,
                                         ten_value_t *prop_value,
                                         ten_error_t *err) {
  TEN_ASSERT(self && ten_schema_store_check_integrity(self),
             "Invalid argument.");
  TEN_ASSERT(prop_name && strlen(prop_name), "Invalid argument.");
  TEN_ASSERT(prop_value && ten_value_check_integrity(prop_value),
             "Invalid argument.");

  if (!self->property) {
    // No `property` schema is defined, which is permitted in TEN runtime.
    return true;
  }

  ten_schema_t *prop_schema =
      ten_schema_object_peek_property_schema(self->property, prop_name);
  if (!prop_schema) {
    return true;
  }

  return ten_schema_adjust_value_type(prop_schema, prop_value, err);
}

bool ten_schema_store_adjust_properties(ten_schema_store_t *self,
                                        ten_value_t *props_value,
                                        ten_error_t *err) {
  TEN_ASSERT(self && ten_schema_store_check_integrity(self),
             "Invalid argument.");
  TEN_ASSERT(props_value && ten_value_check_integrity(props_value),
             "Invalid argument.");
  TEN_ASSERT(err && ten_error_check_integrity(err), "Invalid argument.");

  if (!self->property) {
    // No `property` schema is defined, which is permitted in TEN runtime.
    return true;
  }

  return ten_schema_adjust_value_type(self->property, props_value, err);
}

ten_msg_schema_t *ten_schema_store_get_msg_schema(ten_schema_store_t *self,
                                                  TEN_MSG_TYPE msg_type,
                                                  const char *msg_name,
                                                  bool is_msg_out) {
  TEN_ASSERT(self && ten_schema_store_check_integrity(self),
             "Invalid argument.");
  TEN_ASSERT(msg_type != TEN_MSG_TYPE_INVALID, "Invalid argument.");

  ten_hashtable_t *schema_map = NULL;
  switch (msg_type) {
    case TEN_MSG_TYPE_DATA:
      schema_map = is_msg_out ? &self->data_out : &self->data_in;
      break;

    case TEN_MSG_TYPE_VIDEO_FRAME:
      schema_map = is_msg_out ? &self->video_frame_out : &self->video_frame_in;
      break;

    case TEN_MSG_TYPE_AUDIO_FRAME:
      schema_map = is_msg_out ? &self->audio_frame_out : &self->audio_frame_in;
      break;

    case TEN_MSG_TYPE_CMD:
    case TEN_MSG_TYPE_CMD_RESULT:
      TEN_ASSERT(msg_name && strlen(msg_name), "Invalid argument.");
      schema_map = is_msg_out ? &self->cmd_out : &self->cmd_in;
      break;

    default:
      TEN_ASSERT(0, "Invalid argument.");
      break;
  }

  if (!schema_map) {
    return NULL;
  }

  if (!msg_name || ten_c_string_is_empty(msg_name)) {
    msg_name = TEN_STR_MSG_NAME_TEN_EMPTY;
  }

  ten_hashhandle_t *hh = ten_hashtable_find_string(schema_map, msg_name);
  if (hh) {
    return CONTAINER_OF_FROM_FIELD(hh, ten_msg_schema_t, hh_in_map);
  }

  return NULL;
}

bool ten_schema_store_get_all_msg_names_in_interface_out(
    ten_schema_store_t *self, TEN_MSG_TYPE msg_type, const char *interface_name,
    ten_list_t *msg_names) {
  TEN_ASSERT(self && ten_schema_store_check_integrity(self),
             "Invalid argument.");
  TEN_ASSERT(interface_name && msg_names, "Invalid argument.");

  ten_hashhandle_t *hh =
      ten_hashtable_find_string(&self->interface_out, interface_name);
  if (!hh) {
    return false;
  }

  ten_interface_schema_t *interface_schema =
      CONTAINER_OF_FROM_FIELD(hh, ten_interface_schema_t, hh_in_map);

  ten_list_t *msg_schemas = NULL;
  switch (msg_type) {
    case TEN_MSG_TYPE_CMD:
      msg_schemas = &interface_schema->cmd;
      break;

    case TEN_MSG_TYPE_DATA:
      msg_schemas = &interface_schema->data;
      break;

    case TEN_MSG_TYPE_VIDEO_FRAME:
      msg_schemas = &interface_schema->video_frame;
      break;

    case TEN_MSG_TYPE_AUDIO_FRAME:
      msg_schemas = &interface_schema->audio_frame;
      break;

    default:
      TEN_ASSERT(0, "Invalid argument.");
      break;
  }

  if (!msg_schemas) {
    return false;
  }

  ten_list_foreach (msg_schemas, iter) {
    ten_msg_schema_t *msg_schema = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(msg_schema && ten_msg_schema_check_integrity(msg_schema),
               "Invalid argument.");

    ten_list_push_ptr_back(msg_names, &msg_schema->msg_name, NULL);
  }

  return true;
}
