//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdint.h>

#include "axis_utils/container/list.h"
#include "axis_utils/value/value.h"

typedef struct axis_extension_t axis_extension_t;
typedef struct axis_error_t axis_error_t;
typedef struct axis_msg_t axis_msg_t;

// TEN runtime supports 2 kinds of message mapping.
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
typedef enum axis_MSG_TYPE {
  axis_MSG_TYPE_INVALID,
  axis_MSG_TYPE_CMD,
  axis_MSG_TYPE_CMD_RESULT,
  axis_MSG_TYPE_CMD_CLOSE_APP,
  axis_MSG_TYPE_CMD_START_GRAPH,
  axis_MSG_TYPE_CMD_STOP_GRAPH,
  axis_MSG_TYPE_CMD_TIMER,
  axis_MSG_TYPE_CMD_TIMEOUT,
  axis_MSG_TYPE_DATA,
  axis_MSG_TYPE_VIDEO_FRAME,
  axis_MSG_TYPE_AUDIO_FRAME,
  axis_MSG_TYPE_LAST,
} axis_MSG_TYPE;

/**
 * @brief The "clone" function of a command _does_ generate a new command ID.
 */
axis_RUNTIME_API axis_shared_ptr_t *axis_msg_clone(axis_shared_ptr_t *self,
                                                axis_list_t *excluded_field_ids);

axis_RUNTIME_API bool axis_msg_is_property_exist(axis_shared_ptr_t *self,
                                               const char *path,
                                               axis_error_t *err);

/**
 * @brief Note that the ownership of @a value_kv would be transferred into
 * the TEN runtime, so the caller of this function could _not_ consider the
 * value_kv instance is still valid.
 */
axis_RUNTIME_API bool axis_msg_set_property(axis_shared_ptr_t *self,
                                          const char *path, axis_value_t *value,
                                          axis_error_t *err);

// Because each TEN extension has its own messages (in almost all cases, except
// for the data-type messages), so the returned value_kv of this function is
// from the message directly, not a cloned one.
axis_RUNTIME_API axis_value_t *axis_msg_peek_property(axis_shared_ptr_t *self,
                                                   const char *path,
                                                   axis_error_t *err);

axis_RUNTIME_API bool axis_msg_clear_and_set_dest(
    axis_shared_ptr_t *self, const char *app_uri, const char *graph_id,
    const char *extension_group_name, const char *extension_name,
    axis_error_t *err);

axis_RUNTIME_API bool axis_msg_from_json(axis_shared_ptr_t *self, axis_json_t *json,
                                       axis_error_t *err);

axis_RUNTIME_API axis_json_t *axis_msg_to_json(axis_shared_ptr_t *self,
                                            axis_error_t *err);

axis_RUNTIME_API bool axis_msg_add_locked_res_buf(axis_shared_ptr_t *self,
                                                const uint8_t *data,
                                                axis_error_t *err);

axis_RUNTIME_API bool axis_msg_remove_locked_res_buf(axis_shared_ptr_t *self,
                                                   const uint8_t *data,
                                                   axis_error_t *err);

axis_RUNTIME_API const char *axis_msg_get_name(axis_shared_ptr_t *self);

axis_RUNTIME_API axis_MSG_TYPE axis_msg_get_type(axis_shared_ptr_t *self);

axis_RUNTIME_API bool axis_msg_set_name(axis_shared_ptr_t *self,
                                      const char *msg_name, axis_error_t *err);
