//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/lib/smart_ptr.h"

typedef struct axis_extension_tester_t axis_extension_tester_t;
typedef struct axis_env_tester_t axis_env_tester_t;

typedef enum axis_EXTENSION_TESTER_TEST_MODE {
  axis_EXTENSION_TESTER_TEST_MODE_INVALID,

  axis_EXTENSION_TESTER_TEST_MODE_SINGLE,
  axis_EXTENSION_TESTER_TEST_MODE_GRAPH,
} axis_EXTENSION_TESTER_TEST_MODE;

typedef void (*axis_extension_tester_on_start_func_t)(
    axis_extension_tester_t *self, axis_env_tester_t *axis_env);

typedef void (*axis_extension_tester_on_cmd_func_t)(axis_extension_tester_t *self,
                                                   axis_env_tester_t *axis_env,
                                                   axis_shared_ptr_t *cmd);

typedef void (*axis_extension_tester_on_data_func_t)(
    axis_extension_tester_t *self, axis_env_tester_t *axis_env,
    axis_shared_ptr_t *data);

typedef void (*axis_extension_tester_on_audio_frame_func_t)(
    axis_extension_tester_t *self, axis_env_tester_t *axis_env,
    axis_shared_ptr_t *audio_frame);

typedef void (*axis_extension_tester_on_video_frame_func_t)(
    axis_extension_tester_t *self, axis_env_tester_t *axis_env,
    axis_shared_ptr_t *video_frame);

axis_RUNTIME_API axis_extension_tester_t *axis_extension_tester_create(
    axis_extension_tester_on_start_func_t on_start,
    axis_extension_tester_on_cmd_func_t on_cmd,
    axis_extension_tester_on_data_func_t on_data,
    axis_extension_tester_on_audio_frame_func_t on_audio_frame,
    axis_extension_tester_on_video_frame_func_t on_video_frame);

axis_RUNTIME_API void axis_extension_tester_destroy(axis_extension_tester_t *self);

// Testing a single extension, all messages input by the tester will be directed
// to this extension, and all outputs from the extension will be sent back to
// the tester.
axis_RUNTIME_API void axis_extension_tester_set_test_mode_single(
    axis_extension_tester_t *self, const char *addon_name,
    const char *property_json_str);

// Testing a complete graph which must contain exactly one proxy extension. All
// messages input by the tester will be directed to this proxy extension, and
// all outputs from the proxy extension will be sent back to the tester.
axis_RUNTIME_API void axis_extension_tester_set_test_mode_graph(
    axis_extension_tester_t *self, const char *graph_json);

axis_RUNTIME_API void axis_extension_tester_init_test_app_property_from_json(
    axis_extension_tester_t *self, const char *property_json_str);

axis_RUNTIME_API void axis_extension_tester_add_addon_base_dir(
    axis_extension_tester_t *self, const char *addon_base_dir);

axis_RUNTIME_API bool axis_extension_tester_run(axis_extension_tester_t *self);

axis_RUNTIME_API axis_env_tester_t *axis_extension_tester_get_axis_env_tester(
    axis_extension_tester_t *self);

axis_RUNTIME_PRIVATE_API void axis_extension_tester_on_test_extension_start(
    axis_extension_tester_t *self);

axis_RUNTIME_PRIVATE_API void axis_extension_tester_on_test_extension_deinit(
    axis_extension_tester_t *self);

axis_RUNTIME_PRIVATE_API void axis_extension_tester_on_start_done(
    axis_extension_tester_t *self);
