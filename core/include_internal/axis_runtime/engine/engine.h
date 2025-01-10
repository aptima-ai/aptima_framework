//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/engine/internal/close.h"
#include "include_internal/axis_runtime/path/path_table.h"
#include "axis_utils/container/hash_table.h"
#include "axis_utils/container/list.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/atomic.h"
#include "axis_utils/lib/event.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"

#define axis_ENGINE_SIGNATURE 0x68E43695C0DB905AU

#define CMD_ID_COUNTER_MIN_VALUE 0
#define CMD_ID_COUNTER_MAX_VALUE 4095

typedef struct axis_extension_context_t axis_extension_context_t;
typedef struct axis_app_t axis_app_t;
typedef struct axis_env_t axis_env_t;

struct axis_engine_t {
  axis_signature_t signature;
  axis_sanitizer_thread_check_t thread_check;

  axis_atomic_t is_closing;

  axis_engine_on_closed_func_t on_closed;
  void *on_closed_data;

  axis_app_t *app;
  axis_extension_context_t *extension_context;

  axis_env_t *axis_env;

  // This means that the engine can start to handle messages, i.e. all the
  // extension threads are started successfully.
  //
  // TODO(Wei): Perhaps this variable can be removed.
  bool is_ready_to_handle_msg;

  // When app creates an engine, it will create a randomized graph ID for the
  // engine. It _must_ be a UUID4 string.
  axis_string_t graph_id;

  axis_path_table_t *path_table;

  // Save the original received 'start_graph' command so that after we
  // successfully started the engine, we can return a correct cmd result back
  // according to this saved 'start_graph' command.
  axis_shared_ptr_t *original_start_graph_cmd_of_enabling_engine;

  axis_list_t timers;

  // @{
  axis_hashtable_t remotes;  // axis_remote_t
  axis_list_t weak_remotes;
  // @}

  // @{
  axis_mutex_t *extension_msgs_lock;
  axis_list_t extension_msgs;
  // @}

  // @{
  // Used to send messages to the engine.
  axis_mutex_t *in_msgs_lock;
  axis_list_t in_msgs;
  // @}

  // @{
  // The following members are used for engines which have its own event loop.
  bool has_own_loop;
  axis_runloop_t *loop;
  axis_event_t *belonging_thread_is_set;
  axis_event_t *engine_thread_ready_for_migration;
  // @}

  bool long_running_mode;

  // Store the stop_graph command that will shut down this engine temporarily,
  // so that after the engine has completely closed, the cmd_result can be
  // returned based on this.
  axis_shared_ptr_t *cmd_stop_graph;
};

axis_RUNTIME_PRIVATE_API bool axis_engine_check_integrity(axis_engine_t *self,
                                                        bool check_thread);

axis_RUNTIME_PRIVATE_API axis_engine_t *axis_engine_create(axis_app_t *app,
                                                        axis_shared_ptr_t *cmd);

axis_RUNTIME_PRIVATE_API void axis_engine_destroy(axis_engine_t *self);

axis_RUNTIME_PRIVATE_API axis_runloop_t *axis_engine_get_attached_runloop(
    axis_engine_t *self);

axis_RUNTIME_PRIVATE_API bool axis_engine_is_ready_to_handle_msg(
    axis_engine_t *self);

axis_RUNTIME_PRIVATE_API const char *axis_engine_get_id(axis_engine_t *self,
                                                      bool check_thread);
