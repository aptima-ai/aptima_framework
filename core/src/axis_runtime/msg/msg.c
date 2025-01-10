//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg/msg.h"

#include <string.h>

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/app/predefined_graph.h"
#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/common/loc.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension/extension_info/extension_info.h"
#include "include_internal/axis_runtime/extension_context/extension_context.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/extension_group/on_xxx.h"
#include "include_internal/axis_runtime/extension_store/extension_store.h"
#include "include_internal/axis_runtime/extension_thread/extension_thread.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/axis_runtime/msg/field/field_info.h"
#include "include_internal/axis_runtime/msg/msg_info.h"
#include "include_internal/axis_runtime/schema_store/msg.h"
#include "include_internal/axis_utils/value/value_path.h"
#include "axis_runtime/app/app.h"
#include "axis_runtime/common/errno.h"
#include "axis_runtime/msg/cmd/close_app/cmd.h"
#include "axis_runtime/msg/cmd/stop_graph/cmd.h"
#include "axis_runtime/msg/cmd_result/cmd_result.h"
#include "axis_runtime/msg/msg.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/container/list.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_get.h"
#include "axis_utils/value/value_is.h"
#include "axis_utils/value/value_json.h"
#include "axis_utils/value/value_kv.h"

// Raw msg interface
bool axis_raw_msg_check_integrity(axis_msg_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_MSG_SIGNATURE) {
    return false;
  }
  return true;
}

void axis_raw_msg_init(axis_msg_t *self, axis_MSG_TYPE type) {
  axis_ASSERT(self, "Invalid argument.");

  axis_signature_set(&self->signature, (axis_signature_t)axis_MSG_SIGNATURE);

  self->type = type;
  axis_value_init_string(&self->name);

  axis_loc_init_empty(&self->src_loc);
  axis_list_init(&self->dest_loc);

  axis_value_init_object_with_move(&self->properties, NULL);

  axis_list_init(&self->locked_res);
}

void axis_raw_msg_deinit(axis_msg_t *self) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  axis_LOGV("Destroy c_msg %p", self);

  axis_signature_set(&self->signature, 0);
  axis_value_deinit(&self->name);

  axis_loc_deinit(&self->src_loc);
  axis_list_clear(&self->dest_loc);

  axis_list_clear(&self->locked_res);

  axis_value_deinit(&self->properties);
}

void axis_raw_msg_set_src_to_loc(axis_msg_t *self, axis_loc_t *loc) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");
  axis_loc_set_from_loc(&self->src_loc, loc);
}

void axis_msg_set_src_to_loc(axis_shared_ptr_t *self, axis_loc_t *loc) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  axis_raw_msg_set_src_to_loc(axis_shared_ptr_get_data(self), loc);
}

// The semantics of the following function is to replace the destination
// information to one which is specified through the parameters.
static bool axis_raw_msg_clear_and_set_dest(axis_msg_t *self, const char *uri,
                                           const char *graph_id,
                                           const char *extension_group_name,
                                           const char *extension_name,
                                           axis_UNUSED axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self) &&
                 (uri != NULL || extension_name != NULL),
             "Should not happen.");

  axis_list_clear(&self->dest_loc);
  axis_list_push_ptr_back(
      &self->dest_loc,
      axis_loc_create(uri, graph_id, extension_group_name, extension_name),
      (axis_ptr_listnode_destroy_func_t)axis_loc_destroy);

  return true;
}

void axis_raw_msg_add_dest(axis_msg_t *self, const char *uri,
                          const char *graph_id,
                          const char *extension_group_name,
                          const char *extension_name) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  axis_list_push_ptr_back(
      &self->dest_loc,
      axis_loc_create(uri, graph_id, extension_group_name, extension_name),
      (axis_ptr_listnode_destroy_func_t)axis_loc_destroy);
}

void axis_raw_msg_clear_dest(axis_msg_t *self) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  axis_list_clear(&self->dest_loc);
}

static void axis_raw_msg_clear_and_set_dest_from_msg_src(axis_msg_t *self,
                                                        axis_shared_ptr_t *cmd) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  axis_msg_t *raw_msg = axis_msg_get_raw_msg(cmd);

  axis_raw_msg_clear_and_set_dest(
      self, axis_string_get_raw_str(&raw_msg->src_loc.app_uri),
      axis_string_get_raw_str(&raw_msg->src_loc.graph_id),
      axis_string_get_raw_str(&raw_msg->src_loc.extension_group_name),
      axis_string_get_raw_str(&raw_msg->src_loc.extension_name), NULL);
}

void axis_msg_clear_and_set_dest_from_msg_src(axis_shared_ptr_t *self,
                                             axis_shared_ptr_t *cmd) {
  axis_ASSERT(self && axis_msg_check_integrity(self) && cmd &&
                 axis_msg_check_integrity(cmd),
             "Should not happen.");

  axis_msg_t *raw_msg = axis_msg_get_raw_msg(self);
  axis_raw_msg_clear_and_set_dest_from_msg_src(raw_msg, cmd);
}

static bool axis_raw_msg_src_is_empty(axis_msg_t *self) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");
  return axis_loc_is_empty(&self->src_loc);
}

const char *axis_raw_msg_get_first_dest_uri(axis_msg_t *self) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(axis_list_size(&self->dest_loc) == 1, "Should not happen.");

  axis_loc_t *first_loc = axis_ptr_listnode_get(axis_list_front(&self->dest_loc));
  return axis_string_get_raw_str(&first_loc->app_uri);
}

