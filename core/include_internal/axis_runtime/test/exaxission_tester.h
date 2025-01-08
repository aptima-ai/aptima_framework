//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/binding/common.h"
#include "include_internal/axis_utils/io/runloop.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_runtime/test/extension_tester.h"
#include "axis_utils/lib/signature.h"

#define axis_EXTENSION_TESTER_SIGNATURE 0x2343E0B8559B7147U

typedef struct axis_extension_tester_t axis_extension_tester_t;
typedef struct axis_env_tester_t axis_env_tester_t;

struct axis_extension_tester_t {
  axis_binding_handle_t binding_handle;

  axis_signature_t signature;
  axis_sanitizer_thread_check_t thread_check;

  axis_thread_t *test_app_thread;
  axis_env_proxy_t *test_app_axis_env_proxy;
  axis_event_t *test_app_axis_env_proxy_create_completed;

  axis_env_proxy_t *test_extension_axis_env_proxy;
  axis_event_t *test_extension_axis_env_proxy_create_completed;

  axis_EXTENSION_TESTER_TEST_MODE test_mode;

  union {
    struct {
      axis_string_t addon_name;
      axis_string_t property_json;
    } addon;

    struct {
      axis_string_t graph_json;
    } graph;
  } test_target;

  axis_list_t addon_base_dirs;

  axis_string_t test_app_property_json;

  axis_extension_tester_on_start_func_t on_start;
  axis_extension_tester_on_cmd_func_t on_cmd;
  axis_extension_tester_on_data_func_t on_data;
  axis_extension_tester_on_audio_frame_func_t on_audio_frame;
  axis_extension_tester_on_video_frame_func_t on_video_frame;

  axis_env_tester_t *axis_env_tester;
  axis_runloop_t *tester_runloop;

  void *user_data;
};

axis_RUNTIME_API bool axis_extension_tester_check_integrity(
    axis_extension_tester_t *self, bool check_thread);

axis_RUNTIME_PRIVATE_API void test_app_axis_env_send_cmd(axis_env_t *axis_env,
                                                       void *user_data);
