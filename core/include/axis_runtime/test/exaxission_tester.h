//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include "aptima_utils/lib/smart_ptr.h"

typedef struct aptima_extension_tester_t aptima_extension_tester_t;
typedef struct aptima_env_tester_t aptima_env_tester_t;

typedef enum aptima_EXTENSION_TESTER_TEST_MODE {
  aptima_EXTENSION_TESTER_TEST_MODE_INVALID,

  aptima_EXTENSION_TESTER_TEST_MODE_SINGLE,
  aptima_EXTENSION_TESTER_TEST_MODE_GRAPH,
} aptima_EXTENSION_TESTER_TEST_MODE;

typedef void (*aptima_extension_tester_on_start_func_t)(
    aptima_extension_tester_t *self, aptima_env_tester_t *aptima_env);

typedef void (*aptima_extension_tester_on_cmd_func_t)(aptima_extension_tester_t *self,
                                                   aptima_env_tester_t *aptima_env,
                                                   aptima_shared_ptr_t *cmd);

typedef void (*aptima_extension_tester_on_data_func_t)(
    aptima_extension_tester_t *self, aptima_env_tester_t *aptima_env,
    aptima_shared_ptr_t *data);

typedef void (*aptima_extension_tester_on_audio_frame_func_t)(
    aptima_extension_tester_t *self, aptima_env_tester_t *aptima_env,
    aptima_shared_ptr_t *audio_frame);

typedef void (*aptima_extension_tester_on_video_frame_func_t)(
    aptima_extension_tester_t *self, aptima_env_tester_t *aptima_env,
    aptima_shared_ptr_t *video_frame);

aptima_RUNTIME_API aptima_extension_tester_t *aptima_extension_tester_create(
    aptima_extension_tester_on_start_func_t on_start,
    aptima_extension_tester_on_cmd_func_t on_cmd,
    aptima_extension_tester_on_data_func_t on_data,
    aptima_extension_tester_on_audio_frame_func_t on_audio_frame,
    aptima_extension_tester_on_video_frame_func_t on_video_frame);

aptima_RUNTIME_API void aptima_extension_tester_destroy(aptima_extension_tester_t *self);

// Testing a single extension, all messages input by the tester will be directed
// to this extension, and all outputs from the extension will be sent back to
// the tester.
aptima_RUNTIME_API void aptima_extension_tester_set_test_mode_single(
    aptima_extension_tester_t *self, const char *addon_name,
    const char *property_json_str);

// Testing a complete graph which must contain exactly one proxy extension. All
// messages input by the tester will be directed to this proxy extension, and
// all outputs from the proxy extension will be sent back to the tester.
aptima_RUNTIME_API void aptima_extension_tester_set_test_mode_graph(
    aptima_extension_tester_t *self, const char *graph_json);

aptima_RUNTIME_API void aptima_extension_tester_init_test_app_property_from_json(
    aptima_extension_tester_t *self, const char *property_json_str);

aptima_RUNTIME_API void aptima_extension_tester_add_addon_base_dir(
    aptima_extension_tester_t *self, const char *addon_base_dir);

aptima_RUNTIME_API bool aptima_extension_tester_run(aptima_extension_tester_t *self);

aptima_RUNTIME_API aptima_env_tester_t *aptima_extension_tester_get_aptima_env_tester(
    aptima_extension_tester_t *self);

aptima_RUNTIME_PRIVATE_API void aptima_extension_tester_on_test_extension_start(
    aptima_extension_tester_t *self);

aptima_RUNTIME_PRIVATE_API void aptima_extension_tester_on_test_extension_deinit(
    aptima_extension_tester_t *self);

aptima_RUNTIME_PRIVATE_API void aptima_extension_tester_on_start_done(
    aptima_extension_tester_t *self);