axis_loc_t *axis_raw_msg_get_first_dest_loc(axis_msg_t *self) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(axis_list_size(&self->dest_loc) >= 1, "Should not happen.");

  axis_loc_t *first_loc = axis_ptr_listnode_get(axis_list_front(&self->dest_loc));
  return first_loc;
}

bool axis_msg_check_integrity(axis_shared_ptr_t *self) {
  axis_ASSERT(self, "Should not happen.");
  axis_msg_t *raw_msg = axis_shared_ptr_get_data(self);
  if (axis_raw_msg_check_integrity(raw_msg) == false) {
    return false;
  }
  return true;
}

bool axis_msg_src_is_empty(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  return axis_raw_msg_src_is_empty(axis_msg_get_raw_msg(self));
}

const char *axis_msg_get_first_dest_uri(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  return axis_raw_msg_get_first_dest_uri(axis_msg_get_raw_msg(self));
}

static void axis_raw_msg_set_src(axis_msg_t *self, const char *uri,
                                const char *graph_id,
                                const char *extension_group_name,
                                const char *extension_name) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");
  axis_loc_set(&self->src_loc, uri, graph_id, extension_group_name,
              extension_name);
}

void axis_msg_set_src(axis_shared_ptr_t *self, const char *uri,
                     const char *graph_id, const char *extension_group_name,
                     const char *extension_name) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  axis_raw_msg_set_src(axis_msg_get_raw_msg(self), uri, graph_id,
                      extension_group_name, extension_name);
}

void axis_msg_set_src_to_app(axis_shared_ptr_t *self, axis_app_t *app) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(app && axis_app_check_integrity(app, false), "Should not happen.");

  axis_msg_set_src(self, axis_app_get_uri(app), NULL, NULL, NULL);
}

void axis_msg_set_src_to_engine(axis_shared_ptr_t *self, axis_engine_t *engine) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(engine && axis_engine_check_integrity(engine, false),
             "Should not happen.");

  axis_msg_set_src(self, axis_app_get_uri(engine->app),
                  axis_engine_get_id(engine, true), NULL, NULL);
}

void axis_msg_set_src_to_extension(axis_shared_ptr_t *self,
                                  axis_extension_t *extension) {
  axis_ASSERT(self && axis_msg_check_integrity(self) && extension &&
                 axis_extension_check_integrity(extension, true),
             "Should not happen.");

  axis_extension_group_t *extension_group =
      extension->extension_thread->extension_group;
  axis_ASSERT(
      extension_group &&
          // axis_NOLINTNEXTLINE(thread-check)
          // thread-check: we might be in other threads except extension threads
          // (ex: the JS main thread), and here, we only need to get the name of
          // the extension_group and the engine, and those pointers and the name
          // values will not be changed after they are created, and before the
          // entire engine is closed, so it's thread-safe here.
          axis_extension_group_check_integrity(extension_group, false),
      "Should not happen.");

  axis_engine_t *engine =
      extension_group->extension_thread->extension_context->engine;
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: see above.
  axis_ASSERT(engine && axis_engine_check_integrity(engine, false),
             "Should not happen.");

  axis_msg_set_src(self, axis_app_get_uri(engine->app),
                  axis_engine_get_id(engine, false),
                  axis_extension_group_get_name(extension_group, true),
                  axis_extension_get_name(extension, true));
}

void axis_msg_set_src_to_extension_group(
    axis_shared_ptr_t *self, axis_extension_group_t *extension_group) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(extension_group &&
                 axis_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  axis_engine_t *engine =
      extension_group->extension_thread->extension_context->engine;
  axis_ASSERT(engine && axis_engine_check_integrity(engine, false),
             "Should not happen.");

  axis_msg_set_src(self, axis_app_get_uri(engine->app),
                  axis_engine_get_id(engine, false),
                  axis_extension_group_get_name(extension_group, true), NULL);
}

bool axis_msg_src_uri_is_empty(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  return strlen(axis_msg_get_src_app_uri(self)) == 0;
}

bool axis_msg_src_graph_id_is_empty(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  return strlen(axis_msg_get_src_graph_id(self)) == 0;
}

void axis_msg_set_src_uri(axis_shared_ptr_t *self, const char *uri) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  axis_string_set_formatted(&(axis_msg_get_raw_msg(self)->src_loc.app_uri), "%s",
                           uri);
}

void axis_msg_set_src_uri_if_empty(axis_shared_ptr_t *self, const char *uri) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");

  if (axis_msg_src_uri_is_empty(self)) {
    axis_string_set_formatted(&(axis_msg_get_raw_msg(self)->src_loc.app_uri),
                             "%s", uri);
  }
}

void axis_msg_set_src_engine_if_unspecified(axis_shared_ptr_t *self,
                                           axis_engine_t *engine) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(engine && axis_engine_check_integrity(engine, true),
             "Invalid argument.");

  if (axis_msg_src_graph_id_is_empty(self)) {
    axis_string_copy(&(axis_msg_get_raw_msg(self)->src_loc.graph_id),
                    &engine->graph_id);
  }
}

bool axis_msg_clear_and_set_dest(axis_shared_ptr_t *self, const char *uri,
                                const char *graph_id,
                                const char *extension_group_name,
                                const char *extension_name, axis_error_t *err) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");

  return axis_raw_msg_clear_and_set_dest(axis_msg_get_raw_msg(self), uri,
                                        graph_id, extension_group_name,
                                        extension_name, err);
}

