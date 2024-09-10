//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/msg/msg.h"

#include <string.h>

#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/app/predefined_graph.h"
#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/common/loc.h"
#include "include_internal/ten_runtime/engine/engine.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension/extension_info/extension_info.h"
#include "include_internal/ten_runtime/extension_context/extension_context.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/extension_store/extension_store.h"
#include "include_internal/ten_runtime/extension_thread/extension_thread.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/ten_runtime/msg/field/field_info.h"
#include "include_internal/ten_runtime/msg/msg_info.h"
#include "include_internal/ten_runtime/schema_store/msg.h"
#include "include_internal/ten_utils/value/value_path.h"
#include "ten_runtime/app/app.h"
#include "ten_runtime/common/errno.h"
#include "ten_runtime/msg/audio_frame/audio_frame.h"
#include "ten_runtime/msg/cmd/close_app/cmd.h"
#include "ten_runtime/msg/cmd_result/cmd_result.h"
#include "ten_runtime/msg/data/data.h"
#include "ten_runtime/msg/msg.h"
#include "ten_runtime/msg/video_frame/video_frame.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/container/list.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/log/log.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/memory.h"
#include "ten_utils/value/value.h"

// Raw msg interface
bool ten_raw_msg_check_integrity(ten_msg_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  if (ten_signature_get(&self->signature) !=
      (ten_signature_t)TEN_MSG_SIGNATURE) {
    return false;
  }
  return true;
}

void ten_raw_msg_init(ten_msg_t *self, TEN_MSG_TYPE type) {
  TEN_ASSERT(self, "Invalid argument.");

  ten_signature_set(&self->signature, (ten_signature_t)TEN_MSG_SIGNATURE);

  self->type = type;
  ten_string_init(&self->name);

  ten_loc_init_empty(&self->src_loc);
  ten_list_init(&self->dest_loc);

  ten_value_init_object_with_move(&self->properties, NULL);

  ten_list_init(&self->locked_res);
}

void ten_raw_msg_deinit(ten_msg_t *self) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");

  TEN_LOGV("Destroy c_msg %p", self);

  ten_signature_set(&self->signature, 0);
  ten_string_deinit(&self->name);

  ten_loc_deinit(&self->src_loc);
  ten_list_clear(&self->dest_loc);

  ten_list_clear(&self->locked_res);

  ten_value_deinit(&self->properties);
}

void ten_raw_msg_set_src_to_loc(ten_msg_t *self, ten_loc_t *loc) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");
  ten_loc_init_from_loc(&self->src_loc, loc);
}

void ten_msg_set_src_to_loc(ten_shared_ptr_t *self, ten_loc_t *loc) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");
  ten_raw_msg_set_src_to_loc(ten_shared_ptr_get_data(self), loc);
}

// The semantics of the following function is to replace the destination
// information to one which is specified through the parameters.
static bool ten_raw_msg_clear_and_set_dest(ten_msg_t *self, const char *uri,
                                           const char *graph_name,
                                           const char *extension_group_name,
                                           const char *extension_name,
                                           ten_extension_t *extension,
                                           TEN_UNUSED ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) &&
                 (uri != NULL || extension_name != NULL),
             "Should not happen.");

  ten_list_clear(&self->dest_loc);
  ten_list_push_ptr_back(&self->dest_loc,
                         ten_loc_create(uri, graph_name, extension_group_name,
                                        extension_name, extension),
                         (ten_ptr_listnode_destroy_func_t)ten_loc_destroy);

  return true;
}

void ten_raw_msg_add_dest(ten_msg_t *self, const char *uri,
                          const char *graph_name,
                          const char *extension_group_name,
                          const char *extension_name,
                          ten_extension_t *extension) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");

  ten_list_push_ptr_back(&self->dest_loc,
                         ten_loc_create(uri, graph_name, extension_group_name,
                                        extension_name, extension),
                         (ten_ptr_listnode_destroy_func_t)ten_loc_destroy);
}

void ten_raw_msg_clear_dest(ten_msg_t *self) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");

  ten_list_clear(&self->dest_loc);
}

static void ten_raw_msg_clear_and_set_dest_from_msg_src(ten_msg_t *self,
                                                        ten_shared_ptr_t *cmd) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");

  ten_msg_t *raw_msg = ten_msg_get_raw_msg(cmd);

  ten_raw_msg_clear_and_set_dest(
      self, ten_string_get_raw_str(&raw_msg->src_loc.app_uri),
      ten_string_get_raw_str(&raw_msg->src_loc.graph_name),
      ten_string_get_raw_str(&raw_msg->src_loc.extension_group_name),
      ten_string_get_raw_str(&raw_msg->src_loc.extension_name),
      raw_msg->src_loc.extension, NULL);
}

void ten_msg_clear_and_set_dest_from_msg_src(ten_shared_ptr_t *self,
                                             ten_shared_ptr_t *cmd) {
  TEN_ASSERT(self && ten_msg_check_integrity(self) && cmd &&
                 ten_msg_check_integrity(cmd),
             "Should not happen.");

  ten_msg_t *raw_msg = ten_msg_get_raw_msg(self);
  ten_raw_msg_clear_and_set_dest_from_msg_src(raw_msg, cmd);
}

static bool ten_raw_msg_src_is_empty(ten_msg_t *self) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");
  return ten_loc_is_empty(&self->src_loc);
}

