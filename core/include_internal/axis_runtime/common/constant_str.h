//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

// Message.
#define axis_STR_CMD "cmd"
#define axis_STR_DATA "data"
#define axis_STR_VIDEO_FRAME "video_frame"
#define axis_STR_INTERFACE "interface"
#define axis_STR_AUDIO_FRAME "audio_frame"

// Message relevant fields.
#define axis_STR_PROPERTY "property"
#define axis_STR_RESULT "result"
#define axis_STR_CMD_ID "cmd_id"
#define axis_STR_SEQ_ID "seq_id"

// Location relevant fields.
#define axis_STR_SRC "src"
#define axis_STR_DEST "dest"
#define axis_STR_EXTENSION_GROUP "extension_group"
#define axis_STR_EXTENSION "extension"

// Result command relevant fields.
#define axis_STR_STATUS_CODE "status_code"
#define axis_STR_DETAIL "detail"
#define axis_STR_IS_FINAL "is_final"

// Timer relevant fields.
#define axis_STR_TIMER_ID "timer_id"
#define axis_STR_TIMES "times"
#define axis_STR_TIMEOUT_IN_US "timeout_in_us"

// 'api' field relevant.
#define axis_STR_API "api"
#define axis_STR_CMD_IN "cmd_in"
#define axis_STR_CMD_OUT "cmd_out"
#define axis_STR_DATA_IN "data_in"
#define axis_STR_DATA_OUT "data_out"
#define axis_STR_VIDEO_FRAME_IN "video_frame_in"
#define axis_STR_VIDEO_FRAME_OUT "video_frame_out"
#define axis_STR_INTERFACE_IN "interface_in"
#define axis_STR_INTERFACE_OUT "interface_out"
#define axis_STR_AUDIO_FRAME_IN "audio_frame_in"
#define axis_STR_AUDIO_FRAME_OUT "audio_frame_out"

#define axis_STR_MSG_NAME_axis_NAMESPACE_PREFIX "ten:"

// The message name might be empty, however, the property schema will be stored
// in a hashtable which key is the name of the msg. Using a special name to
// store the schema if the msg name is not provided.
//
// Because ':' is not a legal character in the TEN schema specification for
// message names, ':' is used in special message names for purely internal use.
#define axis_STR_MSG_NAME_axis_EMPTY "ten:empty"

#define axis_STR_MSG_NAME_axis_CLOSE_APP "ten:close_app"
#define axis_STR_MSG_NAME_axis_STOP_GRAPH "ten:stop_graph"
#define axis_STR_MSG_NAME_axis_START_GRAPH "ten:start_graph"
#define axis_STR_MSG_NAME_axis_RESULT "ten:result"
#define axis_STR_MSG_NAME_axis_TIMEOUT "ten:timeout"
#define axis_STR_MSG_NAME_axis_TIMER "ten:timer"

// Special command.
#define axis_STR_STOP_GRAPH "stop_graph"
#define axis_STR_CLOSE_APP "close_app"
#define axis_STR_START_GRAPH "start_graph"
#define axis_STR_TIMER "timer"
#define axis_STR_TIMEOUT "timeout"

// Graph relevant.
#define axis_STR_PREDEFINED_GRAPHS "predefined_graphs"
#define axis_STR_PREDEFINED_GRAPH "predefined_graph"
#define axis_STR_NODES "nodes"
#define axis_STR_CONNECTIONS "connections"

#define axis_STR_PROPERTY_STORE_SCOPE_DELIMITER ":"

// Msg conversion.
#define axis_STR_VALUE "value"
#define axis_STR_MSG_CONVERSION "msg_conversion"
#define axis_STR_CONVERSION_MODE "conversion_mode"
#define axis_STR_PATH "path"
#define axis_STR_FIXED_VALUE "fixed_value"
#define axis_STR_FROM_ORIGINAL "from_original"
#define axis_STR_RULES "rules"
#define axis_STR_ORIGINAL_PATH "original_path"
#define axis_STR_PER_PROPERTY "per_property"
#define axis_STR_KEEP_ORIGINAL "keep_original"

// Path.
#define axis_STR_PATH_TIMEOUT "path_timeout"
#define axis_STR_IN_PATH "in_path"
#define axis_STR_OUT_PATH "out_path"
#define axis_STR_PATH_CHECK_INTERVAL "path_check_interval"
#define axis_STR_ORIGINAL_CMD_TYPE "original_cmd_type"
#define axis_STR_ORIGINAL_CMD_NAME "original_cmd_name"

// Protocol.
#define axis_STR_PROTOCOL "protocol"

// Lang addon loader.
#define axis_STR_ADDON_LOADER "addon_loader"

// Transport.
#define axis_STR_TRANSPORT_TYPE "transport_type"
#define axis_STR_TCP "tcp"

// App uri.
#define axis_STR_URI "uri"
#define axis_STR_LOCALHOST "localhost"
#define axis_STR_CLIENT "client:"

// Data.
#define axis_STR_BUF "buf"

// Video frame.
#define axis_STR_PIXEL_FMT "pixel_fmt"
#define axis_STR_TIMESTAMP "timestamp"
#define axis_STR_WIDTH "width"
#define axis_STR_HEIGHT "height"
#define axis_STR_IS_EOF "is_eof"

// audio frame.
#define axis_STR_DATA_FMT "data_fmt"
#define axis_STR_LINE_SIZE "line_size"
#define axis_STR_BYTES_PER_SAMPLE "bytes_per_sample"
#define axis_STR_CHANNEL_LAYOUT "channel_layout"
#define axis_STR_NUMBER_OF_CHANNEL "number_of_channel"
#define axis_STR_SAMPLE_RATE "sample_rate"
#define axis_STR_SAMPLES_PER_CHANNEL "samples_per_channel"

// Graph.
#define axis_STR_AUTO_START "auto_start"
#define axis_STR_SINGLETON "singleton"
#define axis_STR_GRAPH "graph"
#define axis_STR_GRAPH_NAME "graph_name"
#define axis_STR_GRAPH_ID "graph_id"

#define axis_STR_CASCADE_CLOSE_UPWARD "cascade_close_upward"
#define axis_STR_DUPLICATE "duplicate"
#define axis_STR_ADDON "addon"
#define axis_STR_axis_PACKAGES "axis_packages"
#define axis_STR_ONE_EVENT_LOOP_PER_ENGINE "one_event_loop_per_engine"
#define axis_STR_LOG_LEVEL "log_level"
#define axis_STR_LOG_FILE "log_file"
#define axis_STR_PROPERTIES "properties"
#define axis_STR_LONG_RUNNING_MODE "long_running_mode"
#define axis_STR_TYPE "type"
#define axis_STR_APP "app"
#define axis_STR_NAME "name"
#define axis_STR_UNDERLINE_TEN "_ten"
#define axis_STR_STAR "*"

#define axis_STR_DEFAULT_EXTENSION_GROUP "default_extension_group"
#define axis_STR_axis_TEST_EXTENSION "ten:test_extension"

#define axis_STR_MANIFEST_JSON "manifest.json"