void axis_raw_msg_clear_and_set_dest_to_loc(axis_msg_t *self, axis_loc_t *loc) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self) && loc,
             "Should not happen.");

  if (!loc) {
    axis_raw_msg_clear_dest(self);
  } else {
    axis_raw_msg_clear_and_set_dest(
        self, axis_string_get_raw_str(&loc->app_uri),
        axis_string_get_raw_str(&loc->graph_id),
        axis_string_get_raw_str(&loc->extension_group_name),
        axis_string_get_raw_str(&loc->extension_name), NULL);
  }
}

void axis_msg_clear_and_set_dest_to_loc(axis_shared_ptr_t *self, axis_loc_t *loc) {
  axis_ASSERT(self && axis_msg_check_integrity(self) && loc,
             "Should not happen.");

  axis_raw_msg_clear_and_set_dest_to_loc(axis_shared_ptr_get_data(self), loc);
}

static void axis_msg_clear_dest_graph_id(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");

  axis_list_foreach (axis_msg_get_dest(self), iter) {
    axis_loc_t *loc = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(loc && axis_loc_check_integrity(loc), "Should not happen.");

    axis_string_clear(&loc->graph_id);
  }
}

void axis_msg_set_dest_engine_if_unspecified_or_predefined_graph_name(
    axis_shared_ptr_t *self, axis_engine_t *engine,
    axis_list_t *predefined_graph_infos) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(engine, "Should not happen.");

  axis_list_foreach (axis_msg_get_dest(self), iter) {
    axis_loc_t *dest_loc = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(dest_loc && axis_loc_check_integrity(dest_loc),
               "Should not happen.");

    if (axis_string_is_empty(&dest_loc->graph_id)) {
      axis_string_copy(&dest_loc->graph_id, &engine->graph_id);
    } else if (predefined_graph_infos) {
      // Otherwise, if the target_engine is one of the _singleton_ predefined
      // graph engine, and the destination graph_id is the "name" of that
      // _singleton_ predefined graph engine, then replace the destination
      // graph_id to the "graph_name" of the _singleton_ predefined graph
      // engine.

      axis_predefined_graph_info_t *singleton_predefined_graph =
          axis_predefined_graph_infos_get_singleton_by_name(
              predefined_graph_infos,
              axis_string_get_raw_str(&dest_loc->graph_id));
      if (singleton_predefined_graph && singleton_predefined_graph->engine) {
        axis_ASSERT(engine == singleton_predefined_graph->engine,
                   "Otherwise, the message should not be transferred to this "
                   "engine.");

        axis_string_copy(&dest_loc->graph_id,
                        &singleton_predefined_graph->engine->graph_id);
      }
    }
  }
}

void axis_msg_clear_and_set_dest_from_extension_info(
    axis_shared_ptr_t *self, axis_extension_info_t *extension_info) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Invalid argument.");

  axis_ASSERT(extension_info, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: The graph-related information of the extension remains
  // unchanged during the lifecycle of engine/graph, allowing safe
  // cross-thread access.
  axis_ASSERT(axis_extension_info_check_integrity(extension_info, false),
             "Invalid use of extension_info %p.", extension_info);

  axis_msg_clear_and_set_dest_to_loc(self, &extension_info->loc);
}

axis_list_t *axis_msg_get_dest(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  return &axis_msg_get_raw_msg(self)->dest_loc;
}

size_t axis_raw_msg_get_dest_cnt(axis_msg_t *self) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");
  return axis_list_size(&self->dest_loc);
}

size_t axis_msg_get_dest_cnt(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  return axis_raw_msg_get_dest_cnt(axis_shared_ptr_get_data(self));
}

const char *axis_msg_get_src_app_uri(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  return axis_string_get_raw_str(&axis_msg_get_raw_msg(self)->src_loc.app_uri);
}

const char *axis_msg_get_src_graph_id(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  return axis_string_get_raw_str(&axis_msg_get_raw_msg(self)->src_loc.graph_id);
}

void axis_msg_clear_dest(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");

  axis_list_clear(&axis_msg_get_raw_msg(self)->dest_loc);
}

axis_loc_t *axis_raw_msg_get_src_loc(axis_msg_t *self) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");
  return &self->src_loc;
}

axis_loc_t *axis_msg_get_src_loc(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  return axis_raw_msg_get_src_loc(axis_shared_ptr_get_data(self));
}

axis_loc_t *axis_msg_get_first_dest_loc(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  return axis_raw_msg_get_first_dest_loc(axis_msg_get_raw_msg(self));
}

axis_MSG_TYPE axis_msg_type_from_type_and_name_string(const char *type_str,
                                                    const char *name_str) {
  axis_MSG_TYPE msg_type = axis_MSG_TYPE_INVALID;

  if (!type_str) {
    // If the 'type' of the message is not specified, it would be a 'custom'
    // command.
    msg_type = axis_MSG_TYPE_CMD;
  } else {
    // Find the correct message type.
    for (size_t i = 0; i < axis_msg_info_size; i++) {
      if (axis_msg_info[i].msg_type_name &&
          axis_c_string_is_equal(type_str, axis_msg_info[i].msg_type_name)) {
        msg_type = (axis_MSG_TYPE)i;
        break;
      }
    }
  }

  switch (msg_type) {
    case axis_MSG_TYPE_CMD:
      axis_ASSERT(name_str, "Invalid argument.");

      // If it is a command, determine if it is a special command name to
      // identify whether it is actually a specialized message type.
      for (size_t i = 0; i < axis_msg_info_size; i++) {
        if (axis_msg_info[i].msg_unique_name &&
            axis_c_string_is_equal(name_str, axis_msg_info[i].msg_unique_name)) {
          msg_type = (axis_MSG_TYPE)i;
          break;
        }
      }
      break;

    default:
      break;
  }

  if (!(msg_type > axis_MSG_TYPE_INVALID && msg_type < axis_MSG_TYPE_LAST)) {
    return axis_MSG_TYPE_INVALID;
  }

  return msg_type;
}