const char *ten_raw_msg_get_first_dest_uri(ten_msg_t *self) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(ten_list_size(&self->dest_loc) == 1, "Should not happen.");

  ten_loc_t *first_loc = ten_ptr_listnode_get(ten_list_front(&self->dest_loc));
  return ten_string_get_raw_str(&first_loc->app_uri);
}

ten_loc_t *ten_raw_msg_get_first_dest_loc(ten_msg_t *self) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(ten_list_size(&self->dest_loc) >= 1, "Should not happen.");

  ten_loc_t *first_loc = ten_ptr_listnode_get(ten_list_front(&self->dest_loc));
  return first_loc;
}

bool ten_msg_check_integrity(ten_shared_ptr_t *self) {
  TEN_ASSERT(self, "Should not happen.");
  ten_msg_t *raw_msg = ten_shared_ptr_get_data(self);
  if (ten_raw_msg_check_integrity(raw_msg) == false) {
    return false;
  }
  return true;
}

bool ten_msg_src_is_empty(ten_shared_ptr_t *self) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");
  return ten_raw_msg_src_is_empty(ten_msg_get_raw_msg(self));
}

const char *ten_msg_get_first_dest_uri(ten_shared_ptr_t *self) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");
  return ten_raw_msg_get_first_dest_uri(ten_msg_get_raw_msg(self));
}

void ten_raw_msg_set_src(ten_msg_t *self, const char *uri,
                         const char *graph_name,
                         const char *extension_group_name,
                         const char *extension_name,
                         ten_extension_t *extension) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");
  ten_loc_set(&self->src_loc, uri, graph_name, extension_group_name,
              extension_name, extension);
}

void ten_msg_set_src(ten_shared_ptr_t *self, const char *uri,
                     const char *graph_name, const char *extension_group_name,
                     const char *extension_name, ten_extension_t *extension) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");
  ten_raw_msg_set_src(ten_msg_get_raw_msg(self), uri, graph_name,
                      extension_group_name, extension_name, extension);
}

void ten_msg_set_src_to_app(ten_shared_ptr_t *self, ten_app_t *app) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(app && ten_app_check_integrity(app, false), "Should not happen.");

  ten_msg_set_src(self, ten_string_get_raw_str(ten_app_get_uri(app)), NULL,
                  NULL, NULL, NULL);
}

void ten_msg_set_src_to_engine(ten_shared_ptr_t *self, ten_engine_t *engine) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(engine && ten_engine_check_integrity(engine, false),
             "Should not happen.");

  ten_msg_set_src(self, ten_string_get_raw_str(ten_app_get_uri(engine->app)),
                  ten_string_get_raw_str(&engine->graph_name), NULL, NULL,
                  NULL);
}

void ten_msg_set_src_to_extension(ten_shared_ptr_t *self,
                                  ten_extension_t *extension) {
  TEN_ASSERT(self && ten_msg_check_integrity(self) && extension &&
                 ten_extension_check_integrity(extension, true),
             "Should not happen.");

  ten_extension_group_t *extension_group =
      extension->extension_thread->extension_group;
  TEN_ASSERT(
      extension_group &&
          // TEN_NOLINTNEXTLINE(thread-check)
          // thread-check: we might be in other threads except extension threads
          // (ex: the JS main thread), and here, we only need to get the name of
          // the extension_group and the engine, and those pointers and the name
          // values will not be changed after they are created, and before the
          // entire engine is closed, so it's thread-safe here.
          ten_extension_group_check_integrity(extension_group, false),
      "Should not happen.");

  ten_engine_t *engine =
      extension_group->extension_thread->extension_context->engine;
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: see above.
  TEN_ASSERT(engine && ten_engine_check_integrity(engine, false),
             "Should not happen.");

  ten_msg_set_src(self, ten_string_get_raw_str(ten_app_get_uri(engine->app)),
                  ten_string_get_raw_str(&engine->graph_name),
                  ten_string_get_raw_str(&extension_group->name),
                  ten_string_get_raw_str(&extension->name), extension);
}

void ten_msg_set_src_to_extension_group(
    ten_shared_ptr_t *self, ten_extension_group_t *extension_group) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(extension_group &&
                 ten_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  ten_engine_t *engine =
      extension_group->extension_thread->extension_context->engine;
  TEN_ASSERT(engine && ten_engine_check_integrity(engine, false),
             "Should not happen.");

  ten_msg_set_src(self, ten_string_get_raw_str(ten_app_get_uri(engine->app)),
                  ten_string_get_raw_str(&engine->graph_name),
                  ten_string_get_raw_str(&extension_group->name), NULL, NULL);
}

bool ten_msg_src_uri_is_empty(ten_shared_ptr_t *self) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");
  return strlen(ten_msg_get_src_app_uri(self)) == 0;
}

bool ten_msg_src_graph_name_is_empty(ten_shared_ptr_t *self) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");
  return strlen(ten_msg_get_src_graph_name(self)) == 0;
}

void ten_msg_set_src_uri(ten_shared_ptr_t *self, const char *uri) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");
  ten_string_set_formatted(&(ten_msg_get_raw_msg(self)->src_loc.app_uri), "%s",
                           uri);
}

void ten_msg_set_src_uri_if_empty(ten_shared_ptr_t *self, const char *uri) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");

  if (ten_msg_src_uri_is_empty(self)) {
    ten_string_set_formatted(&(ten_msg_get_raw_msg(self)->src_loc.app_uri),
                             "%s", uri);
  }
}

