//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/schema_store/store.h"

#include <stddef.h>
#include <string.h>

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/schema_store/cmd.h"
#include "include_internal/axis_runtime/schema_store/interface.h"
#include "include_internal/axis_runtime/schema_store/msg.h"
#include "include_internal/axis_runtime/schema_store/property.h"
#include "include_internal/axis_utils/schema/constant_str.h"
#include "include_internal/axis_utils/schema/schema.h"
#include "include_internal/axis_utils/schema/types/schema_object.h"
#include "axis_runtime/common/errno.h"
#include "axis_runtime/msg/msg.h"
#include "axis_utils/container/hash_table.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_ptr.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/field.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_is.h"
#include "axis_utils/value/value_kv.h"
#include "axis_utils/value/value_object.h"

bool axis_schema_store_check_integrity(axis_schema_store_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (axis_signature_get(&self->signature) != axis_SCHEMA_STORE_SIGNATURE) {
    return false;
  }

  return true;
}

// Parse 'cmd_in' and 'cmd_out'.
//
// "cmd_in": [
// ]
static void axis_schemas_parse_cmd_part(axis_hashtable_t *cmd_schema_map,
                                       axis_value_t *cmds_schema_value) {
  axis_ASSERT(cmd_schema_map, "Invalid argument.");
  axis_ASSERT(cmds_schema_value && axis_value_check_integrity(cmds_schema_value),
             "Invalid argument.");

  if (!axis_value_is_array(cmds_schema_value)) {
    axis_ASSERT(0, "The schema should be an array.");
    return;
  }

  axis_value_array_foreach(cmds_schema_value, iter) {
    axis_value_t *cmd_schema_value = axis_ptr_listnode_get(iter.node);

    axis_cmd_schema_t *cmd_schema = axis_cmd_schema_create(cmd_schema_value);
    if (!cmd_schema) {
      axis_ASSERT(0, "Failed to create schema for cmd.");
      return;
    }

    axis_hashtable_add_string(
        cmd_schema_map, &cmd_schema->hdr.hh_in_map,
        axis_string_get_raw_str(axis_cmd_schema_get_cmd_name(cmd_schema)),
        axis_cmd_schema_destroy);
  }
}

// "data_in": [
// ]
static void axis_schemas_parse_msg_part(axis_hashtable_t *msg_schema_map,
                                       axis_value_t *msgs_schema_value) {
  axis_ASSERT(msg_schema_map, "Invalid argument.");
  axis_ASSERT(msgs_schema_value && axis_value_check_integrity(msgs_schema_value),
             "Invalid argument.");

  if (!axis_value_is_array(msgs_schema_value)) {
    axis_ASSERT(0, "The schema should be an array.");
    return;
  }

  axis_value_array_foreach(msgs_schema_value, iter) {
    axis_value_t *msg_schema_value = axis_ptr_listnode_get(iter.node);

    axis_msg_schema_t *msg_schema = axis_msg_schema_create(msg_schema_value);
    if (!msg_schema) {
      axis_ASSERT(0, "Failed to create schema for msg.");
      return;
    }

    axis_hashtable_add_string(msg_schema_map, &msg_schema->hh_in_map,
                             axis_string_get_raw_str(&msg_schema->msg_name),
                             axis_msg_schema_destroy);
  }
}

