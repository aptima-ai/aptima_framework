//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/extension_thread/extension_thread.h"

typedef struct axis_extension_thread_on_addon_create_extension_done_ctx_t {
  axis_extension_t *extension;
  axis_addon_context_t *addon_context;
} axis_extension_thread_on_addon_create_extension_done_ctx_t;

axis_RUNTIME_PRIVATE_API axis_extension_thread_on_addon_create_extension_done_ctx_t *
axis_extension_thread_on_addon_create_extension_done_ctx_create(void);

axis_RUNTIME_PRIVATE_API void
axis_extension_thread_on_addon_create_extension_done_ctx_destroy(
    axis_extension_thread_on_addon_create_extension_done_ctx_t *self);

axis_RUNTIME_API void axis_extension_inherit_thread_ownership(
    axis_extension_t *self, axis_extension_thread_t *extension_thread);

axis_RUNTIME_PRIVATE_API void
axis_extension_thread_on_extension_group_on_init_done(void *self_, void *arg);

axis_RUNTIME_PRIVATE_API void
axis_extension_thread_on_extension_group_on_deinit_done(void *self_, void *arg);

axis_RUNTIME_PRIVATE_API void axis_extension_thread_on_all_extensions_deleted(
    void *self_, void *arg);

axis_RUNTIME_PRIVATE_API void
axis_extension_thread_on_addon_create_extension_done(void *self_, void *arg);

axis_RUNTIME_PRIVATE_API void
axis_extension_thread_on_addon_destroy_extension_done(void *self_, void *arg);

axis_RUNTIME_PRIVATE_API void axis_extension_thread_create_extension_instance(
    void *self_, void *arg);

axis_RUNTIME_PRIVATE_API void axis_extension_thread_destroy_addon_instance(
    void *self_, void *arg);