void ten_msg_set_src_engine_if_unspecified(ten_shared_ptr_t *self,
                                           ten_engine_t *engine) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(engine && ten_engine_check_integrity(engine, true),
             "Invalid argument.");

  if (ten_msg_src_graph_name_is_empty(self)) {
    ten_string_copy(&(ten_msg_get_raw_msg(self)->src_loc.graph_name),
                    &engine->graph_name);
  }
}

bool ten_msg_clear_and_set_dest(ten_shared_ptr_t *self, const char *uri,
                                const char *graph_name,
                                const char *extension_group_name,
                                const char *extension_name,
                                ten_extension_t *extension, ten_error_t *err) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");

  return ten_raw_msg_clear_and_set_dest(ten_msg_get_raw_msg(self), uri,
                                        graph_name, extension_group_name,
                                        extension_name, extension, err);
}

void ten_raw_msg_clear_and_set_dest_to_loc(ten_msg_t *self, ten_loc_t *loc) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) && loc,
             "Should not happen.");

  if (!loc) {
    ten_raw_msg_clear_dest(self);
  } else {
    ten_raw_msg_clear_and_set_dest(
        self, ten_string_get_raw_str(&loc->app_uri),
        ten_string_get_raw_str(&loc->graph_name),
        ten_string_get_raw_str(&loc->extension_group_name),
        ten_string_get_raw_str(&loc->extension_name), loc->extension, NULL);
  }
}

void ten_msg_clear_and_set_dest_to_loc(ten_shared_ptr_t *self, ten_loc_t *loc) {
  TEN_ASSERT(self && ten_msg_check_integrity(self) && loc,
             "Should not happen.");

  ten_raw_msg_clear_and_set_dest_to_loc(ten_shared_ptr_get_data(self), loc);
}

static void ten_msg_clear_dest_graph_name(ten_shared_ptr_t *self) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");

  ten_list_foreach (ten_msg_get_dest(self), iter) {
    ten_loc_t *loc = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(loc && ten_loc_check_integrity(loc), "Should not happen.");

    ten_string_clear(&loc->graph_name);
  }
}

void ten_msg_set_dest_engine_if_unspecified_or_predefined_graph_name(
    ten_shared_ptr_t *self, ten_engine_t *engine,
    ten_list_t *predefined_graph_infos) {
  TEN_ASSERT(self && ten_msg_check_integrity(self) && engine,
             "Should not happen.");

  ten_list_foreach (ten_msg_get_dest(self), iter) {
    ten_loc_t *loc = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(loc && ten_loc_check_integrity(loc), "Should not happen.");

    if (ten_string_is_empty(&loc->graph_name)) {
      ten_string_copy(&loc->graph_name, &engine->graph_name);
    } else if (predefined_graph_infos) {
      // Otherwise, if the target_engine is one of the predefined graph engine,
      // and the destination graph_name is the "name" of that predefined graph
      // engine, then replace the destination graph_name to the "graph_name" of
      // the predefined graph engine.

      ten_list_foreach (predefined_graph_infos, iter) {
        ten_predefined_graph_info_t *predefined_graph_info =
            (ten_predefined_graph_info_t *)ten_ptr_listnode_get(iter.node);

        // If the 'graph_name' is the name of one of the predefined graph
        // engine, we replace it to the graph_name of the predefined graph
        // engine.
        if (ten_string_is_equal(&loc->graph_name,
                                &predefined_graph_info->name)) {
          // If the predefined graph engine is not started yet, this command
          // will trigger the start of the predefined graph engine.
          if (predefined_graph_info->engine == NULL) {
            break;
          }

          TEN_ASSERT(engine == predefined_graph_info->engine,
                     "Otherwise, the message should not be transferred to this "
                     "engine.");

          ten_string_copy(&loc->graph_name,
                          &predefined_graph_info->engine->graph_name);
          break;
        }
      }
    }
  }
}

void ten_msg_clear_and_set_dest_to_extension(ten_shared_ptr_t *self,
                                             ten_extension_t *extension) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Invalid argument.");

  TEN_ASSERT(extension, "Invalid argument.");
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: The graph-related information of the extension remains
  // unchanged during the lifecycle of engine/graph, allowing safe
  // cross-thread access.
  TEN_ASSERT(ten_extension_check_integrity(extension, false),
             "Invalid use of extension %p.", extension);

  ten_msg_clear_and_set_dest(
      self,
      ten_string_get_raw_str(
          ten_app_get_uri(extension->extension_context->engine->app)),
      ten_string_get_raw_str(&extension->extension_context->engine->graph_name),
      ten_string_get_raw_str(
          &extension->extension_thread->extension_group->name),
      ten_string_get_raw_str(&extension->name),
      // Remember extension pointer for faster lookup in the future.
      extension, NULL);
}

void ten_msg_clear_and_set_dest_from_extension_info(
    ten_shared_ptr_t *self, ten_extension_info_t *extension_info) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Invalid argument.");

  TEN_ASSERT(extension_info, "Invalid argument.");
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: The graph-related information of the extension remains
  // unchanged during the lifecycle of engine/graph, allowing safe
  // cross-thread access.
  TEN_ASSERT(ten_extension_info_check_integrity(extension_info, false),
             "Invalid use of extension_info %p.", extension_info);

  TEN_ASSERT(extension_info->loc.extension == NULL, "Should not happen.");

  ten_msg_clear_and_set_dest_to_loc(self, &extension_info->loc);
}