axis_MSG_TYPE axis_msg_type_from_unique_name_string(const char *name_str) {
  axis_ASSERT(name_str, "Invalid argument.");

  axis_MSG_TYPE msg_type = axis_MSG_TYPE_INVALID;

  for (size_t i = 0; i < axis_msg_info_size; i++) {
    if (axis_msg_info[i].msg_unique_name &&
        axis_c_string_is_equal(name_str, axis_msg_info[i].msg_unique_name)) {
      msg_type = (axis_MSG_TYPE)i;
      break;
    }
  }

  return msg_type;
}

const char *axis_msg_type_to_string(const axis_MSG_TYPE type) {
  if (type >= axis_msg_info_size) {
    return NULL;
  }
  return axis_msg_info[type].msg_type_name;
}

static bool axis_raw_msg_get_one_field_from_json_internal(
    axis_msg_t *self, axis_msg_field_process_data_t *field, void *user_data,
    bool include_internal_field, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(field, "Should not happen.");
  axis_ASSERT(
      field->field_value && axis_value_check_integrity(field->field_value),
      "Should not happen.");

  axis_json_t *json = (axis_json_t *)user_data;
  axis_ASSERT(json, "Should not happen.");

  if (!field->is_user_defined_properties) {
    // Internal fields are uniformly stored in the "_ten" section.
    if (include_internal_field) {
      json = axis_json_object_peek_object_forcibly(json, axis_STR_UNDERLINE_TEN);
      axis_ASSERT(json, "Should not happen.");

      json = axis_json_object_peek(json, field->field_name);
      if (!json) {
        // Some fields are optional, and it is allowed for the corresponding
        // JSON block to be absent during deserialization.
        return true;
      }

      if (!axis_value_set_from_json(field->field_value, json)) {
        // If the field value cannot be set from the JSON, it means that the
        // JSON format is incorrect.
        if (err) {
          axis_error_set(err, axis_ERRNO_INVALID_JSON,
                        "Invalid JSON format for field %s.", field->field_name);
        }

        return false;
      }
    }
  } else {
    // User-defined fields are stored in the root of the JSON. The field value
    // is an object which includes all the key-value pairs.
    axis_ASSERT(axis_value_is_object(field->field_value), "Should not happen.");

    const char *key = NULL;
    axis_json_t *item = NULL;
    axis_json_object_foreach(json, key, item) {
      if (axis_c_string_is_equal(key, axis_STR_UNDERLINE_TEN)) {
        // The "aptima" section is reserved for internal usage.
        continue;
      }

      axis_value_t *value = axis_value_from_json(item);
      if (!value) {
        // If the value cannot be created from the JSON, it means that the JSON
        // format is incorrect.
        if (err) {
          axis_error_set(err, axis_ERRNO_INVALID_JSON,
                        "Invalid JSON format for field %s.", field->field_name);
        }

        return false;
      }

      // TODO(xilin): If the key is already existed, should we overwrite it?
      axis_list_push_ptr_back(
          &field->field_value->content.object, axis_value_kv_create(key, value),
          (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);
    }
  }

  // During JSON deserialization, the field value may be modified, so we set the
  // value_is_changed_after_process flag.
  field->value_is_changed_after_process = true;

  return true;
}

static bool axis_raw_msg_get_one_field_from_json(
    axis_msg_t *self, axis_msg_field_process_data_t *field, void *user_data,
    axis_error_t *err) {
  return axis_raw_msg_get_one_field_from_json_internal(self, field, user_data,
                                                      false, err);
}

bool axis_raw_msg_get_one_field_from_json_include_internal_field(
    axis_msg_t *self, axis_msg_field_process_data_t *field, void *user_data,
    axis_error_t *err) {
  return axis_raw_msg_get_one_field_from_json_internal(self, field, user_data,
                                                      true, err);
}

static bool axis_raw_msg_put_one_field_to_json_internal(
    axis_msg_t *self, axis_msg_field_process_data_t *field, void *user_data,
    bool include_internal_field, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(field, "Should not happen.");
  axis_ASSERT(
      field->field_value && axis_value_check_integrity(field->field_value),
      "Should not happen.");

  axis_json_t *json = (axis_json_t *)user_data;
  axis_ASSERT(json, "Should not happen.");

  if (!field->is_user_defined_properties) {
    // Internal fields are uniformly stored in the "_ten" section.

    if (include_internal_field) {
      json = axis_json_object_peek_object_forcibly(json, axis_STR_UNDERLINE_TEN);
      axis_ASSERT(json, "Should not happen.");

      axis_json_object_set_new(json, field->field_name,
                              axis_value_to_json(field->field_value));
    }
  } else {
    axis_ASSERT(axis_value_is_object(field->field_value), "Should not happen.");

    axis_value_object_foreach(field->field_value, iter) {
      axis_value_kv_t *kv = axis_ptr_listnode_get(iter.node);
      axis_ASSERT(kv && axis_value_kv_check_integrity(kv), "Should not happen.");

      // The User-defined fields are stored in the root of the JSON.
      axis_json_object_set_new(json, axis_string_get_raw_str(&kv->key),
                              axis_value_to_json(kv->value));
    }
  }

  // The field value is not modified during JSON serialization.
  field->value_is_changed_after_process = false;

  return true;
}

static bool axis_raw_msg_put_one_field_to_json_include_internal_field(
    axis_msg_t *self, axis_msg_field_process_data_t *field, void *user_data,
    axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(field, "Should not happen.");
  axis_ASSERT(
      field->field_value && axis_value_check_integrity(field->field_value),
      "Should not happen.");

  return axis_raw_msg_put_one_field_to_json_internal(self, field, user_data,
                                                    true, err);
}

bool axis_raw_msg_put_one_field_to_json(axis_msg_t *self,
                                       axis_msg_field_process_data_t *field,
                                       void *user_data, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(field, "Should not happen.");
  axis_ASSERT(
      field->field_value && axis_value_check_integrity(field->field_value),
      "Should not happen.");

  return axis_raw_msg_put_one_field_to_json_internal(self, field, user_data,
                                                    false, err);
}

bool axis_raw_msg_process_field(axis_msg_t *self,
                               axis_raw_msg_process_one_field_func_t cb,
                               void *user_data, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self) && cb,
             "Should not happen.");

  for (size_t i = 0; i < axis_msg_fields_info_size; ++i) {
    axis_msg_process_field_func_t process_field =
        axis_msg_fields_info[i].process_field;
    if (process_field) {
      if (!process_field(self, cb, user_data, err)) {
        return false;
      }
    }
  }

  return true;
}

