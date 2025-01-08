//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/engine/msg_interface/close_app.h"
#include "include_internal/axis_runtime/engine/msg_interface/cmd_result.h"
#include "include_internal/axis_runtime/engine/msg_interface/start_graph.h"
#include "include_internal/axis_runtime/engine/msg_interface/stop_graph.h"
#include "include_internal/axis_runtime/engine/msg_interface/timer.h"
#include "include_internal/axis_runtime/msg/audio_frame/audio_frame.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/close_app/cmd.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/custom/cmd.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/start_graph/cmd.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/stop_graph/cmd.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/timeout/cmd.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/timer/cmd.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd_result/cmd.h"
#include "include_internal/axis_runtime/msg/data/data.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/msg/video_frame/video_frame.h"
#include "axis_runtime/msg/cmd/start_graph/cmd.h"
#include "axis_runtime/msg/msg.h"

typedef void (*axis_msg_engine_handler_func_t)(axis_engine_t *engine,
                                              axis_shared_ptr_t *msg,
                                              axis_error_t *err);

typedef axis_msg_t *(*axis_raw_msg_clone_func_t)(axis_msg_t *msg,
                                               axis_list_t *excluded_field_ids);

typedef bool (*axis_raw_msg_loop_all_fields_func_t)(
    axis_msg_t *msg, axis_raw_msg_process_one_field_func_t cb, void *user_data,
    axis_error_t *err);

typedef bool (*axis_raw_msg_validate_schema_func_t)(
    axis_msg_t *msg, axis_schema_store_t *schema_store, bool is_msg_out,
    axis_error_t *err);

typedef bool (*axis_raw_msg_set_axis_property_func_t)(axis_msg_t *msg,
                                                    axis_list_t *paths,
                                                    axis_value_t *value,
                                                    axis_error_t *err);

typedef axis_value_t *(*axis_raw_msg_peek_axis_property_func_t)(axis_msg_t *msg,
                                                             axis_list_t *paths,
                                                             axis_error_t *err);

typedef struct axis_msg_info_t {
  const char *msg_type_name;
  const char *msg_unique_name;

  bool create_in_path;
  axis_msg_engine_handler_func_t engine_handler;
  axis_raw_msg_clone_func_t clone;
  axis_raw_msg_loop_all_fields_func_t loop_all_fields;
  axis_raw_msg_validate_schema_func_t validate_schema;
  axis_raw_msg_set_axis_property_func_t set_axis_property;
  axis_raw_msg_peek_axis_property_func_t peek_axis_property;
} axis_msg_info_t;

#define axis_MSG_INFO_INIT_VALUES     \
  {                                  \
      NULL,  /* msg_type_name */     \
      NULL,  /* unique_name */       \
      false, /* create_in_path */    \
      NULL,  /* engine_handler */    \
      NULL,  /* clone */             \
      NULL,  /* loop_all_fields */   \
      NULL,  /* validate_schema */   \
      NULL,  /* set_axis_property */  \
      NULL,  /* peek_axis_property */ \
  }

