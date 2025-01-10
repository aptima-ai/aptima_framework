//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/container/list.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/event.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/sanitizer/thread_check.h"

#define axis_EXTENSION_THREAD_SIGNATURE 0xA1C756A818B47E1FU

#define EXTENSION_THREAD_QUEUE_SIZE 12800

typedef struct axis_extension_group_t axis_extension_group_t;
typedef struct axis_extension_store_t axis_extension_store_t;
typedef struct axis_extension_context_t axis_extension_context_t;
typedef struct axis_extension_t axis_extension_t;

typedef enum axis_EXTENSION_THREAD_STATE {
  axis_EXTENSION_THREAD_STATE_INIT,
  axis_EXTENSION_THREAD_STATE_CREATING_EXTENSIONS,
  axis_EXTENSION_THREAD_STATE_NORMAL,
  axis_EXTENSION_THREAD_STATE_PREPARE_TO_CLOSE,

  // All extensions of this extension thread are closed, and removed from this
  // extension thread.
  axis_EXTENSION_THREAD_STATE_CLOSED,
} axis_EXTENSION_THREAD_STATE;

typedef struct axis_acquire_lock_mode_result_t {
  axis_event_t *completed;
  axis_error_t err;
} axis_acquire_lock_mode_result_t;

typedef struct axis_extension_thread_t {
  axis_signature_t signature;
  axis_sanitizer_thread_check_t thread_check;

  axis_EXTENSION_THREAD_STATE state;
  bool is_close_triggered;

  axis_mutex_t *lock_mode_lock;
  bool in_lock_mode;

  axis_list_t pending_msgs;

  axis_list_t extensions;  // axis_extension_t*
  size_t extensions_cnt_of_deleted;

  // Store all extensions (axis_extension_t*) belong to this extension thread.
  axis_extension_store_t *extension_store;

  // One extension thread corresponds to one extension group.
  axis_extension_group_t *extension_group;
  axis_extension_context_t *extension_context;

  axis_runloop_t *runloop;
  axis_event_t *runloop_is_ready_to_use;
} axis_extension_thread_t;

axis_RUNTIME_API bool axis_extension_thread_not_call_by_me(
    axis_extension_thread_t *self);

axis_RUNTIME_PRIVATE_API bool axis_extension_thread_call_by_me(
    axis_extension_thread_t *self);

axis_RUNTIME_PRIVATE_API bool
axis_extension_thread_check_integrity_if_in_lock_mode(
    axis_extension_thread_t *self);

axis_RUNTIME_PRIVATE_API bool axis_extension_thread_check_integrity(
    axis_extension_thread_t *self, bool check_thread);

axis_RUNTIME_PRIVATE_API axis_extension_thread_t *axis_extension_thread_create(
    void);

axis_RUNTIME_PRIVATE_API void axis_extension_thread_attach_to_context_and_group(
    axis_extension_thread_t *self, axis_extension_context_t *extension_context,
    axis_extension_group_t *extension_group);

axis_RUNTIME_PRIVATE_API void axis_extension_thread_destroy(
    axis_extension_thread_t *self);

axis_RUNTIME_PRIVATE_API void axis_extension_thread_remove_from_extension_context(
    axis_extension_thread_t *self);

axis_RUNTIME_PRIVATE_API void axis_extension_thread_start(
    axis_extension_thread_t *self);

axis_RUNTIME_PRIVATE_API void *axis_extension_thread_main_actual(
    axis_extension_thread_t *self);

axis_RUNTIME_PRIVATE_API void axis_extension_thread_close(
    axis_extension_thread_t *self);

axis_RUNTIME_PRIVATE_API axis_EXTENSION_THREAD_STATE
axis_extension_thread_get_state(axis_extension_thread_t *self);

axis_RUNTIME_PRIVATE_API void axis_extension_thread_set_state(
    axis_extension_thread_t *self, axis_EXTENSION_THREAD_STATE state);

axis_RUNTIME_PRIVATE_API void axis_extension_thread_call_all_extension_on_start(
    axis_extension_thread_t *self);

axis_RUNTIME_PRIVATE_API void axis_extension_thread_add_all_created_extensions(
    axis_extension_thread_t *self);

axis_RUNTIME_PRIVATE_API axis_runloop_t *
axis_extension_thread_get_attached_runloop(axis_extension_thread_t *self);

axis_RUNTIME_PRIVATE_API void
axis_extension_thread_process_acquire_lock_mode_task(void *self_, void *arg);

axis_RUNTIME_PRIVATE_API void
axis_extension_thread_stop_life_cycle_of_all_extensions(
    axis_extension_thread_t *self);