static axis_json_t *axis_raw_msg_to_json(axis_msg_t *self, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");
  axis_json_t *json = axis_json_create_object();
  axis_ASSERT(json, "Should not happen.");

  bool rc = axis_raw_msg_loop_all_fields(self, axis_raw_msg_put_one_field_to_json,
                                        json, err);

  if (!rc) {
    axis_json_destroy(json);
    return NULL;
  }

  return json;
}

axis_json_t *axis_msg_to_json(axis_shared_ptr_t *self, axis_error_t *err) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");

  return axis_raw_msg_to_json(axis_msg_get_raw_msg(self), err);
}

static axis_json_t *axis_raw_msg_to_json_include_internal_field(
    axis_msg_t *self, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");
  axis_json_t *json = axis_json_create_object();
  axis_ASSERT(json, "Should not happen.");

  bool rc = axis_raw_msg_loop_all_fields(
      self, axis_raw_msg_put_one_field_to_json_include_internal_field, json,
      err);

  if (!rc) {
    axis_json_destroy(json);
    return NULL;
  }

  return json;
}

axis_json_t *axis_msg_to_json_include_internal_field(axis_shared_ptr_t *self,
                                                   axis_error_t *err) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");

  return axis_raw_msg_to_json_include_internal_field(axis_msg_get_raw_msg(self),
                                                    err);
}

void axis_raw_msg_copy_field(axis_msg_t *self, axis_msg_t *src,
                            axis_list_t *excluded_field_ids) {
  axis_ASSERT(src && axis_raw_msg_check_integrity(src), "Should not happen.");

  for (size_t i = 0; i < axis_msg_fields_info_size; ++i) {
    if (excluded_field_ids) {
      bool skip = false;

      axis_list_foreach (excluded_field_ids, iter) {
        if (axis_msg_fields_info[i].field_id ==
            axis_int32_listnode_get(iter.node)) {
          skip = true;
          break;
        }
      }

      if (skip) {
        continue;
      }
    }

    axis_msg_copy_field_func_t copy_field = axis_msg_fields_info[i].copy_field;
    if (copy_field) {
      copy_field(self, src, excluded_field_ids);
    }
  }
}

static bool axis_raw_msg_init_from_json(axis_msg_t *self, axis_json_t *json,
                                       axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(json && axis_json_check_integrity(json), "Should not happen.");

  bool rc = axis_raw_msg_loop_all_fields(
      self, axis_raw_msg_get_one_field_from_json, json, err);

  if (!rc) {
    if (err) {
      axis_error_set(
          err, axis_ERRNO_INVALID_JSON,
          "Failed to init a message from json, because the fields are "
          "incorrect.");
    }
    return false;
  }

  return true;
}

bool axis_msg_from_json(axis_shared_ptr_t *self, axis_json_t *json,
                       axis_error_t *err) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(json && axis_json_check_integrity(json), "Should not happen.");

  return axis_raw_msg_init_from_json(axis_shared_ptr_get_data(self), json, err);
}

axis_shared_ptr_t *axis_msg_clone(axis_shared_ptr_t *self,
                                axis_list_t *excluded_field_ids) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");

  axis_msg_t *raw_result = NULL;
  axis_shared_ptr_t *result = NULL;

  axis_raw_msg_clone_func_t clone = axis_msg_info[axis_msg_get_type(self)].clone;
  if (clone) {
    raw_result = clone(axis_msg_get_raw_msg(self), excluded_field_ids);
    if (raw_result) {
      result = axis_shared_ptr_create(raw_result, axis_raw_msg_destroy);

      if (axis_msg_is_cmd_and_result(self)) {
        // Create a relationship between the newly generated message and the
        // original message.
        axis_raw_cmd_base_save_cmd_id_to_parent_cmd_id(
            (axis_cmd_base_t *)raw_result);

        // The rule is simple:
        //
        // If generating/cloning a new message, that message will contain a new
        // command ID. If don't want a new command ID, then don't generate/clone
        // a new message.
        axis_raw_cmd_base_gen_new_cmd_id_forcibly((axis_cmd_base_t *)raw_result);
      }
    }
  }

  return result;
}