axis_UNUSED static const axis_msg_info_t axis_msg_info[] = {
    [axis_MSG_TYPE_INVALID] = axis_MSG_INFO_INIT_VALUES,
    [axis_MSG_TYPE_CMD] =
        {
            axis_STR_CMD, /* msg_type_name */

            // General cmd do not have a special name.
            NULL, /* msg_unique_name */

            true, /* create_in_path */

            // The engine does not need to handle general cmd.
            NULL, /* engine_handler */

            axis_raw_cmd_custom_as_msg_clone,     /* clone */
            axis_raw_cmd_custom_loop_all_fields,  /* loop_all_fields */
            axis_raw_msg_validate_schema,         /* validate_schema */
            axis_raw_cmd_custom_set_axis_property, /* set_axis_property */
            NULL,                                /* peek_axis_property */
        },
    [axis_MSG_TYPE_CMD_STOP_GRAPH] =
        {
            axis_STR_STOP_GRAPH,                     /* msg_type_name */
            axis_STR_MSG_NAME_axis_STOP_GRAPH,        /* msg_unique_name */
            false,                                  /* create_in_path */
            axis_engine_handle_cmd_stop_graph,       /* engine_handler */
            NULL,                                   /* clone */
            axis_raw_cmd_stop_graph_loop_all_fields, /* loop_all_fields */
            NULL,                                   /* validate_schema */
            NULL,                                   /* set_axis_property */
            NULL,                                   /* peek_axis_property */
        },
    [axis_MSG_TYPE_CMD_START_GRAPH] =
        {
            axis_STR_START_GRAPH,                     /* msg_type_name */
            axis_STR_MSG_NAME_axis_START_GRAPH,        /* msg_unique_name */
            true,                                    /* create_in_path */
            axis_engine_handle_cmd_start_graph,       /* engine_handler */
            axis_raw_cmd_start_graph_as_msg_clone,    /* clone */
            axis_raw_cmd_start_graph_loop_all_fields, /* loop_all_fields */
            NULL,                                    /* validate_schema */
            NULL,                                    /* set_axis_property */
            NULL,                                    /* peek_axis_property */
        },
    [axis_MSG_TYPE_CMD_RESULT] =
        {
            axis_STR_RESULT,                     /* msg_type_name */
            axis_STR_MSG_NAME_axis_RESULT,        /* msg_unique_name */
            false,                              /* create_in_path */
            axis_engine_handle_cmd_result,       /* engine_handler */
            axis_raw_cmd_result_as_msg_clone,    /* clone */
            axis_raw_cmd_result_loop_all_fields, /* loop_all_fields */
            axis_raw_cmd_result_validate_schema, /* validate_schema */
            NULL,                               /* set_axis_property */
            NULL,                               /* peek_axis_property */
        },
    [axis_MSG_TYPE_CMD_CLOSE_APP] =
        {
            axis_STR_CLOSE_APP,                     /* msg_type_name */
            axis_STR_MSG_NAME_axis_CLOSE_APP,        /* msg_unique_name */
            false,                                 /* create_in_path */
            axis_engine_handle_cmd_close_app,       /* engine_handler */
            NULL,                                  /* clone */
            axis_raw_cmd_close_app_loop_all_fields, /* loop_all_fields */
            NULL,                                  /* validate_schema */
            NULL,                                  /* set_axis_property */
            NULL,                                  /* peek_axis_property */
        },
    [axis_MSG_TYPE_CMD_TIMEOUT] =
        {
            axis_STR_TIMEOUT,                     /* msg_type_name */
            axis_STR_MSG_NAME_axis_TIMEOUT,        /* msg_unique_name */
            false,                               /* create_in_path */
            NULL,                                /* engine_handler */
            NULL,                                /* clone */
            axis_raw_cmd_timeout_loop_all_fields, /* loop_all_fields */
            NULL,                                /* validate_schema */
            NULL,                                /* set_axis_property */
            NULL,                                /* peek_axis_property */
        },
    [axis_MSG_TYPE_CMD_TIMER] =
        {
            axis_STR_TIMER,                      /* msg_type_name */
            axis_STR_MSG_NAME_axis_TIMER,         /* msg_unique_name */
            true,                               /* create_in_path */
            axis_engine_handle_cmd_timer,        /* engine_handler */
            NULL,                               /* clone */
            axis_raw_cmd_timer_loop_all_fields,  /* loop_all_fields */
            NULL,                               /* validate_schema */
            axis_raw_cmd_timer_set_axis_property, /* set_axis_property */
            NULL,                               /* peek_axis_property */
        },
    [axis_MSG_TYPE_DATA] =
        {
            axis_STR_DATA, /* msg_type_name */

            // General data do not have a special name.
            NULL, /* msg_unique_name */

            false, /* create_in_path */

            // The engine does not need to handle general data.
            NULL, /* engine_handler */

            axis_raw_data_as_msg_clone,          /* clone */
            axis_raw_data_loop_all_fields,       /* loop_all_fields */
            axis_raw_msg_validate_schema,        /* validate_schema */
            axis_raw_data_like_set_axis_property, /* set_axis_property */
            NULL,                               /* peek_axis_property */
        },
    [axis_MSG_TYPE_AUDIO_FRAME] =
        {
            axis_STR_AUDIO_FRAME, /* msg_type_name */

            // General audio frame do not have a special name.
            NULL, /* msg_unique_name */

            false, /* create_in_path */

            // The engine does not need to handle general audio frame.
            NULL, /* engine_handler */

            axis_raw_audio_frame_as_msg_clone,      /* clone */
            axis_raw_audio_frame_loop_all_fields,   /* loop_all_fields */
            axis_raw_msg_validate_schema,           /* validate_schema */
            axis_raw_data_like_set_axis_property,    /* set_axis_property */
            axis_raw_audio_frame_peek_axis_property, /* peek_axis_property */
        },
    [axis_MSG_TYPE_VIDEO_FRAME] =
        {
            axis_STR_VIDEO_FRAME, /* msg_type_name */

            // General video frame do not have a special name.
            NULL, /* msg_unique_name */

            false, /* create_in_path */

            // The engine does not need to handle general video frame.
            NULL, /* engine_handler */

            axis_raw_video_frame_as_msg_clone,      /* clone */
            axis_raw_video_frame_loop_all_fields,   /* loop_all_fields */
            axis_raw_msg_validate_schema,           /* validate_schema */
            axis_raw_video_frame_set_axis_property,  /* set_axis_property */
            axis_raw_video_frame_peek_axis_property, /* peek_axis_property */
        },
    [axis_MSG_TYPE_LAST] = axis_MSG_INFO_INIT_VALUES,
};

static const size_t axis_msg_info_size =
    sizeof(axis_msg_info) / sizeof(axis_msg_info[0]);
