//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/common/loc.h"
#include "axis_runtime/msg/msg.h"
#include "axis_utils/container/list.h"
#include "axis_utils/lib/atomic.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/sanitizer/thread_check.h"

#define axis_EXTENSION_CONTEXT_SIGNATURE 0x5968C666394DBCCCU

typedef struct axis_env_t axis_env_t;
typedef struct axis_engine_t axis_engine_t;
typedef struct axis_extension_t axis_extension_t;
typedef struct axis_extension_info_t axis_extension_info_t;
typedef struct axis_extension_store_t axis_extension_store_t;
typedef struct axis_extension_context_t axis_extension_context_t;
typedef struct axis_extension_group_info_t axis_extension_group_info_t;
typedef struct axis_extension_group_t axis_extension_group_t;

typedef void (*axis_extension_context_on_closed_func_t)(
    axis_extension_context_t *self, void *on_closed_data);

struct axis_extension_context_t {
  axis_signature_t signature;
  axis_sanitizer_thread_check_t thread_check;

  axis_list_t extension_groups;
  size_t extension_groups_cnt_of_being_destroyed;

  // Even if enabling runtime Extension-group addition/deletion, all
  // 'extension_threads' relevant operations should be done in the engine's main
  // thread, so we don't need to apply any locking mechanism for it.
  axis_list_t extension_threads;

  size_t extension_threads_cnt_of_ready;
  size_t extension_threads_cnt_of_closed;

  axis_list_t extension_groups_info_from_graph;
  axis_list_t extensions_info_from_graph;  // axis_extension_info_t*

  axis_atomic_t is_closing;
  axis_extension_context_on_closed_func_t on_closed;
  void *on_closed_data;

  axis_engine_t *engine;

  // 'state_requester_cmd' will be used in the following scenarios:
  // 1. starting all extension threads when client sends 'start_graph' cmd, and
  //    the state_requester_cmd is the start_graph cmd.
  // 2. closing all extension threads when receiving a close cmd, and the
  //    state_requester_cmd is the close cmd.
  axis_shared_ptr_t *state_requester_cmd;
};

axis_RUNTIME_PRIVATE_API bool axis_extension_context_check_integrity(
    axis_extension_context_t *self, bool check_thread);

axis_RUNTIME_PRIVATE_API axis_extension_context_t *axis_extension_context_create(
    axis_engine_t *engine);

axis_RUNTIME_PRIVATE_API void axis_extension_context_close(
    axis_extension_context_t *self);

axis_RUNTIME_PRIVATE_API void axis_extension_context_set_on_closed(
    axis_extension_context_t *self,
    axis_extension_context_on_closed_func_t on_closed, void *on_closed_data);

axis_RUNTIME_PRIVATE_API void axis_extension_context_on_close(
    axis_extension_context_t *self);

axis_RUNTIME_PRIVATE_API axis_extension_info_t *
axis_extension_context_get_extension_info_by_name(
    axis_extension_context_t *self, const char *app_uri, const char *graph_id,
    const char *extension_group_name, const char *extension_name);

axis_RUNTIME_PRIVATE_API bool axis_extension_context_start_extension_group(
    axis_extension_context_t *self, axis_shared_ptr_t *requester,
    axis_error_t *err);
