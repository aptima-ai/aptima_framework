//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/app/on_xxx.h"

#include "include_internal/axis_runtime/addon/addon.h"
#include "axis_runtime/app/app.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"

axis_app_thread_on_addon_create_protocol_done_ctx_t *
axis_app_thread_on_addon_create_protocol_done_ctx_create(void) {
  axis_app_thread_on_addon_create_protocol_done_ctx_t *self =
      axis_MALLOC(sizeof(axis_app_thread_on_addon_create_protocol_done_ctx_t));

  self->protocol = NULL;
  self->addon_context = NULL;

  return self;
}

static void axis_app_thread_on_addon_create_protocol_done_ctx_destroy(
    axis_app_thread_on_addon_create_protocol_done_ctx_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_FREE(self);
}

void axis_app_thread_on_addon_create_protocol_done(void *self, void *arg) {
  axis_app_t *app = (axis_app_t *)self;
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Invalid argument.");

  axis_app_thread_on_addon_create_protocol_done_ctx_t *ctx =
      (axis_app_thread_on_addon_create_protocol_done_ctx_t *)arg;
  axis_ASSERT(ctx, "Invalid argument.");

  axis_protocol_t *protocol = ctx->protocol;
  axis_addon_context_t *addon_context = ctx->addon_context;

  axis_ASSERT(addon_context, "Invalid argument.");

  if (addon_context->create_instance_done_cb) {
    addon_context->create_instance_done_cb(
        app->axis_env, protocol, addon_context->create_instance_done_cb_data);
  }

  axis_addon_context_destroy(addon_context);
  axis_app_thread_on_addon_create_protocol_done_ctx_destroy(ctx);
}

axis_app_thread_on_addon_create_addon_loader_done_ctx_t *
axis_app_thread_on_addon_create_addon_loader_done_ctx_create(void) {
  axis_app_thread_on_addon_create_addon_loader_done_ctx_t *self = axis_MALLOC(
      sizeof(axis_app_thread_on_addon_create_addon_loader_done_ctx_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  self->addon_loader = NULL;
  self->addon_context = NULL;

  return self;
}

static void axis_app_thread_on_addon_create_addon_loader_done_ctx_destroy(
    axis_app_thread_on_addon_create_addon_loader_done_ctx_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_FREE(self);
}

void axis_app_thread_on_addon_create_addon_loader_done(void *self, void *arg) {
  axis_app_t *app = (axis_app_t *)self;
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Invalid argument.");

  axis_app_thread_on_addon_create_addon_loader_done_ctx_t *ctx =
      (axis_app_thread_on_addon_create_addon_loader_done_ctx_t *)arg;
  axis_ASSERT(ctx, "Invalid argument.");

  axis_addon_loader_t *addon_loader = ctx->addon_loader;
  axis_addon_context_t *addon_context = ctx->addon_context;

  axis_ASSERT(addon_context, "Invalid argument.");

  if (addon_context->create_instance_done_cb) {
    addon_context->create_instance_done_cb(
        app->axis_env, addon_loader,
        addon_context->create_instance_done_cb_data);
  }

  axis_addon_context_destroy(addon_context);
  axis_app_thread_on_addon_create_addon_loader_done_ctx_destroy(ctx);
}