axis_shared_ptr_t *axis_msg_create_from_msg_type(axis_MSG_TYPE msg_type) {
  switch (msg_type) {
    case axis_MSG_TYPE_CMD_CLOSE_APP:
      return axis_cmd_close_app_create();
    case axis_MSG_TYPE_CMD:
      return axis_cmd_custom_create_empty();
    case axis_MSG_TYPE_CMD_START_GRAPH:
      return axis_cmd_start_graph_create();
    case axis_MSG_TYPE_CMD_STOP_GRAPH:
      return axis_cmd_stop_graph_create();
    case axis_MSG_TYPE_CMD_TIMEOUT:
      return axis_cmd_timeout_create(0);
    case axis_MSG_TYPE_CMD_RESULT:
      return axis_cmd_result_create_from_cmd(axis_STATUS_CODE_OK, NULL);
    case axis_MSG_TYPE_DATA:
      return axis_data_create_empty();
    case axis_MSG_TYPE_AUDIO_FRAME:
      return axis_audio_frame_create_empty();
    case axis_MSG_TYPE_VIDEO_FRAME:
      return axis_video_frame_create_empty();
    default:
      return NULL;
  }
}

bool axis_msg_type_to_handle_when_closing(axis_shared_ptr_t *msg) {
  axis_ASSERT(msg && axis_msg_check_integrity(msg), "Should not happen.");

  switch (axis_msg_get_type(msg)) {
    case axis_MSG_TYPE_CMD_RESULT:
      return true;
    default:
      return false;
  }
}

const char *axis_raw_msg_get_type_string(axis_msg_t *self) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");
  return axis_msg_type_to_string(axis_raw_msg_get_type(self));
}

const char *axis_msg_get_type_string(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  return axis_raw_msg_get_type_string(axis_shared_ptr_get_data(self));
}

axis_MSG_TYPE axis_msg_type_from_type_string(const char *type_str) {
  axis_MSG_TYPE msg_type = axis_MSG_TYPE_INVALID;

  // Find the correct message type.
  for (size_t i = 0; i < axis_msg_info_size; i++) {
    if (axis_msg_info[i].msg_type_name &&
        axis_c_string_is_equal(type_str, axis_msg_info[i].msg_type_name)) {
      msg_type = (axis_MSG_TYPE)i;
      break;
    }
  }

  if (!(msg_type > axis_MSG_TYPE_INVALID && msg_type < axis_MSG_TYPE_LAST)) {
    return axis_MSG_TYPE_INVALID;
  }

  return msg_type;
}

void axis_msg_correct_dest(axis_shared_ptr_t *msg, axis_engine_t *engine) {
  axis_ASSERT(msg && axis_msg_check_integrity(msg), "Should not happen.");
  axis_ASSERT(engine && axis_engine_check_integrity(engine, false),
             "Should not happen.");

  const char *app_uri = axis_app_get_uri(engine->app);

  axis_msg_t *raw_msg = axis_msg_get_raw_msg(msg);
  axis_list_foreach (&raw_msg->dest_loc, iter) {
    axis_loc_t *dest_loc = axis_ptr_listnode_get(iter.node);

    bool is_local_app = false;

    if (axis_string_is_equal_c_str(&dest_loc->app_uri, app_uri)) {
      is_local_app = true;
    } else if (axis_string_is_equal_c_str(&dest_loc->app_uri,
                                         axis_STR_LOCALHOST)) {
      // Extension could use 'localhost' to indicate that the destination of the
      // message is to the local app, therefore, the APTIMA runtime needs to
      // 'correct' the real destination location from 'localhost' to the real
      // URI of the app.

      axis_string_set_from_c_str(&dest_loc->app_uri, app_uri, strlen(app_uri));
      is_local_app = true;
    }

    if (is_local_app) {
      // If the destination location is the local app, extension could omit the
      // engine graph ID in the message, therefore, APTIMA runtime needs to 'add'
      // this information back to the message, so that the APTIMA runtime could
      // know how to transit it after the APTIMA runtime receives that message.
      //
      // However, some command, for example, the 'start_graph' command, is a
      // special case, because this kind of command will be handled by the app,
      // rather than by any engines, therefore, we should not set the
      // destination graph_id in the case of such command.
      if (axis_msg_get_type(msg) == axis_MSG_TYPE_CMD_START_GRAPH) {
        axis_msg_clear_dest_graph_id(msg);
      } else {
        axis_msg_set_dest_engine_if_unspecified_or_predefined_graph_name(
            msg, engine, &engine->app->predefined_graph_infos);
      }
    }
  }

  // In 'start_graph' command, the uri of an extension could be 'localhost',
  // which indicates that the extension will be located in the local app.
  // Therefore, the APTIMA runtime needs to 'correct' the real app uri from
  // 'localhost' to the real uri of the app.
  if (axis_msg_get_type(msg) == axis_MSG_TYPE_CMD_START_GRAPH) {
    axis_list_t *extensions_info = axis_cmd_start_graph_get_extensions_info(msg);
    axis_list_foreach (extensions_info, iter) {
      axis_extension_info_t *extension_info =
          axis_shared_ptr_get_data(axis_smart_ptr_listnode_get(iter.node));
      axis_extension_info_translate_localhost_to_app_uri(extension_info,
                                                        app_uri);
    }
  }
}