ten_list_t *ten_msg_get_dest(ten_shared_ptr_t *self) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");
  return &ten_msg_get_raw_msg(self)->dest_loc;
}

size_t ten_raw_msg_get_dest_cnt(ten_msg_t *self) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");
  return ten_list_size(&self->dest_loc);
}

size_t ten_msg_get_dest_cnt(ten_shared_ptr_t *self) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");
  return ten_raw_msg_get_dest_cnt(ten_shared_ptr_get_data(self));
}

const char *ten_msg_get_src_app_uri(ten_shared_ptr_t *self) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");
  return ten_string_get_raw_str(&ten_msg_get_raw_msg(self)->src_loc.app_uri);
}

const char *ten_msg_get_src_graph_name(ten_shared_ptr_t *self) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");
  return ten_string_get_raw_str(&ten_msg_get_raw_msg(self)->src_loc.graph_name);
}

// This hack is only used by msgpack when serializing/deserializing the connect
// command. Eventually, we should remove this hack.
static void ten_msg_clear_dest_msgpack_serialization_hack(
    ten_shared_ptr_t *self) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");

  // If this message contains a JSON, remove the 'dest' information from the
  // json, too.
  ten_value_t *value =
      ten_msg_peek_property(self, TEN_STR_MSGPACK_SERIALIZATION_HACK, NULL);
  if (value) {
    TEN_ASSERT(ten_value_is_string(value), "Should not happen.");

    ten_json_t *json = ten_json_from_string(ten_value_peek_c_str(value), NULL);
    TEN_ASSERT(ten_json_check_integrity(json), "Should not happen.");

    ten_json_object_del(json, TEN_STR_DEST);
    TEN_ASSERT(ten_json_check_integrity(json), "Should not happen.");

    bool must_free = false;
    const char *json_str = ten_json_to_string(json, NULL, &must_free);
    ten_value_reset_to_string_with_size(value, json_str, strlen(json_str));

    if (must_free) {
      TEN_FREE(json_str);
    }
    ten_json_destroy(json);
  }
}

void ten_msg_clear_dest(ten_shared_ptr_t *self) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");

  ten_list_clear(&ten_msg_get_raw_msg(self)->dest_loc);

  ten_msg_clear_dest_msgpack_serialization_hack(self);
}

ten_loc_t *ten_raw_msg_get_src_loc(ten_msg_t *self) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");
  return &self->src_loc;
}

ten_loc_t *ten_msg_get_src_loc(ten_shared_ptr_t *self) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");
  return ten_raw_msg_get_src_loc(ten_shared_ptr_get_data(self));
}

ten_loc_t *ten_msg_get_first_dest_loc(ten_shared_ptr_t *self) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");
  return ten_raw_msg_get_first_dest_loc(ten_msg_get_raw_msg(self));
}

// TODO(Wei): Can not set the extension_or_extension_info in ten_loc_t when the
// graph can be changed.
ten_extension_t *ten_msg_try_to_find_dest_extension_in_fast_path(
    ten_shared_ptr_t *msg) {
  TEN_ASSERT(msg && ten_msg_get_dest_cnt(msg) == 1, "Invalid argument.");

  ten_loc_t *dest_loc = ten_msg_get_first_dest_loc(msg);
  TEN_ASSERT(dest_loc, "Should not happen.");

  return dest_loc->extension ? dest_loc->extension : NULL;
}

TEN_MSG_TYPE ten_msg_type_from_type_and_name_string(const char *type_str,
                                                    const char *name_str) {
  TEN_MSG_TYPE msg_type = TEN_MSG_TYPE_INVALID;

  if (!type_str) {
    // If the 'type' of the message is not specified, it would be a 'custom'
    // command.
    msg_type = TEN_MSG_TYPE_CMD;
  } else {
    // Find the correct message type.
    for (size_t i = 0; i < ten_msg_info_size; i++) {
      if (ten_msg_info[i].msg_type_name &&
          ten_c_string_is_equal(type_str, ten_msg_info[i].msg_type_name)) {
        msg_type = (TEN_MSG_TYPE)i;
        break;
      }
    }
  }

  switch (msg_type) {
    case TEN_MSG_TYPE_CMD:
      TEN_ASSERT(name_str, "Invalid argument.");

      // If it is a command, determine if it is a special command name to
      // identify whether it is actually a specialized message type.
      for (size_t i = 0; i < ten_msg_info_size; i++) {
        if (ten_msg_info[i].msg_unique_name &&
            ten_c_string_is_equal(name_str, ten_msg_info[i].msg_unique_name)) {
          msg_type = (TEN_MSG_TYPE)i;
          break;
        }
      }
      break;

    default:
      break;
  }

  if (!(msg_type > TEN_MSG_TYPE_INVALID && msg_type < TEN_MSG_TYPE_LAST)) {
    return TEN_MSG_TYPE_INVALID;
  }

  return msg_type;
}

const char *ten_msg_type_to_string(const TEN_MSG_TYPE type) {
  if (type >= ten_msg_info_size) {
    return NULL;
  }
  return ten_msg_info[type].msg_type_name;
}

