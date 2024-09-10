//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_utils/container/list.h"
#include "ten_utils/io/runloop.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/event.h"
#include "ten_utils/lib/mutex.h"
#include "ten_utils/lib/signature.h"
#include "ten_utils/sanitizer/thread_check.h"

#define TEN_EXTENSION_THREAD_SIGNATURE 0xA1C756A818B47E1FU

#define EXTENSION_THREAD_QUEUE_SIZE 12800

typedef struct ten_extension_group_t ten_extension_group_t;
typedef struct ten_extension_store_t ten_extension_store_t;
typedef struct ten_extension_context_t ten_extension_context_t;
typedef struct ten_extension_t ten_extension_t;

typedef enum TEN_EXTENSION_THREAD_STATE {
  // All received messages will be kept in a temporary buffer, and wait until
  // the state switched to NORMAL.
  TEN_EXTENSION_THREAD_STATE_INIT,

  // All received messages will be fed into the extensions directly.
  TEN_EXTENSION_THREAD_STATE_NORMAL,

  // All the extensions have been started completely. The extension thread could
  // be 'suspended' only in this state.
  TEN_EXTENSION_THREAD_STATE_ALL_STARTED,

  // Give extension a chance to do something before the whole engine shuting
  // down.
  TEN_EXTENSION_THREAD_STATE_PREPARE_TO_CLOSE,

  // All received messages will be dropped.
  TEN_EXTENSION_THREAD_STATE_CLOSING,

  // The closing procedure is completed, so the extension thread can be
  // destroyed safely.
  TEN_EXTENSION_THREAD_STATE_CLOSED,
} TEN_EXTENSION_THREAD_STATE;

typedef struct ten_acquire_lock_mode_result_t {
  ten_event_t *completed;
  ten_error_t err;
} ten_acquire_lock_mode_result_t;

typedef struct ten_extension_thread_t {
  ten_signature_t signature;
  ten_sanitizer_thread_check_t thread_check;

  TEN_EXTENSION_THREAD_STATE state;

  ten_mutex_t *lock_mode_lock;
  bool in_lock_mode;

  ten_list_t pending_msgs;

  ten_list_t extensions;  // ten_extension_t*
  size_t extensions_cnt_of_added_to_engine;
  size_t extensions_cnt_of_deleted_from_engine;
  size_t extensions_cnt_of_on_init_done;
  size_t extensions_cnt_of_on_start_done;
  size_t extensions_cnt_of_on_stop_done;
  size_t extensions_cnt_of_set_closing_flag;

  // Store all extensions (ten_extension_t*) belong to this extension thread.
  ten_extension_store_t *extension_store;

  // One extension thread corresponds to one extension group.
  ten_extension_group_t *extension_group;
  ten_extension_context_t *extension_context;

  ten_runloop_t *runloop;
} ten_extension_thread_t;

TEN_RUNTIME_API bool ten_extension_thread_not_call_by_me(
    ten_extension_thread_t *self);

TEN_RUNTIME_PRIVATE_API bool ten_extension_thread_call_by_me(
    ten_extension_thread_t *self);

TEN_RUNTIME_PRIVATE_API bool ten_extension_thread_check_integrity_if_in_lock_mode(
    ten_extension_thread_t *self);

TEN_RUNTIME_PRIVATE_API bool ten_extension_thread_check_integrity(
    ten_extension_thread_t *self, bool check_thread);

TEN_RUNTIME_PRIVATE_API ten_extension_thread_t *ten_extension_thread_create(void);

TEN_RUNTIME_PRIVATE_API void ten_extension_thread_attach_to_context_and_group(
    ten_extension_thread_t *self, ten_extension_context_t *extension_context,
    ten_extension_group_t *extension_group);

TEN_RUNTIME_PRIVATE_API void ten_extension_thread_attach_to_group(
    ten_extension_thread_t *self, ten_extension_group_t *extension_group);

TEN_RUNTIME_PRIVATE_API void ten_extension_thread_destroy(
    ten_extension_thread_t *self);

TEN_RUNTIME_PRIVATE_API void ten_extension_thread_remove_from_extension_context(
    ten_extension_thread_t *self);

TEN_RUNTIME_PRIVATE_API void ten_extension_thread_start(
    ten_extension_thread_t *self);

TEN_RUNTIME_PRIVATE_API void *ten_extension_thread_main_actual(
    ten_extension_thread_t *self);

TEN_RUNTIME_PRIVATE_API void ten_extension_thread_close(
    ten_extension_thread_t *self);

TEN_RUNTIME_PRIVATE_API TEN_EXTENSION_THREAD_STATE
ten_extension_thread_get_state(ten_extension_thread_t *self);

TEN_RUNTIME_PRIVATE_API void ten_extension_thread_set_state(
    ten_extension_thread_t *self, TEN_EXTENSION_THREAD_STATE state);

TEN_RUNTIME_PRIVATE_API void
ten_extension_thread_determine_all_extension_dest_from_graph(
    ten_extension_thread_t *self);

TEN_RUNTIME_PRIVATE_API void ten_extension_thread_call_all_extension_on_start(
    ten_extension_thread_t *self);

TEN_RUNTIME_PRIVATE_API void
ten_extension_thread_start_to_add_all_created_extension_to_engine(
    ten_extension_thread_t *self);

TEN_RUNTIME_PRIVATE_API ten_runloop_t *ten_extension_thread_get_attached_runloop(
    ten_extension_thread_t *self);

TEN_RUNTIME_PRIVATE_API void ten_extension_thread_process_acquire_lock_mode_task(
    void *self_, void *arg);