static bool axis_raw_msg_dump_internal(axis_msg_t *msg, axis_error_t *err,
                                      const char *fmt, va_list ap) {
  axis_ASSERT(msg && axis_raw_msg_check_integrity(msg), "Should not happen.");

  axis_json_t *msg_json = axis_raw_msg_to_json_include_internal_field(msg, err);
  axis_ASSERT(msg_json, "Failed to convert msg type(%s), name(%s) to JSON.",
             axis_msg_type_to_string(msg->type),
             axis_value_peek_raw_str(&msg->name, err));
  if (!msg_json) {
    return false;
  }

  bool must_free = false;
  const char *msg_json_str = axis_json_to_string(msg_json, NULL, &must_free);

  axis_string_t description;
  axis_string_init(&description);
  axis_string_append_from_va_list(&description, fmt, ap);

  const char *p = axis_string_get_raw_str(&description);

  axis_string_t dump_str;
  axis_string_init(&dump_str);

  while (*p) {
    if ('^' != *p) {
      axis_string_append_formatted(&dump_str, "%c", *p++);
      continue;
    }

    switch (*++p) {
      // The message content.
      case 'm':
        axis_string_append_formatted(&dump_str, "%s", msg_json_str);
        p++;
        break;

        // If next char can't match any mode, keep it.
      default:
        axis_string_append_formatted(&dump_str, "%c", *p++);
        break;
    }
  }

  axis_LOGE("%s", axis_string_get_raw_str(&dump_str));

  axis_string_deinit(&dump_str);
  axis_string_deinit(&description);

  if (must_free) {
    axis_FREE(msg_json_str);
  }
  axis_json_destroy(msg_json);

  return true;
}

bool axis_raw_msg_dump(axis_msg_t *msg, axis_error_t *err, const char *fmt, ...) {
  axis_ASSERT(msg && axis_raw_msg_check_integrity(msg), "Should not happen.");

  va_list ap;
  va_start(ap, fmt);
  bool rc = axis_raw_msg_dump_internal(msg, err, fmt, ap);
  va_end(ap);

  return rc;
}

bool axis_msg_dump(axis_shared_ptr_t *msg, axis_error_t *err, const char *fmt,
                  ...) {
  axis_ASSERT(msg && axis_msg_check_integrity(msg), "Should not happen.");

  va_list ap;
  va_start(ap, fmt);
  bool rc =
      axis_raw_msg_dump_internal(axis_shared_ptr_get_data(msg), err, fmt, ap);
  va_end(ap);

  return rc;
}

/**
 * @note The ownership of @a value is transferred to the @a msg.
 */
static bool axis_raw_msg_set_property(axis_msg_t *self, const char *path,
                                     axis_value_t *value, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(value && axis_value_check_integrity(value), "Should not happen.");

  if (!path || !strlen(path)) {
    // If the path is empty, clear and set all properties.
    axis_value_deinit(&self->properties);
    bool result = axis_value_init_object_with_move(&self->properties,
                                                  axis_value_peek_object(value));
    if (result) {
      // The contents of `value` have been completely moved into `properties`,
      // so the outer `value` wrapper is removed to avoid memory leaks.
      axis_value_destroy(value);
    }
    return result;
  }

  bool success = true;

  axis_list_t paths = axis_LIST_INIT_VAL;
  if (!axis_value_path_parse(path, &paths, NULL)) {
    goto done;
  }

  bool in_axis_namespace = false;

  axis_list_foreach (&paths, item_iter) {
    axis_value_path_item_t *item = axis_ptr_listnode_get(item_iter.node);
    axis_ASSERT(item, "Invalid argument.");

    switch (item->type) {
      case axis_VALUE_PATH_ITEM_TYPE_OBJECT_ITEM: {
        if ((item_iter.index == 0) &&
            !strcmp(axis_STR_UNDERLINE_TEN,
                    axis_string_get_raw_str(&item->obj_item_str))) {
          // It is the '_ten::' namespace.
          in_axis_namespace = true;

          // Remove the '_ten' namespace path part.
          axis_list_remove_front(&paths);

          axis_raw_msg_set_axis_property_func_t set_axis_property =
              axis_msg_info[axis_raw_msg_get_type(self)].set_axis_property;
          if (set_axis_property) {
            success = set_axis_property(self, &paths, value, err);
          }

          axis_value_destroy(value);
        }
        break;
      }

      default:
        break;
    }
  }

  if (!in_axis_namespace) {
    success = axis_value_set_from_path_list_with_move(&self->properties, &paths,
                                                     value, err);
  }

done:
  axis_list_clear(&paths);

  return success;
}

bool axis_msg_set_property(axis_shared_ptr_t *self, const char *path,
                          axis_value_t *value, axis_error_t *err) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(value && axis_value_check_integrity(value), "Should not happen.");

  return axis_raw_msg_set_property(axis_shared_ptr_get_data(self), path, value,
                                  err);
}