bool ten_raw_msg_get_field_from_json(ten_msg_t *self, ten_json_t *json,
                                     ten_error_t *err) {
  TEN_ASSERT(self && json, "Should not happen.");

  for (size_t i = 0; i < ten_msg_fields_info_size; ++i) {
    ten_msg_get_field_from_json_func_t get_field_from_json =
        ten_msg_fields_info[i].get_field_from_json;
    if (get_field_from_json) {
      if (!get_field_from_json(self, json, err)) {
        return false;
      }
    }
  }

  return true;
}

bool ten_raw_msg_put_field_to_json(ten_msg_t *self, ten_json_t *json,
                                   ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) && json,
             "Should not happen.");

  for (size_t i = 0; i < ten_msg_fields_info_size; ++i) {
    ten_msg_put_field_to_json_func_t put_field_to_json =
        ten_msg_fields_info[i].put_field_to_json;
    if (put_field_to_json) {
      if (!put_field_to_json(self, json, err)) {
        return false;
      }
    }
  }

  return true;
}

ten_json_t *ten_msg_to_json(ten_shared_ptr_t *self, ten_error_t *err) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");

  ten_json_t *json = NULL;

  ten_raw_msg_to_json_func_t to_json =
      ten_msg_info[ten_msg_get_type(self)].to_json;
  if (to_json) {
    ten_msg_t *raw_msg = ten_msg_get_raw_msg(self);
    json = to_json(raw_msg, err);
  }

  return json;
}

void ten_raw_msg_copy_field(ten_msg_t *self, ten_msg_t *src,
                            ten_list_t *excluded_field_ids) {
  TEN_ASSERT(src && ten_raw_msg_check_integrity(src), "Should not happen.");

  for (size_t i = 0; i < ten_msg_fields_info_size; ++i) {
    if (excluded_field_ids) {
      bool skip = false;

      ten_list_foreach (excluded_field_ids, iter) {
        if (ten_msg_fields_info[i].field_id ==
            ten_int32_listnode_get(iter.node)) {
          skip = true;
          break;
        }
      }

      if (skip) {
        continue;
      }
    }

    ten_msg_copy_field_func_t copy_field = ten_msg_fields_info[i].copy_field;
    if (copy_field) {
      copy_field(self, src, excluded_field_ids);
    }
  }
}

static bool ten_raw_msg_init_from_json(ten_msg_t *self, ten_json_t *json,
                                       ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");

  const char *type_str =
      ten_msg_json_get_string_field_in_ten(json, TEN_STR_TYPE);
  const char *name_str =
      ten_msg_json_get_string_field_in_ten(json, TEN_STR_NAME);

  ten_raw_msg_check_type_and_name_func_t check_msg_type_and_name_string =
      ten_msg_info[ten_raw_msg_get_type(self)].check_type_and_name;
  if (check_msg_type_and_name_string) {
    if (!check_msg_type_and_name_string(self, type_str, name_str, err)) {
      if (err) {
        ten_error_set(err, TEN_ERRNO_INVALID_JSON,
                      "Failed to init a message from json, because the type "
                      "and name are incorrect.");
      }
      return false;
    }
  }

  ten_raw_msg_init_from_json_func_t init_raw_from_json =
      ten_msg_info[ten_raw_msg_get_type(self)].init_from_json;
  if (init_raw_from_json) {
    return init_raw_from_json(self, json, err);
  }

  TEN_ASSERT(0, "Should not happen.");
  if (err) {
    ten_error_set(
        err, TEN_ERRNO_INVALID_JSON,
        "Failed to init a message from json, because the type is unknown.");
  }
  return false;
}

bool ten_msg_from_json(ten_shared_ptr_t *self, ten_json_t *json,
                       ten_error_t *err) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");

  return ten_raw_msg_init_from_json(ten_shared_ptr_get_data(self), json, err);
}

ten_json_t *ten_raw_msg_to_json(ten_msg_t *self, ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");

  ten_json_t *json = NULL;

  ten_raw_msg_to_json_func_t to_json =
      ten_msg_info[ten_raw_msg_get_type(self)].to_json;
  if (to_json) {
    json = to_json(self, err);
  }

  return json;
}

ten_shared_ptr_t *ten_msg_clone(ten_shared_ptr_t *self,
                                ten_list_t *excluded_field_ids) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");

  ten_msg_t *raw_result = NULL;
  ten_shared_ptr_t *result = NULL;

  ten_raw_msg_clone_func_t clone = ten_msg_info[ten_msg_get_type(self)].clone;
  if (clone) {
    raw_result = clone(ten_msg_get_raw_msg(self), excluded_field_ids);
    if (raw_result) {
      result = ten_shared_ptr_create(raw_result, ten_raw_msg_destroy);

      if (ten_msg_is_cmd_base(self)) {
        // Create a relationship between the newly generated message and the
        // original message.
        ten_raw_cmd_base_save_cmd_id_to_parent_cmd_id(
            (ten_cmd_base_t *)raw_result);

        // The rule is simple:
        //
        // If generating/cloning a new message, that message will contain a new
        // command ID. If don't want a new command ID, then don't generate/clone
        // a new message.
        ten_raw_cmd_base_gen_new_cmd_id_forcibly((ten_cmd_base_t *)raw_result);
      }
    }
  }

  return result;
}

