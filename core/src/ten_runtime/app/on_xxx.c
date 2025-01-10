//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/app/on_xxx.h"

#include "include_internal/ten_runtime/addon/addon.h"
#include "ten_runtime/app/app.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/memory.h"

ten_app_thread_on_addon_create_protocol_done_ctx_t *
ten_app_thread_on_addon_create_protocol_done_ctx_create(void) {
  ten_app_thread_on_addon_create_protocol_done_ctx_t *self =
      TEN_MALLOC(sizeof(ten_app_thread_on_addon_create_protocol_done_ctx_t));

  self->protocol = NULL;
  self->addon_context = NULL;

  return self;
}

static void ten_app_thread_on_addon_create_protocol_done_ctx_destroy(
    ten_app_thread_on_addon_create_protocol_done_ctx_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_FREE(self);
}

void ten_app_thread_on_addon_create_protocol_done(void *self, void *arg) {
  ten_app_t *app = (ten_app_t *)self;
  TEN_ASSERT(app && ten_app_check_integrity(app, true), "Invalid argument.");

  ten_app_thread_on_addon_create_protocol_done_ctx_t *ctx =
      (ten_app_thread_on_addon_create_protocol_done_ctx_t *)arg;
  TEN_ASSERT(ctx, "Invalid argument.");

  ten_protocol_t *protocol = ctx->protocol;
  ten_addon_context_t *addon_context = ctx->addon_context;

  TEN_ASSERT(addon_context, "Invalid argument.");

  if (addon_context->create_instance_done_cb) {
    addon_context->create_instance_done_cb(
        app->ten_env, protocol, addon_context->create_instance_done_cb_data);
  }

  ten_addon_context_destroy(addon_context);
  ten_app_thread_on_addon_create_protocol_done_ctx_destroy(ctx);
}

ten_app_thread_on_addon_create_addon_loader_done_ctx_t *
ten_app_thread_on_addon_create_addon_loader_done_ctx_create(void) {
  ten_app_thread_on_addon_create_addon_loader_done_ctx_t *self = TEN_MALLOC(
      sizeof(ten_app_thread_on_addon_create_addon_loader_done_ctx_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  self->addon_loader = NULL;
  self->addon_context = NULL;

  return self;
}

static void ten_app_thread_on_addon_create_addon_loader_done_ctx_destroy(
    ten_app_thread_on_addon_create_addon_loader_done_ctx_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_FREE(self);
}

void ten_app_thread_on_addon_create_addon_loader_done(void *self, void *arg) {
  ten_app_t *app = (ten_app_t *)self;
  TEN_ASSERT(app && ten_app_check_integrity(app, true), "Invalid argument.");

  ten_app_thread_on_addon_create_addon_loader_done_ctx_t *ctx =
      (ten_app_thread_on_addon_create_addon_loader_done_ctx_t *)arg;
  TEN_ASSERT(ctx, "Invalid argument.");

  ten_addon_loader_t *addon_loader = ctx->addon_loader;
  ten_addon_context_t *addon_context = ctx->addon_context;

  TEN_ASSERT(addon_context, "Invalid argument.");

  if (addon_context->create_instance_done_cb) {
    addon_context->create_instance_done_cb(
        app->ten_env, addon_loader,
        addon_context->create_instance_done_cb_data);
  }

  ten_addon_context_destroy(addon_context);
  ten_app_thread_on_addon_create_addon_loader_done_ctx_destroy(ctx);
}
