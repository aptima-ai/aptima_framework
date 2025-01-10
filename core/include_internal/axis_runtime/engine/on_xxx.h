//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/addon/addon.h"

typedef struct axis_env_t axis_env_t;
typedef struct axis_engine_t axis_engine_t;
typedef struct axis_extension_thread_t axis_extension_thread_t;
typedef struct axis_protocol_t axis_protocol_t;

typedef struct axis_engine_thread_on_addon_create_protocol_done_ctx_t {
  axis_protocol_t *protocol;
  axis_addon_context_t *addon_context;
} axis_engine_thread_on_addon_create_protocol_done_ctx_t;

axis_RUNTIME_PRIVATE_API void axis_engine_on_extension_thread_closed(void *self_,
                                                                   void *arg);

axis_RUNTIME_PRIVATE_API void axis_engine_on_addon_create_extension_group_done(
    void *self_, void *arg);

axis_RUNTIME_PRIVATE_API void axis_engine_on_addon_destroy_extension_group_done(
    void *self_, void *arg);

axis_RUNTIME_PRIVATE_API axis_engine_thread_on_addon_create_protocol_done_ctx_t *
axis_engine_thread_on_addon_create_protocol_done_ctx_create(void);

axis_RUNTIME_PRIVATE_API void axis_engine_thread_on_addon_create_protocol_done(
    void *self, void *arg);