ten_shared_ptr_t *ten_msg_create_from_msg_type(TEN_MSG_TYPE msg_type) {
  switch (msg_type) {
    case TEN_MSG_TYPE_CMD_CLOSE_APP:
      return ten_cmd_close_app_create();
    case TEN_MSG_TYPE_CMD:
      return ten_cmd_custom_create_empty();
    case TEN_MSG_TYPE_CMD_RESULT:
      return ten_cmd_result_create_from_cmd(TEN_STATUS_CODE_OK, NULL);
    case TEN_MSG_TYPE_DATA:
      return ten_data_create();
    case TEN_MSG_TYPE_AUDIO_FRAME:
      return ten_audio_frame_create();
    case TEN_MSG_TYPE_VIDEO_FRAME:
      return ten_video_frame_create();
    default:
      return NULL;
  }
}

bool ten_msg_type_to_handle_when_closing(ten_shared_ptr_t *msg) {
  TEN_ASSERT(msg && ten_msg_check_integrity(msg), "Should not happen.");

  switch (ten_msg_get_type(msg)) {
    case TEN_MSG_TYPE_CMD_RESULT:
      return true;
    default:
      return false;
  }
}

const char *ten_raw_msg_get_type_string(ten_msg_t *self) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");
  return ten_msg_type_to_string(ten_raw_msg_get_type(self));
}

const char *ten_msg_get_type_string(ten_shared_ptr_t *self) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");
  return ten_raw_msg_get_type_string(ten_shared_ptr_get_data(self));
}

void ten_msg_correct_dest(ten_shared_ptr_t *msg, ten_engine_t *engine) {
  TEN_ASSERT(msg && ten_msg_check_integrity(msg), "Should not happen.");
  TEN_ASSERT(engine && ten_engine_check_integrity(engine, false),
             "Should not happen.");

  ten_string_t *app_uri = ten_app_get_uri(engine->app);

  ten_msg_t *raw_msg = ten_msg_get_raw_msg(msg);
  ten_list_foreach (&raw_msg->dest_loc, iter) {
    ten_loc_t *dest_loc = ten_ptr_listnode_get(iter.node);

    bool is_local_app = false;

    if (ten_string_is_equal(&dest_loc->app_uri, app_uri)) {
      is_local_app = true;
    } else if (ten_string_is_equal_c_str(&dest_loc->app_uri,
                                         TEN_STR_LOCALHOST)) {
      // Extension could use 'localhost' to indicate that the destination of the
      // message is to the local app, therefore, the TEN runtime needs to
      // 'correct' the real destination location from 'localhost' to the real
      // URI of the app.

      ten_string_copy(&dest_loc->app_uri, app_uri);
      is_local_app = true;
    }

    if (is_local_app) {
      // If the destination location is the local app, extension could omit the
      // engine graph ID in the message, therefore, TEN runtime needs to 'add'
      // this information back to the message, so that the TEN runtime could
      // know how to transit it after the TEN runtime receives that message.
      //
      // However, some command, for example, the 'start_graph' command, is a
      // special case, because this kind of command will be handled by the app,
      // rather than by any engines, therefore, we should not set the
      // destination graph_name in the case of such command.
      if (ten_msg_get_type(msg) == TEN_MSG_TYPE_CMD_START_GRAPH) {
        ten_msg_clear_dest_graph_name(msg);
      } else {
        ten_msg_set_dest_engine_if_unspecified_or_predefined_graph_name(
            msg, engine, &engine->app->predefined_graph_infos);
      }
    }
  }

  // In 'start_graph' command, the uri of an extension could be 'localhost',
  // which indicates that the extension will be located in the local app.
  // Therefore, the TEN runtime needs to 'correct' the real app uri from
  // 'localhost' to the real uri of the app.
  if (ten_msg_get_type(msg) == TEN_MSG_TYPE_CMD_START_GRAPH) {
    ten_list_t *extensions_info = ten_cmd_start_graph_get_extensions_info(msg);
    ten_list_foreach (extensions_info, iter) {
      ten_extension_info_t *extension_info =
          ten_shared_ptr_get_data(ten_smart_ptr_listnode_get(iter.node));
      ten_extension_info_translate_localhost_to_app_uri(
          extension_info, ten_string_get_raw_str(app_uri));
    }
  }
}

bool ten_msg_json_get_is_ten_field_exist(ten_json_t *json, const char *field) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");
  TEN_ASSERT(field, "Should not happen.");

  ten_json_t *ten_json =
      ten_json_object_peek_object(json, TEN_STR_UNDERLINE_TEN);
  TEN_ASSERT(ten_json, "Should not happen.");

  return ten_json_object_peek(ten_json, field) != NULL;
}

int64_t ten_msg_json_get_integer_field_in_ten(ten_json_t *json,
                                              const char *field) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");
  TEN_ASSERT(field, "Should not happen.");

  ten_json_t *ten_json =
      ten_json_object_peek_object(json, TEN_STR_UNDERLINE_TEN);
  TEN_ASSERT(ten_json, "Should not happen.");

  return ten_json_object_get_integer(ten_json, field);
}

const char *ten_msg_json_get_string_field_in_ten(ten_json_t *json,
                                                 const char *field) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");
  TEN_ASSERT(field, "Should not happen.");

  ten_json_t *ten_json =
      ten_json_object_peek_object(json, TEN_STR_UNDERLINE_TEN);
  if (!ten_json) {
    return NULL;
  }

  return ten_json_object_peek_string(ten_json, field);
}

TEN_MSG_TYPE ten_msg_json_get_msg_type(ten_json_t *json) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");

  const char *type_str =
      ten_msg_json_get_string_field_in_ten(json, TEN_STR_TYPE);
  const char *name_str =
      ten_msg_json_get_string_field_in_ten(json, TEN_STR_NAME);

  return ten_msg_type_from_type_and_name_string(type_str, name_str);
}

