//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "include_internal/ten_runtime/addon/addon.h"
#include "include_internal/ten_runtime/extension_thread/extension_thread.h"

typedef struct ten_extension_thread_on_addon_create_extension_done_info_t {
  ten_extension_t *extension;
  ten_addon_context_t *addon_context;
} ten_extension_thread_on_addon_create_extension_done_info_t;

TEN_RUNTIME_PRIVATE_API ten_extension_thread_on_addon_create_extension_done_info_t *
ten_extension_thread_on_addon_create_extension_done_info_create(void);

TEN_RUNTIME_PRIVATE_API void
ten_extension_thread_on_addon_create_extension_done_info_destroy(
    ten_extension_thread_on_addon_create_extension_done_info_t *self);

TEN_RUNTIME_API void ten_extension_inherit_thread_ownership(
    ten_extension_t *self, ten_extension_thread_t *extension_thread);

TEN_RUNTIME_PRIVATE_API void ten_extension_thread_on_extension_added_to_engine(
    void *self_, void *arg);

TEN_RUNTIME_PRIVATE_API void
ten_extension_thread_on_extension_deleted_from_engine(void *self_, void *arg);

TEN_RUNTIME_PRIVATE_API void ten_extension_thread_on_extension_group_on_init_done(
    void *self_, void *arg);

TEN_RUNTIME_PRIVATE_API void ten_extension_thread_on_extension_on_init_done(
    void *self_, void *arg);

TEN_RUNTIME_PRIVATE_API void ten_extension_thread_on_extension_on_start_done(
    void *self_, void *arg);

TEN_RUNTIME_PRIVATE_API void ten_extension_thread_on_extension_on_stop_done(
    void *self_, void *arg);

TEN_RUNTIME_PRIVATE_API void ten_extension_thread_call_all_extensions_on_start(
    void *self_, void *arg);

TEN_RUNTIME_PRIVATE_API void ten_extension_thread_call_all_extensions_on_deinit(
    void *self_, void *arg);

TEN_RUNTIME_PRIVATE_API void ten_extension_thread_pre_close(void *self_,
                                                          void *arg);

TEN_RUNTIME_PRIVATE_API void ten_extension_thread_on_extension_set_closing_flag(
    void *self_, void *arg);

TEN_RUNTIME_PRIVATE_API void ten_extension_thread_on_extension_on_deinit_done(
    void *self_, void *arg);

TEN_RUNTIME_PRIVATE_API void
ten_extension_thread_on_all_extensions_in_all_extension_threads_added_to_engine(
    void *self_, void *arg);

TEN_RUNTIME_PRIVATE_API void
ten_extension_thread_on_extension_group_on_deinit_done(void *self_, void *arg);

TEN_RUNTIME_PRIVATE_API void ten_extension_thread_on_all_extensions_deleted(
    void *self_, void *arg);

TEN_RUNTIME_PRIVATE_API void ten_extension_thread_on_all_extensions_created(
    void *self_, void *arg);

TEN_RUNTIME_PRIVATE_API void ten_extension_thread_on_addon_create_extension_done(
    void *self_, void *arg);

TEN_RUNTIME_PRIVATE_API void ten_extension_thread_on_addon_destroy_extension_done(
    void *self_, void *arg);

TEN_RUNTIME_PRIVATE_API void ten_extension_thread_create_addon_instance(
    void *self_, void *arg);

TEN_RUNTIME_PRIVATE_API void ten_extension_thread_destroy_addon_instance(
    void *self_, void *arg);