void axis_schema_store_init(axis_schema_store_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_signature_set(&self->signature, axis_SCHEMA_STORE_SIGNATURE);

  self->property = NULL;
  axis_hashtable_init(&self->cmd_in, offsetof(axis_msg_schema_t, hh_in_map));
  axis_hashtable_init(&self->cmd_out, offsetof(axis_msg_schema_t, hh_in_map));
  axis_hashtable_init(&self->data_in, offsetof(axis_msg_schema_t, hh_in_map));
  axis_hashtable_init(&self->data_out, offsetof(axis_msg_schema_t, hh_in_map));
  axis_hashtable_init(&self->video_frame_in,
                     offsetof(axis_msg_schema_t, hh_in_map));
  axis_hashtable_init(&self->video_frame_out,
                     offsetof(axis_msg_schema_t, hh_in_map));
  axis_hashtable_init(&self->audio_frame_in,
                     offsetof(axis_msg_schema_t, hh_in_map));
  axis_hashtable_init(&self->audio_frame_out,
                     offsetof(axis_msg_schema_t, hh_in_map));
  axis_hashtable_init(&self->interface_in,
                     offsetof(axis_interface_schema_t, hh_in_map));
  axis_hashtable_init(&self->interface_out,
                     offsetof(axis_interface_schema_t, hh_in_map));
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
bool axis_schema_store_set_schema_definition(axis_schema_store_t *self,
                                            axis_value_t *schema_def,
                                            axis_error_t *err) {
  axis_ASSERT(self && axis_schema_store_check_integrity(self),
             "Invalid argument.");
  axis_ASSERT(axis_value_check_integrity(schema_def), "Invalid argument.");
  axis_ASSERT(err && axis_error_check_integrity(err), "Invalid argument.");

  if (!axis_value_is_object(schema_def)) {
    axis_error_set(err, axis_ERRNO_GENERIC, "The schema should be an object.");
    return false;
  }

  // App/Extension property does not support `required` keyword.
  axis_value_t *required_schema_value =
      axis_value_object_peek(schema_def, axis_SCHEMA_KEYWORD_STR_REQUIRED);
  if (required_schema_value) {
    axis_error_set(
        err, axis_ERRNO_GENERIC,
        "The schema keyword [required] is only supported in the msg schema.");
    return false;
  }

  axis_value_t *props_schema_value =
      axis_value_object_peek(schema_def, axis_STR_PROPERTY);
  if (props_schema_value) {
    if (!axis_value_is_object(props_schema_value)) {
      axis_error_set(err, axis_ERRNO_GENERIC,
                    "The schema [property] should be an object.");
      return false;
    }

    self->property = axis_schemas_parse_schema_object_for_property(schema_def);
  }

  axis_value_t *cmds_in_schema_value =
      axis_value_object_peek(schema_def, axis_STR_CMD_IN);
  if (cmds_in_schema_value) {
    axis_schemas_parse_cmd_part(&self->cmd_in, cmds_in_schema_value);
  }

  axis_value_t *cmds_out_schema_value =
      axis_value_object_peek(schema_def, axis_STR_CMD_OUT);
  if (cmds_out_schema_value) {
    axis_schemas_parse_cmd_part(&self->cmd_out, cmds_out_schema_value);
  }

  axis_value_t *datas_in_schema_value =
      axis_value_object_peek(schema_def, axis_STR_DATA_IN);
  if (datas_in_schema_value) {
    axis_schemas_parse_msg_part(&self->data_in, datas_in_schema_value);
  }

  axis_value_t *datas_out_schema_value =
      axis_value_object_peek(schema_def, axis_STR_DATA_OUT);
  if (datas_out_schema_value) {
    axis_schemas_parse_msg_part(&self->data_out, datas_out_schema_value);
  }

  axis_value_t *video_frames_in_schema_value =
      axis_value_object_peek(schema_def, axis_STR_VIDEO_FRAME_IN);
  if (video_frames_in_schema_value) {
    axis_schemas_parse_msg_part(&self->video_frame_in,
                               video_frames_in_schema_value);
  }

  axis_value_t *video_frames_out_schema_value =
      axis_value_object_peek(schema_def, axis_STR_VIDEO_FRAME_OUT);
  if (video_frames_out_schema_value) {
    axis_schemas_parse_msg_part(&self->video_frame_out,
                               video_frames_out_schema_value);
  }

  axis_value_t *audio_frames_in_schema_value =
      axis_value_object_peek(schema_def, axis_STR_AUDIO_FRAME_IN);
  if (audio_frames_in_schema_value) {
    axis_schemas_parse_msg_part(&self->audio_frame_in,
                               audio_frames_in_schema_value);
  }

  axis_value_t *audio_frames_out_schema_value =
      axis_value_object_peek(schema_def, axis_STR_AUDIO_FRAME_OUT);
  if (audio_frames_out_schema_value) {
    axis_schemas_parse_msg_part(&self->audio_frame_out,
                               audio_frames_out_schema_value);
  }

  return true;
}

#if defined(axis_ENABLE_axis_RUST_APIS)
static void axis_schemas_parse_interface_part(
    axis_hashtable_t *interface_schema_map, axis_value_t *interface_schema_value,
    const char *base_dir) {
  axis_ASSERT(interface_schema_map, "Invalid argument.");
  axis_ASSERT(interface_schema_value &&
                 axis_value_check_integrity(interface_schema_value),
             "Invalid argument.");

  if (!axis_value_is_array(interface_schema_value)) {
    axis_ASSERT(0, "The schema should be an array.");
    return;
  }

  axis_error_t err;
  axis_error_init(&err);

  do {
    axis_value_t *resolved_interface_schemas = axis_interface_schema_info_resolve(
        interface_schema_value, base_dir, &err);
    if (!resolved_interface_schemas) {
      axis_LOGW("Failed to resolve interface schema, %s.",
               axis_error_errmsg(&err));
      break;
    }

    axis_value_array_foreach(resolved_interface_schemas, iter) {
      axis_value_t *resolved_interface_schema = axis_ptr_listnode_get(iter.node);

      axis_interface_schema_t *interface_schema =
          axis_interface_schema_create(resolved_interface_schema);
      if (!interface_schema) {
        axis_ASSERT(0, "Failed to create schema for interface.");
        break;
      }

      axis_hashtable_add_string(interface_schema_map,
                               &interface_schema->hh_in_map,
                               axis_string_get_raw_str(&interface_schema->name),
                               axis_interface_schema_destroy);
    }

    axis_value_destroy(resolved_interface_schemas);
  } while (0);

  axis_error_deinit(&err);
}

static bool axis_schema_store_merge_interface_schemas_into_msg_schemas(
    axis_schema_store_t *self, bool is_msg_out,
    axis_hashtable_t *interface_schemas, axis_error_t *err) {
  axis_ASSERT(self && axis_schema_store_check_integrity(self),
             "Invalid argument.");

  axis_hashtable_t *cmd_schemas_map =
      is_msg_out ? &self->cmd_out : &self->cmd_in;
  axis_hashtable_t *data_schemas_map =
      is_msg_out ? &self->data_out : &self->data_in;
  axis_hashtable_t *video_frame_schemas_map =
      is_msg_out ? &self->video_frame_out : &self->video_frame_in;
  axis_hashtable_t *audio_frame_schemas_map =
      is_msg_out ? &self->audio_frame_out : &self->audio_frame_in;

  axis_hashtable_foreach(interface_schemas, iter) {
    axis_hashhandle_t *hh = iter.node;
    axis_interface_schema_t *interface_schema =
        CONTAINER_OF_FROM_OFFSET(hh, interface_schemas->hh_offset);
    axis_ASSERT(interface_schema &&
                   axis_interface_schema_check_integrity(interface_schema),
               "Invalid argument.");

    if (!axis_interface_schema_merge_into_msg_schema(
            interface_schema, axis_MSG_TYPE_CMD, cmd_schemas_map, err)) {
      return false;
    }

    if (!axis_interface_schema_merge_into_msg_schema(
            interface_schema, axis_MSG_TYPE_DATA, data_schemas_map, err)) {
      return false;
    }

    if (!axis_interface_schema_merge_into_msg_schema(
            interface_schema, axis_MSG_TYPE_VIDEO_FRAME, video_frame_schemas_map,
            err)) {
      return false;
    }

    if (!axis_interface_schema_merge_into_msg_schema(
            interface_schema, axis_MSG_TYPE_AUDIO_FRAME, audio_frame_schemas_map,
            err)) {
      return false;
    }
  }

  return true;
}
#endif

/**
 * @param base_dir The base directory of the addon. If the interface definition
 * is a file reference, it is used to resolve the file reference based on the
 * base_dir.
 */
bool axis_schema_store_set_interface_schema_definition(
    axis_schema_store_t *self, axis_value_t *schema_def,
    axis_UNUSED const char *base_dir, axis_error_t *err) {
  axis_ASSERT(self && axis_schema_store_check_integrity(self),
             "Invalid argument.");
  axis_ASSERT(schema_def && axis_value_check_integrity(schema_def),
             "Invalid argument.");
  axis_ASSERT(err && axis_error_check_integrity(err), "Invalid argument.");

#if defined(axis_ENABLE_axis_RUST_APIS)
  if (!axis_value_is_object(schema_def)) {
    axis_error_set(err, axis_ERRNO_GENERIC,
                  "The interface schema should be an object.");
    return false;
  }

  axis_value_t *interface_in =
      axis_value_object_peek(schema_def, axis_STR_INTERFACE_IN);
  if (interface_in) {
    axis_schemas_parse_interface_part(&self->interface_in, interface_in,
                                     base_dir);

    if (!axis_schema_store_merge_interface_schemas_into_msg_schemas(
            self, false, &self->interface_in, err)) {
      return false;
    }
  }

  axis_value_t *interface_out =
      axis_value_object_peek(schema_def, axis_STR_INTERFACE_OUT);
  if (interface_out) {
    axis_schemas_parse_interface_part(&self->interface_out, interface_out,
                                     base_dir);

    if (!axis_schema_store_merge_interface_schemas_into_msg_schemas(
            self, true, &self->interface_out, err)) {
      return false;
    }
  }
#endif

  return true;
}

void axis_schema_store_deinit(axis_schema_store_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_signature_set(&self->signature, 0);
  if (self->property) {
    axis_schema_destroy(self->property);
    self->property = NULL;
  }

  axis_hashtable_deinit(&self->cmd_in);
  axis_hashtable_deinit(&self->cmd_out);
  axis_hashtable_deinit(&self->data_in);
  axis_hashtable_deinit(&self->data_out);
  axis_hashtable_deinit(&self->video_frame_in);
  axis_hashtable_deinit(&self->video_frame_out);
  axis_hashtable_deinit(&self->audio_frame_in);
  axis_hashtable_deinit(&self->audio_frame_out);
  axis_hashtable_deinit(&self->interface_in);
  axis_hashtable_deinit(&self->interface_out);
}

// {
//   "foo": 3,
//   "bar": "hello",
//   ...
// }
bool axis_schema_store_validate_properties(axis_schema_store_t *self,
                                          axis_value_t *props_value,
                                          axis_error_t *err) {
  axis_ASSERT(self && axis_schema_store_check_integrity(self),
             "Invalid argument.");
  axis_ASSERT(props_value && axis_value_check_integrity(props_value),
             "Invalid argument.");
  axis_ASSERT(err && axis_error_check_integrity(err), "Invalid argument.");

  if (!self->property) {
    // No `property` schema is defined, which is permitted in APTIMA runtime.
    return true;
  }

  return axis_schema_validate_value(self->property, props_value, err);
}

bool axis_schema_store_validate_property_kv(axis_schema_store_t *self,
                                           const char *prop_name,
                                           axis_value_t *prop_value,
                                           axis_error_t *err) {
  axis_ASSERT(self && axis_schema_store_check_integrity(self),
             "Invalid argument.");
  axis_ASSERT(prop_name && strlen(prop_name), "Invalid argument.");
  axis_ASSERT(prop_value && axis_value_check_integrity(prop_value),
             "Invalid argument.");

  if (!self->property) {
    // No `property` schema is defined, which is permitted in APTIMA runtime.
    return true;
  }

  axis_schema_t *prop_schema =
      axis_schema_object_peek_property_schema(self->property, prop_name);
  if (!prop_schema) {
    return true;
  }

  return axis_schema_validate_value(prop_schema, prop_value, err);
}

bool axis_schema_store_adjust_property_kv(axis_schema_store_t *self,
                                         const char *prop_name,
                                         axis_value_t *prop_value,
                                         axis_error_t *err) {
  axis_ASSERT(self && axis_schema_store_check_integrity(self),
             "Invalid argument.");
  axis_ASSERT(prop_name && strlen(prop_name), "Invalid argument.");
  axis_ASSERT(prop_value && axis_value_check_integrity(prop_value),
             "Invalid argument.");

  if (!self->property) {
    // No `property` schema is defined, which is permitted in APTIMA runtime.
    return true;
  }

  axis_schema_t *prop_schema =
      axis_schema_object_peek_property_schema(self->property, prop_name);
  if (!prop_schema) {
    return true;
  }

  return axis_schema_adjust_value_type(prop_schema, prop_value, err);
}

bool axis_schema_store_adjust_properties(axis_schema_store_t *self,
                                        axis_value_t *props_value,
                                        axis_error_t *err) {
  axis_ASSERT(self && axis_schema_store_check_integrity(self),
             "Invalid argument.");
  axis_ASSERT(props_value && axis_value_check_integrity(props_value),
             "Invalid argument.");
  axis_ASSERT(err && axis_error_check_integrity(err), "Invalid argument.");

  if (!self->property) {
    // No `property` schema is defined, which is permitted in APTIMA runtime.
    return true;
  }

  return axis_schema_adjust_value_type(self->property, props_value, err);
}

axis_msg_schema_t *axis_schema_store_get_msg_schema(axis_schema_store_t *self,
                                                  axis_MSG_TYPE msg_type,
                                                  const char *msg_name,
                                                  bool is_msg_out) {
  axis_ASSERT(self && axis_schema_store_check_integrity(self),
             "Invalid argument.");
  axis_ASSERT(msg_type != axis_MSG_TYPE_INVALID, "Invalid argument.");

  axis_hashtable_t *schema_map = NULL;
  switch (msg_type) {
    case axis_MSG_TYPE_DATA:
      schema_map = is_msg_out ? &self->data_out : &self->data_in;
      break;

    case axis_MSG_TYPE_VIDEO_FRAME:
      schema_map = is_msg_out ? &self->video_frame_out : &self->video_frame_in;
      break;

    case axis_MSG_TYPE_AUDIO_FRAME:
      schema_map = is_msg_out ? &self->audio_frame_out : &self->audio_frame_in;
      break;

    case axis_MSG_TYPE_CMD:
    case axis_MSG_TYPE_CMD_RESULT:
      axis_ASSERT(msg_name && strlen(msg_name), "Invalid argument.");
      schema_map = is_msg_out ? &self->cmd_out : &self->cmd_in;
      break;

    default:
      axis_ASSERT(0, "Invalid argument.");
      break;
  }

  if (!schema_map) {
    return NULL;
  }

  if (!msg_name || axis_c_string_is_empty(msg_name)) {
    msg_name = axis_STR_MSG_NAME_axis_EMPTY;
  }

  axis_hashhandle_t *hh = axis_hashtable_find_string(schema_map, msg_name);
  if (hh) {
    return CONTAINER_OF_FROM_FIELD(hh, axis_msg_schema_t, hh_in_map);
  }

  return NULL;
}

bool axis_schema_store_get_all_msg_names_in_interface_out(
    axis_schema_store_t *self, axis_MSG_TYPE msg_type, const char *interface_name,
    axis_list_t *msg_names) {
  axis_ASSERT(self && axis_schema_store_check_integrity(self),
             "Invalid argument.");
  axis_ASSERT(interface_name && msg_names, "Invalid argument.");

  axis_hashhandle_t *hh =
      axis_hashtable_find_string(&self->interface_out, interface_name);
  if (!hh) {
    return false;
  }

  axis_interface_schema_t *interface_schema =
      CONTAINER_OF_FROM_FIELD(hh, axis_interface_schema_t, hh_in_map);

  axis_list_t *msg_schemas = NULL;
  switch (msg_type) {
    case axis_MSG_TYPE_CMD:
      msg_schemas = &interface_schema->cmd;
      break;

    case axis_MSG_TYPE_DATA:
      msg_schemas = &interface_schema->data;
      break;

    case axis_MSG_TYPE_VIDEO_FRAME:
      msg_schemas = &interface_schema->video_frame;
      break;

    case axis_MSG_TYPE_AUDIO_FRAME:
      msg_schemas = &interface_schema->audio_frame;
      break;

    default:
      axis_ASSERT(0, "Invalid argument.");
      break;
  }

  if (!msg_schemas) {
    return false;
  }

  axis_list_foreach (msg_schemas, iter) {
    axis_msg_schema_t *msg_schema = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(msg_schema && axis_msg_schema_check_integrity(msg_schema),
               "Invalid argument.");

    axis_list_push_ptr_back(msg_names, &msg_schema->msg_name, NULL);
  }

  return true;
}