axis_value_t *axis_raw_msg_peek_property(axis_msg_t *self, const char *path,
                                       axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  if (!path || !strlen(path)) {
    // If the path is empty, return all properties.
    return &self->properties;
  }

  axis_value_t *result = NULL;

  axis_list_t paths = axis_LIST_INIT_VAL;
  if (!axis_value_path_parse(path, &paths, NULL)) {
    goto done;
  }

  bool in_axis_namespace = false;

  axis_list_foreach (&paths, item_iter) {
    axis_value_path_item_t *item = axis_ptr_listnode_get(item_iter.node);
    axis_ASSERT(item, "Invalid argument.");

    switch (item->type) {
      case axis_VALUE_PATH_ITEM_TYPE_OBJECT_ITEM: {
        if ((item_iter.index == 0) &&
            !strcmp(axis_STR_UNDERLINE_TEN,
                    axis_string_get_raw_str(&item->obj_item_str))) {
          // It is the '_ten::' namespace.
          in_axis_namespace = true;

          // Remove the '_ten' namespace path part.
          axis_list_remove_front(&paths);

          axis_raw_msg_peek_axis_property_func_t peek_axis_property =
              axis_msg_info[axis_raw_msg_get_type(self)].peek_axis_property;
          if (peek_axis_property) {
            result = peek_axis_property(self, &paths, err);
          }
        }
        break;
      }

      default:
        break;
    }
  }

  if (!in_axis_namespace) {
    result = axis_value_peek_from_path(&self->properties, path, err);
  }

done:
  axis_list_clear(&paths);

  return result;
}

axis_value_t *axis_msg_peek_property(axis_shared_ptr_t *self, const char *path,
                                   axis_error_t *err) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");

  return axis_raw_msg_peek_property(axis_msg_get_raw_msg(self), path, err);
}

static bool axis_raw_msg_has_locked_res(axis_msg_t *self) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");
  return !axis_list_is_empty(&self->locked_res);
}

bool axis_msg_has_locked_res(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  return axis_raw_msg_has_locked_res(axis_shared_ptr_get_data(self));
}

static const char *axis_raw_msg_get_name(axis_msg_t *self) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");
  return axis_value_peek_raw_str(&self->name, NULL);
}

const char *axis_msg_get_name(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");

  axis_msg_t *raw_msg = axis_msg_get_raw_msg(self);
  axis_ASSERT(raw_msg, "Should not happen.");

  return axis_raw_msg_get_name(raw_msg);
}

bool axis_raw_msg_set_name_with_len(axis_msg_t *self, const char *msg_name,
                                   size_t msg_name_len, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  if (msg_name == NULL) {
    if (err) {
      axis_error_set(err, axis_ERRNO_GENERIC, "%s",
                    "Failed to set message name to an empty string.");
    }
    return false;
  }

  axis_string_set_formatted(axis_value_peek_string(&self->name), "%.*s",
                           msg_name_len, msg_name);
  return true;
}

bool axis_raw_msg_set_name(axis_msg_t *self, const char *msg_name,
                          axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");
  return axis_raw_msg_set_name_with_len(self, msg_name, strlen(msg_name), err);
}

bool axis_msg_set_name_with_len(axis_shared_ptr_t *self, const char *msg_name,
                               size_t msg_name_len, axis_error_t *err) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  return axis_raw_msg_set_name_with_len(axis_shared_ptr_get_data(self), msg_name,
                                       msg_name_len, err);
}

bool axis_msg_set_name(axis_shared_ptr_t *self, const char *msg_name,
                      axis_error_t *err) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  return axis_raw_msg_set_name(axis_shared_ptr_get_data(self), msg_name, err);
}

bool axis_raw_msg_validate_schema(axis_msg_t *self,
                                 axis_schema_store_t *schema_store,
                                 bool is_msg_out, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Invalid argument.");
  axis_ASSERT(schema_store && axis_schema_store_check_integrity(schema_store),
             "Invalid argument.");
  axis_ASSERT(err && axis_error_check_integrity(err), "Invalid argument.");

  const char *msg_name = axis_raw_msg_get_name(self);
  if (axis_raw_msg_is_cmd_and_result(self)) {
    axis_ASSERT(msg_name && strlen(msg_name), "Should not happen.");
  }

  axis_msg_schema_t *schema = axis_schema_store_get_msg_schema(
      schema_store, axis_raw_msg_get_type(self), msg_name, is_msg_out);
  if (!schema) {
    return true;
  }

  if (!axis_msg_schema_adjust_properties(schema, &self->properties, err)) {
    return false;
  }

  if (!axis_msg_schema_validate_properties(schema, &self->properties, err)) {
    return false;
  }

  return true;
}

bool axis_msg_validate_schema(axis_shared_ptr_t *self,
                             axis_schema_store_t *schema_store, bool is_msg_out,
                             axis_error_t *err) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(schema_store && axis_schema_store_check_integrity(schema_store),
             "Should not happen.");
  axis_ASSERT(err && axis_error_check_integrity(err), "Invalid argument.");

  axis_msg_t *msg = axis_shared_ptr_get_data(self);
  axis_raw_msg_validate_schema_func_t validate_schema =
      axis_msg_info[axis_raw_msg_get_type(msg)].validate_schema;
  if (validate_schema) {
    return validate_schema(msg, schema_store, is_msg_out, err);
  }

  return true;
}

axis_MSG_TYPE axis_msg_get_type(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  return axis_raw_msg_get_type(axis_msg_get_raw_msg(self));
}
