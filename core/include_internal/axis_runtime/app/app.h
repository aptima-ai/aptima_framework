//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/binding/common.h"
#include "include_internal/axis_runtime/metadata/metadata.h"
#include "include_internal/axis_runtime/schema_store/store.h"
#include "axis_runtime/app/app.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/container/list.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/sanitizer/thread_check.h"

#define axis_APP_SIGNATURE 0xF4551554E1E1240EU

typedef struct axis_connection_t axis_connection_t;
typedef struct axis_engine_t axis_engine_t;
typedef struct axis_protocol_t axis_protocol_t;

typedef enum axis_APP_STATE {
  axis_APP_STATE_INIT,
  axis_APP_STATE_CLOSING,  // The overall closing flow is started.
  axis_APP_STATE_CLOSED,   // on_deinit_done() is called actually.
} axis_APP_STATE;

typedef struct axis_app_t {
  axis_binding_handle_t binding_handle;

  axis_signature_t signature;
  axis_sanitizer_thread_check_t thread_check;

  axis_mutex_t *state_lock;
  axis_APP_STATE state;

  // @{
  // The accessing of this group of queues are all in the app thread, so no
  // need to apply any synchronization method on it.
  axis_list_t engines;
  axis_list_t orphan_connections;
  // @}

  bool run_in_background;

  // In some situations (for example, in the TEN nodejs environment, in order
  // not to blocking the JS main thread), we need to create a new thread to run
  // TEN app (run_in_background == true). And in the original thread, we need to
  // wait until we get the information of the actual thread of the TEN app, so
  // that if we call app_wait() in the original thread, the behavior would be
  // correct.
  //
  // Below is a pseudo code of this situation:
  // ====
  // app = new App();
  // app->run(true);
  // app->wait();
  axis_event_t *belonging_thread_is_set;

  axis_runloop_t *loop;

  axis_protocol_t *endpoint_protocol;

  axis_env_t *axis_env;

  axis_value_t manifest;
  axis_value_t property;

  axis_metadata_info_t *manifest_info;
  axis_metadata_info_t *property_info;

  axis_list_t predefined_graph_infos;  // axis_predefined_graph_info_t*

  axis_app_on_configure_func_t on_configure;
  axis_app_on_init_func_t on_init;
  axis_app_on_deinit_func_t on_deinit;

  // @{
  // Used to send messages to the app.
  axis_mutex_t *in_msgs_lock;
  axis_list_t in_msgs;
  // @}

  // @{
  // This section is for app default properties, we extract them from app
  // property store for efficiency.
  axis_string_t uri;
  bool one_event_loop_per_engine;
  bool long_running_mode;
  // @}

  axis_schema_store_t schema_store;
  axis_string_t base_dir;

  axis_list_t axis_package_base_dirs;

  void *user_data;
} axis_app_t;

axis_RUNTIME_PRIVATE_API void axis_app_add_orphan_connection(
    axis_app_t *self, axis_connection_t *connection);

axis_RUNTIME_PRIVATE_API void axis_app_del_orphan_connection(
    axis_app_t *self, axis_connection_t *connection);

axis_RUNTIME_PRIVATE_API bool axis_app_has_orphan_connection(
    axis_app_t *self, axis_connection_t *connection);

axis_RUNTIME_PRIVATE_API void axis_app_start(axis_app_t *self);

axis_RUNTIME_API axis_sanitizer_thread_check_t *axis_app_get_thread_check(
    axis_app_t *self);

axis_RUNTIME_PRIVATE_API bool axis_app_thread_call_by_me(axis_app_t *self);

axis_RUNTIME_PRIVATE_API axis_runloop_t *axis_app_get_attached_runloop(
    axis_app_t *self);

axis_RUNTIME_PRIVATE_API const char *axis_app_get_uri(axis_app_t *self);

axis_RUNTIME_PRIVATE_API void axis_app_on_configure(axis_env_t *axis_env);

axis_RUNTIME_PRIVATE_API void axis_app_on_init(axis_env_t *axis_env);

axis_RUNTIME_PRIVATE_API void axis_app_on_init_done(axis_env_t *axis_env);

axis_RUNTIME_PRIVATE_API void axis_app_on_deinit(axis_app_t *self);

axis_RUNTIME_PRIVATE_API void axis_app_on_deinit_done(axis_env_t *axis_env);

axis_RUNTIME_PRIVATE_API void axis_app_on_configure_done(axis_env_t *axis_env);

axis_RUNTIME_PRIVATE_API void axis_app_add_axis_package_base_dir(
    axis_app_t *self, const char *base_dir);
