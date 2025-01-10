//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/app/app.h"

typedef struct axis_protocol_t axis_protocol_t;
typedef struct axis_addon_loader_t axis_addon_loader_t;

typedef struct axis_app_thread_on_addon_create_protocol_done_ctx_t {
  axis_protocol_t *protocol;
  axis_addon_context_t *addon_context;
} axis_app_thread_on_addon_create_protocol_done_ctx_t;

typedef struct axis_app_thread_on_addon_create_addon_loader_done_ctx_t {
  axis_addon_loader_t *addon_loader;
  axis_addon_context_t *addon_context;
} axis_app_thread_on_addon_create_addon_loader_done_ctx_t;

axis_RUNTIME_PRIVATE_API axis_app_thread_on_addon_create_protocol_done_ctx_t *
axis_app_thread_on_addon_create_protocol_done_ctx_create(void);

axis_RUNTIME_PRIVATE_API void axis_app_thread_on_addon_create_protocol_done(
    void *self, void *arg);

axis_RUNTIME_PRIVATE_API axis_app_thread_on_addon_create_addon_loader_done_ctx_t *
axis_app_thread_on_addon_create_addon_loader_done_ctx_create(void);

axis_RUNTIME_PRIVATE_API void axis_app_thread_on_addon_create_addon_loader_done(
    void *self, void *arg);

axis_RUNTIME_PRIVATE_API void axis_app_on_all_addon_loaders_created(
    axis_app_t *self);
