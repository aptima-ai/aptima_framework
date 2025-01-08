//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/engine/on_xxx.h"

#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/engine/msg_interface/common.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension_context/extension_context.h"
#include "include_internal/axis_runtime/extension_context/axis_env/on_xxx.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/extension_thread/extension_thread.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/macro/memory.h"
#include "axis_utils/sanitizer/thread_check.h"

void axis_engine_on_extension_thread_closed(void *self_, void *arg) {
  axis_engine_t *self = self_;
  axis_ASSERT(self && axis_engine_check_integrity(self, true),
             "Should not happen.");

  axis_extension_thread_t *extension_thread = arg;
  axis_ASSERT(extension_thread &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: this function does not access this
                 // extension_thread, we just check if the arg is an
                 // axis_extension_thread_t.
                 axis_extension_thread_check_integrity(extension_thread, false),
             "Should not happen.");

  axis_LOGD("[%s] Waiting for extension thread (%p) be reclaimed.",
           axis_engine_get_id(self, true), extension_thread);
  axis_UNUSED int rc =
      axis_thread_join(axis_sanitizer_thread_check_get_belonging_thread(
                          &extension_thread->thread_check),
                      -1);
  axis_ASSERT(!rc, "Should not happen.");
  axis_LOGD("[%s] Extension thread (%p) is reclaimed.",
           axis_engine_get_id(self, true), extension_thread);

  // Extension thread is disappear, so we migrate the extension_group and
  // extension_thread to the engine thread now.
  axis_sanitizer_thread_check_inherit_from(&extension_thread->thread_check,
                                          &self->thread_check);
  axis_sanitizer_thread_check_inherit_from(
      &extension_thread->extension_group->thread_check, &self->thread_check);
  axis_sanitizer_thread_check_inherit_from(
      &extension_thread->extension_group->axis_env->thread_check,
      &self->thread_check);

  self->extension_context->extension_threads_cnt_of_closed++;

  axis_extension_context_on_close(self->extension_context);
}

void axis_engine_on_addon_create_extension_group_done(void *self_, void *arg) {
  axis_engine_t *self = self_;
  axis_ASSERT(self && axis_engine_check_integrity(self, true),
             "Should not happen.");

  axis_extension_context_on_addon_create_extension_group_done_ctx_t *ctx = arg;
  axis_ASSERT(ctx, "Should not happen.");

  axis_extension_group_t *extension_group = ctx->extension_group;
  axis_ASSERT(extension_group &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: The extension thread has not been created
                 // yet, so it is thread safe
                 axis_extension_group_check_integrity(extension_group, false),
             "Should not happen.");

  axis_extension_context_on_addon_create_extension_group_done(
      self->axis_env, extension_group, ctx->addon_context);

  axis_extension_context_on_addon_create_extension_group_done_ctx_destroy(ctx);
}

void axis_engine_on_addon_destroy_extension_group_done(void *self_, void *arg) {
  axis_engine_t *self = self_;
  axis_ASSERT(self && axis_engine_check_integrity(self, true),
             "Should not happen.");

  axis_addon_context_t *addon_context = arg;
  axis_ASSERT(addon_context, "Should not happen.");

  // This happens on the engine thread, so it's thread safe.

  axis_extension_context_on_addon_destroy_extension_group_done(self->axis_env,
                                                              addon_context);
}

axis_engine_thread_on_addon_create_protocol_done_ctx_t *
axis_engine_thread_on_addon_create_protocol_done_ctx_create(void) {
  axis_engine_thread_on_addon_create_protocol_done_ctx_t *self =
      axis_MALLOC(sizeof(axis_engine_thread_on_addon_create_protocol_done_ctx_t));

  self->protocol = NULL;
  self->addon_context = NULL;

  return self;
}

static void axis_engine_thread_on_addon_create_protocol_done_ctx_destroy(
    axis_engine_thread_on_addon_create_protocol_done_ctx_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_FREE(self);
}

void axis_engine_thread_on_addon_create_protocol_done(void *self, void *arg) {
  axis_engine_t *engine = self;
  axis_ASSERT(engine && axis_engine_check_integrity(engine, true),
             "Should not happen.");

  axis_engine_thread_on_addon_create_protocol_done_ctx_t *ctx = arg;
  axis_ASSERT(ctx, "Should not happen.");

  axis_protocol_t *protocol = ctx->protocol;
  axis_addon_context_t *addon_context = ctx->addon_context;
  axis_ASSERT(addon_context, "Should not happen.");

  if (addon_context->create_instance_done_cb) {
    addon_context->create_instance_done_cb(
        engine->axis_env, protocol, addon_context->create_instance_done_cb_data);
  }

  axis_addon_context_destroy(addon_context);
  axis_engine_thread_on_addon_create_protocol_done_ctx_destroy(ctx);
}
