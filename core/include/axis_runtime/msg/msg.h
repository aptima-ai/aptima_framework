//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include <stdint.h>

#include "aptima_utils/container/list.h"
#include "aptima_utils/value/value.h"

typedef struct aptima_extension_t aptima_extension_t;
typedef struct aptima_error_t aptima_error_t;
typedef struct aptima_msg_t aptima_msg_t;

// APTIMA runtime supports 2 kinds of message mapping.
//
// > 1-to-1
//   Apply for : all messages.
//   This is the normal message mapping. The message will be transmitted to
//   the next node in the graph for non-status-command message, and to the
//   previous node in the graph for status-command message.
//
// > 1-to-N (when a message leaves an extension)
//   Apply for : all messages.
//   This can be declared in 'dests' in the graph declaration. The message
//   will be cloned to N copies, and sent to the N destinations.

// Note: To achieve the best compatibility, any new enum item, whether it is
// cmd/data/video_frame/audio_frame, should be added to the end to avoid
// changing the value of previous enum items.
typedef enum aptima_MSG_TYPE {
  aptima_MSG_TYPE_INVALID,
  aptima_MSG_TYPE_CMD,
  aptima_MSG_TYPE_CMD_RESULT,
  aptima_MSG_TYPE_CMD_CLOSE_APP,
  aptima_MSG_TYPE_CMD_START_GRAPH,
  aptima_MSG_TYPE_CMD_STOP_GRAPH,
  aptima_MSG_TYPE_CMD_TIMER,
  aptima_MSG_TYPE_CMD_TIMEOUT,
  aptima_MSG_TYPE_DATA,
  aptima_MSG_TYPE_VIDEO_FRAME,
  aptima_MSG_TYPE_AUDIO_FRAME,
  aptima_MSG_TYPE_LAST,
} aptima_MSG_TYPE;

/**
 * @brief The "clone" function of a command _does_ generate a new command ID.
 */
aptima_RUNTIME_API aptima_shared_ptr_t *aptima_msg_clone(aptima_shared_ptr_t *self,
                                                aptima_list_t *excluded_field_ids);

aptima_RUNTIME_API bool aptima_msg_is_property_exist(aptima_shared_ptr_t *self,
                                               const char *path,
                                               aptima_error_t *err);

/**
 * @brief Note that the ownership of @a value_kv would be transferred into
 * the APTIMA runtime, so the caller of this function could _not_ consider the
 * value_kv instance is still valid.
 */
aptima_RUNTIME_API bool aptima_msg_set_property(aptima_shared_ptr_t *self,
                                          const char *path, aptima_value_t *value,
                                          aptima_error_t *err);

// Because each APTIMA extension has its own messages (in almost all cases, except
// for the data-type messages), so the returned value_kv of this function is
// from the message directly, not a cloned one.
aptima_RUNTIME_API aptima_value_t *aptima_msg_peek_property(aptima_shared_ptr_t *self,
                                                   const char *path,
                                                   aptima_error_t *err);

aptima_RUNTIME_API bool aptima_msg_clear_and_set_dest(
    aptima_shared_ptr_t *self, const char *app_uri, const char *graph_id,
    const char *extension_group_name, const char *extension_name,
    aptima_error_t *err);

aptima_RUNTIME_API bool aptima_msg_from_json(aptima_shared_ptr_t *self, aptima_json_t *json,
                                       aptima_error_t *err);

aptima_RUNTIME_API aptima_json_t *aptima_msg_to_json(aptima_shared_ptr_t *self,
                                            aptima_error_t *err);

aptima_RUNTIME_API bool aptima_msg_add_locked_res_buf(aptima_shared_ptr_t *self,
                                                const uint8_t *data,
                                                aptima_error_t *err);

aptima_RUNTIME_API bool aptima_msg_remove_locked_res_buf(aptima_shared_ptr_t *self,
                                                   const uint8_t *data,
                                                   aptima_error_t *err);

aptima_RUNTIME_API const char *aptima_msg_get_name(aptima_shared_ptr_t *self);

aptima_RUNTIME_API aptima_MSG_TYPE aptima_msg_get_type(aptima_shared_ptr_t *self);

aptima_RUNTIME_API bool aptima_msg_set_name(aptima_shared_ptr_t *self,
                                      const char *msg_name, aptima_error_t *err);