static bool ten_raw_msg_dump_internal(ten_msg_t *msg, ten_error_t *err,
                                      const char *fmt, va_list ap) {
  TEN_ASSERT(msg && ten_raw_msg_check_integrity(msg), "Should not happen.");

  ten_json_t *msg_json = ten_raw_msg_to_json(msg, err);
  if (!msg_json) {
    return false;
  }

  TEN_ASSERT(msg_json, "Failed to convert msg type(%s), key(%s) to JSON.",
             ten_msg_type_to_string(msg->type),
             ten_string_get_raw_str(&msg->name));

  bool must_free = false;
  const char *msg_json_str = ten_json_to_string(msg_json, NULL, &must_free);

  ten_string_t description;
  ten_string_init(&description);
  ten_string_set_from_va_list(&description, fmt, ap);

  const char *p = ten_string_get_raw_str(&description);

  ten_string_t dump_str;
  ten_string_init(&dump_str);

  while (*p) {
    if ('^' != *p) {
      ten_string_append_formatted(&dump_str, "%c", *p++);
      continue;
    }

    switch (*++p) {
      // The message content.
      case 'm':
        ten_string_append_formatted(&dump_str, "%s", msg_json_str);
        p++;
        break;

        // If next char can't match any mode, keep it.
      default:
        ten_string_append_formatted(&dump_str, "%c", *p++);
        break;
    }
  }

  TEN_LOGV("%s", ten_string_get_raw_str(&dump_str));

  ten_string_deinit(&dump_str);
  ten_string_deinit(&description);

  if (must_free) {
    TEN_FREE(msg_json_str);
  }
  ten_json_destroy(msg_json);

  return true;
}

bool ten_raw_msg_dump(ten_msg_t *msg, ten_error_t *err, const char *fmt, ...) {
  TEN_ASSERT(msg && ten_raw_msg_check_integrity(msg), "Should not happen.");

  va_list ap;
  va_start(ap, fmt);
  bool rc = ten_raw_msg_dump_internal(msg, err, fmt, ap);
  va_end(ap);

  return rc;
}

bool ten_msg_dump(ten_shared_ptr_t *msg, ten_error_t *err, const char *fmt,
                  ...) {
  TEN_ASSERT(msg && ten_msg_check_integrity(msg), "Should not happen.");

  va_list ap;
  va_start(ap, fmt);
  bool rc =
      ten_raw_msg_dump_internal(ten_shared_ptr_get_data(msg), err, fmt, ap);
  va_end(ap);

  return rc;
}

/**
 * @note The ownership of @a value is transferred to the @a msg.
 */
static bool ten_raw_msg_set_property(ten_msg_t *self, const char *path,
                                     ten_value_t *value, ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(value && ten_value_check_integrity(value), "Should not happen.");

  if (!path || !strlen(path)) {
    if (err) {
      ten_error_set(err, TEN_ERRNO_INVALID_ARGUMENT,
                    "path should not be empty.");
    }
    return NULL;
  }

  bool success = true;

  ten_list_t paths = TEN_LIST_INIT_VAL;
  if (!ten_value_path_parse(path, &paths, NULL)) {
    goto done;
  }

  bool in_ten_namespace = false;

  ten_list_foreach (&paths, item_iter) {
    ten_value_path_item_t *item = ten_ptr_listnode_get(item_iter.node);
    TEN_ASSERT(item, "Invalid argument.");

    switch (item->type) {
      case TEN_VALUE_PATH_ITEM_TYPE_OBJECT_ITEM: {
        if ((item_iter.index == 0) &&
            !strcmp(TEN_STR_UNDERLINE_TEN,
                    ten_string_get_raw_str(&item->obj_item_str))) {
          // It is the '_ten::' namespace.
          in_ten_namespace = true;

          // Remove the '_ten' namespace path part.
          ten_list_remove_front(&paths);

          ten_raw_msg_set_ten_property_func_t set_ten_property =
              ten_msg_info[ten_raw_msg_get_type(self)].set_ten_property;
          if (set_ten_property) {
            success = set_ten_property(self, &paths, value, err);
          }

          ten_value_destroy(value);
        }
        break;
      }

      default:
        break;
    }
  }

  if (!in_ten_namespace) {
    success = ten_value_set_from_path_list_with_move(&self->properties, &paths,
                                                     value, err);
  }

done:
  ten_list_clear(&paths);

  return success;
}

bool ten_msg_set_property(ten_shared_ptr_t *self, const char *path,
                          ten_value_t *value, ten_error_t *err) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(value && ten_value_check_integrity(value), "Should not happen.");

  if (!path || !strlen(path)) {
    if (err) {
      ten_error_set(err, TEN_ERRNO_INVALID_ARGUMENT,
                    "path should not be empty.");
    }
    return NULL;
  }

  return ten_raw_msg_set_property(ten_shared_ptr_get_data(self), path, value,
                                  err);
}

ten_value_t *ten_raw_msg_peek_property(ten_msg_t *self, const char *path,
                                       ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");

  if (!path || !strlen(path)) {
    if (err) {
      ten_error_set(err, TEN_ERRNO_INVALID_ARGUMENT,
                    "path should not be empty.");
    }
    return NULL;
  }

  ten_value_t *result = NULL;

  ten_list_t paths = TEN_LIST_INIT_VAL;
  if (!ten_value_path_parse(path, &paths, NULL)) {
    goto done;
  }

  bool in_ten_namespace = false;

  ten_list_foreach (&paths, item_iter) {
    ten_value_path_item_t *item = ten_ptr_listnode_get(item_iter.node);
    TEN_ASSERT(item, "Invalid argument.");

    switch (item->type) {
      case TEN_VALUE_PATH_ITEM_TYPE_OBJECT_ITEM: {
        if ((item_iter.index == 0) &&
            !strcmp(TEN_STR_UNDERLINE_TEN,
                    ten_string_get_raw_str(&item->obj_item_str))) {
          // It is the '_ten::' namespace.
          in_ten_namespace = true;

          // Remove the '_ten' namespace path part.
          ten_list_remove_front(&paths);

          ten_raw_msg_peek_ten_property_func_t peek_ten_property =
              ten_msg_info[ten_raw_msg_get_type(self)].peek_ten_property;
          if (peek_ten_property) {
            result = peek_ten_property(self, &paths, err);
          }
        }
        break;
      }

      default:
        break;
    }
  }

  if (!in_ten_namespace) {
    result = ten_value_peek_from_path(&self->properties, path, err);
  }

done:
  ten_list_clear(&paths);

  return result;
}

ten_value_t *ten_msg_peek_property(ten_shared_ptr_t *self, const char *path,
                                   ten_error_t *err) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && strlen(path), "path should not be empty.");

  return ten_raw_msg_peek_property(ten_msg_get_raw_msg(self), path, err);
}

static bool ten_raw_msg_has_locked_res(ten_msg_t *self) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");
  return !ten_list_is_empty(&self->locked_res);
}

bool ten_msg_has_locked_res(ten_shared_ptr_t *self) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");
  return ten_raw_msg_has_locked_res(ten_shared_ptr_get_data(self));
}

const char *ten_raw_msg_get_name(ten_msg_t *self) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");
  return ten_string_get_raw_str(&self->name);
}

const char *ten_msg_get_name(ten_shared_ptr_t *self) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");

  ten_msg_t *raw_msg = ten_msg_get_raw_msg(self);
  TEN_ASSERT(raw_msg, "Should not happen.");

  return ten_raw_msg_get_name(raw_msg);
}

bool ten_raw_msg_set_name_with_size(ten_msg_t *self, const char *msg_name,
                                    size_t msg_name_len, ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");

  if (msg_name == NULL) {
    if (err) {
      ten_error_set(err, TEN_ERRNO_GENERIC, "%s",
                    "Failed to set message name to an empty string.");
    }
    return false;
  }

  ten_string_set_formatted(&self->name, "%.*s", msg_name_len, msg_name);
  return true;
}

bool ten_raw_msg_set_name(ten_msg_t *self, const char *msg_name,
                          ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");
  return ten_raw_msg_set_name_with_size(self, msg_name, strlen(msg_name), err);
}

bool ten_msg_set_name_with_size(ten_shared_ptr_t *self, const char *msg_name,
                                size_t msg_name_len, ten_error_t *err) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");
  return ten_raw_msg_set_name_with_size(ten_shared_ptr_get_data(self), msg_name,
                                        msg_name_len, err);
}

bool ten_msg_set_name(ten_shared_ptr_t *self, const char *msg_name,
                      ten_error_t *err) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");
  return ten_raw_msg_set_name(ten_shared_ptr_get_data(self), msg_name, err);
}

bool ten_raw_msg_validate_schema(ten_msg_t *self,
                                 ten_schema_store_t *schema_store,
                                 bool is_msg_out, ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Invalid argument.");
  TEN_ASSERT(schema_store && ten_schema_store_check_integrity(schema_store),
             "Invalid argument.");
  TEN_ASSERT(err && ten_error_check_integrity(err), "Invalid argument.");

  const char *msg_name = ten_raw_msg_get_name(self);
  if (ten_raw_msg_is_cmd_base(self)) {
    TEN_ASSERT(msg_name && strlen(msg_name), "Should not happen.");
  }

  ten_msg_schema_t *schema = ten_schema_store_get_msg_schema(
      schema_store, ten_raw_msg_get_type(self), msg_name, is_msg_out);
  if (!schema) {
    return true;
  }

  if (!ten_msg_schema_adjust_properties(schema, &self->properties, err)) {
    return false;
  }

  if (!ten_msg_schema_validate_properties(schema, &self->properties, err)) {
    return false;
  }

  return true;
}

bool ten_msg_validate_schema(ten_shared_ptr_t *self,
                             ten_schema_store_t *schema_store, bool is_msg_out,
                             ten_error_t *err) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(schema_store && ten_schema_store_check_integrity(schema_store),
             "Should not happen.");
  TEN_ASSERT(err && ten_error_check_integrity(err), "Invalid argument.");

  ten_msg_t *msg = ten_shared_ptr_get_data(self);
  ten_raw_msg_validate_schema_func_t validate_schema =
      ten_msg_info[ten_raw_msg_get_type(msg)].validate_schema;
  if (validate_schema) {
    return validate_schema(msg, schema_store, is_msg_out, err);
  }

  return true;
}

TEN_MSG_TYPE ten_msg_get_type(ten_shared_ptr_t *self) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");
  return ten_raw_msg_get_type(ten_msg_get_raw_msg(self));
}
