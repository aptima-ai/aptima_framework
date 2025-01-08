//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/addon/axis_env/on_xxx.h"

#include <stdlib.h>

#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/addon/addon_host.h"
#include "include_internal/axis_runtime/addon/common/store.h"
#include "include_internal/axis_runtime/addon_loader/addon_loader.h"
#include "include_internal/axis_runtime/app/on_xxx.h"
#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/engine/on_xxx.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension_context/axis_env/on_xxx.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/extension_thread/extension_thread.h"
#include "include_internal/axis_runtime/extension_thread/on_xxx.h"
#include "include_internal/axis_runtime/metadata/metadata_info.h"
#include "include_internal/axis_runtime/protocol/protocol.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/addon/addon.h"
#include "axis_runtime/app/app.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"

void axis_addon_on_init_done(axis_env_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  axis_ASSERT(axis_env_check_integrity(self, false), "Invalid use of axis_env %p.",
             self);

  axis_addon_host_t *addon_host = axis_env_get_attached_addon(self);
  axis_ASSERT(addon_host && axis_addon_host_check_integrity(addon_host),
             "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  bool rc = axis_handle_manifest_info_when_on_configure_done(
      &addon_host->manifest_info, NULL, &addon_host->manifest, &err);
  if (!rc) {
    axis_LOGW("Failed to load addon manifest data, FATAL ERROR.");
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    exit(EXIT_FAILURE);
  }

  rc = axis_handle_property_info_when_on_configure_done(
      &addon_host->property_info, NULL, &addon_host->property, &err);
  if (!rc) {
    axis_LOGW("Failed to load addon property data, FATAL ERROR.");
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    exit(EXIT_FAILURE);
  }

  axis_value_t *manifest_name_value =
      axis_value_object_peek(&addon_host->manifest, axis_STR_NAME);

  const char *manifest_name = NULL;
  if (manifest_name_value) {
    manifest_name = axis_value_peek_raw_str(manifest_name_value, &err);
  }

  axis_error_deinit(&err);

  if (manifest_name) {
    axis_ASSERT(manifest_name, "Should not happen.");

    if (axis_string_len(&addon_host->name) &&
        !axis_string_is_equal_c_str(&addon_host->name, manifest_name)) {
      axis_LOGW(
          "The registered extension name (%s) is not equal to the name (%s) in "
          "the manifest.",
          axis_string_get_raw_str(&addon_host->name), manifest_name);

      // Get 'name' from manifest, and check the consistency between the name
      // specified in the argument, and the name specified in the manifest.
      //
      // The name in the manifest could be checked by the TEN store to ensure
      // the uniqueness of the name.
      axis_ASSERT(0, "Should not happen.");
    }

    // If an extension defines an extension name in its manifest file, TEN
    // runtime would use that name instead of the name specified in the codes to
    // register it to the extension store.
    if (strlen(manifest_name)) {
      axis_string_set_from_c_str(&addon_host->name, manifest_name,
                                strlen(manifest_name));
    }
  }
}

void axis_addon_on_deinit_done(axis_env_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  axis_ASSERT(axis_env_check_integrity(self, false), "Invalid use of axis_env %p.",
             self);

  axis_ASSERT(self->attach_to == axis_ENV_ATTACH_TO_ADDON, "Should not happen.");

  axis_addon_host_t *addon_host = axis_env_get_attached_addon(self);
  axis_ASSERT(addon_host && axis_addon_host_check_integrity(addon_host),
             "Should not happen.");

  if (addon_host->addon->on_destroy) {
    addon_host->addon->on_destroy(addon_host->addon);
  }

  axis_addon_host_destroy(addon_host);
}

static void axis_extension_addon_on_create_instance_done(axis_env_t *self,
                                                        void *instance,
                                                        void *context) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  axis_ASSERT(axis_env_check_integrity(self, false), "Invalid use of axis_env %p.",
             self);

  axis_ASSERT(self->attach_to == axis_ENV_ATTACH_TO_ADDON, "Should not happen.");

  axis_addon_host_t *addon_host = axis_env_get_attached_addon(self);
  axis_ASSERT(addon_host && axis_addon_host_check_integrity(addon_host),
             "Should not happen.");

  axis_addon_context_t *addon_context = (axis_addon_context_t *)context;
  axis_ASSERT(addon_context, "Invalid argument.");

  axis_env_t *caller_ten = addon_context->caller_axis_env;
  axis_ASSERT(caller_ten, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  axis_ASSERT(axis_env_check_integrity(caller_ten, false),
             "Invalid use of axis_env %p.", caller_ten);

  axis_ASSERT(addon_context->create_instance_done_cb, "Should not happen.");

  switch (caller_ten->attach_to) {
    case axis_ENV_ATTACH_TO_EXTENSION_GROUP: {
      axis_extension_t *extension = instance;
      // axis_NOLINTNEXTLINE(thread-check)
      // thread-check: Maybe in the thread other than the extension thread
      // (ex: JS main thread), and all the function calls in this case are
      // thread safe.
      axis_ASSERT(extension && axis_extension_check_integrity(extension, false),
                 "Should not happen.");

      axis_extension_set_addon(extension, addon_host);

      axis_extension_group_t *extension_group =
          axis_env_get_attached_extension_group(caller_ten);
      axis_ASSERT(
          extension_group &&
              // axis_NOLINTNEXTLINE(thread-check)
              // thread-check: Maybe in the thread other than the extension
              // thread (ex: JS main thread), and all the function calls in
              // this case are thread safe.
              axis_extension_group_check_integrity(extension_group, false),
          "Invalid argument.");

      axis_extension_thread_t *extension_thread =
          extension_group->extension_thread;
      axis_ASSERT(
          extension_thread &&
              // axis_NOLINTNEXTLINE(thread-check)
              // thread-check: Maybe in the thread other than the extension
              // thread (ex: JS main thread), and all the function calls in
              // this case are thread safe.
              axis_extension_thread_check_integrity(extension_thread, false),
          "Should not happen.");

      axis_extension_thread_on_addon_create_extension_done_ctx_t *ctx =
          axis_extension_thread_on_addon_create_extension_done_ctx_create();

      ctx->extension = extension;
      ctx->addon_context = addon_context;

      axis_runloop_post_task_tail(
          axis_extension_group_get_attached_runloop(extension_group),
          axis_extension_thread_on_addon_create_extension_done, extension_thread,
          ctx);
      break;
    }

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }
}

static void axis_extension_group_addon_on_create_instance_done(axis_env_t *self,
                                                              void *instance,
                                                              void *context) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  axis_ASSERT(axis_env_check_integrity(self, false), "Invalid use of axis_env %p.",
             self);

  axis_ASSERT(self->attach_to == axis_ENV_ATTACH_TO_ADDON, "Should not happen.");

  axis_addon_host_t *addon_host = axis_env_get_attached_addon(self);
  axis_ASSERT(addon_host && axis_addon_host_check_integrity(addon_host),
             "Should not happen.");

  axis_addon_context_t *addon_context = (axis_addon_context_t *)context;
  axis_ASSERT(addon_context, "Invalid argument.");

  axis_env_t *caller_ten = addon_context->caller_axis_env;
  axis_ASSERT(caller_ten, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  axis_ASSERT(axis_env_check_integrity(caller_ten, false),
             "Invalid use of axis_env %p.", caller_ten);

  axis_ASSERT(addon_context->create_instance_done_cb, "Should not happen.");

  switch (caller_ten->attach_to) {
    case axis_ENV_ATTACH_TO_ENGINE: {
      axis_extension_group_t *extension_group = instance;
      axis_ASSERT(
          extension_group &&
              // axis_NOLINTNEXTLINE(thread-check)
              // thread-check: The extension thread has not been created yet, so
              // it is thread safe at this time.
              axis_extension_group_check_integrity(extension_group, false),
          "Invalid argument.");
      axis_extension_group_set_addon(extension_group, addon_host);

      axis_engine_t *engine = axis_env_get_attached_engine(caller_ten);
      // axis_NOLINTNEXTLINE(thread-check)
      // thread-check: Maybe in the thread other than the engine thread (ex: JS
      // main thread), and all the function calls in this case are thread safe.
      axis_engine_check_integrity(engine, false);

      axis_extension_context_on_addon_create_extension_group_done_ctx_t *ctx =
          axis_extension_context_on_addon_create_extension_group_done_ctx_create();

      ctx->extension_group = extension_group;
      ctx->addon_context = addon_context;

      axis_runloop_post_task_tail(
          axis_engine_get_attached_runloop(engine),
          axis_engine_on_addon_create_extension_group_done, engine, ctx);
      break;
    }

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }
}

static void axis_protocol_addon_on_create_instance_done(axis_env_t *self,
                                                       void *instance,
                                                       void *context) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  axis_ASSERT(axis_env_check_integrity(self, false), "Invalid use of axis_env %p.",
             self);
  axis_ASSERT(self->attach_to == axis_ENV_ATTACH_TO_ADDON, "Should not happen.");

  axis_addon_host_t *addon_host = axis_env_get_attached_addon(self);
  axis_ASSERT(addon_host && axis_addon_host_check_integrity(addon_host),
             "Should not happen.");
  axis_ASSERT(addon_host->type == axis_ADDON_TYPE_PROTOCOL, "Should not happen.");

  axis_protocol_t *protocol = instance;
  axis_ASSERT(protocol && axis_protocol_check_integrity(protocol, false),
             "Should not happen.");

  axis_sanitizer_thread_check_set_belonging_thread_to_current_thread(
      &protocol->thread_check);
  axis_ASSERT(axis_protocol_check_integrity(protocol, true),
             "Should not happen.");

  if (!protocol->addon_host) {
    axis_protocol_set_addon(protocol, addon_host);
  }

  axis_addon_context_t *addon_context = (axis_addon_context_t *)context;
  if (!addon_context) {
    return;
  }

  axis_env_t *caller_ten = addon_context->caller_axis_env;
  axis_ASSERT(caller_ten, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(caller_ten, true),
             "Invalid use of axis_env %p.", caller_ten);

  axis_ASSERT(addon_context->create_instance_done_cb, "Should not happen.");

  switch (caller_ten->attach_to) {
    case axis_ENV_ATTACH_TO_ENGINE: {
      axis_engine_t *engine = axis_env_get_attached_engine(caller_ten);
      axis_ASSERT(engine && axis_engine_check_integrity(engine, true),
                 "Should not happen.");

      axis_engine_thread_on_addon_create_protocol_done_ctx_t *ctx =
          axis_engine_thread_on_addon_create_protocol_done_ctx_create();

      ctx->protocol = protocol;
      ctx->addon_context = addon_context;

      axis_runloop_post_task_tail(
          axis_engine_get_attached_runloop(engine),
          axis_engine_thread_on_addon_create_protocol_done, engine, ctx);
      break;
    }

    case axis_ENV_ATTACH_TO_APP: {
      axis_app_t *app = axis_env_get_attached_app(caller_ten);
      axis_ASSERT(app && axis_app_check_integrity(app, true),
                 "Should not happen.");

      axis_app_thread_on_addon_create_protocol_done_ctx_t *ctx =
          axis_app_thread_on_addon_create_protocol_done_ctx_create();

      ctx->protocol = instance;
      ctx->addon_context = addon_context;

      axis_runloop_post_task_tail(axis_app_get_attached_runloop(app),
                                 axis_app_thread_on_addon_create_protocol_done,
                                 app, ctx);
      break;
    }

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }
}

static void axis_addon_loader_addon_on_create_instance_done(axis_env_t *self,
                                                           void *instance,
                                                           void *context) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  axis_ASSERT(axis_env_check_integrity(self, false), "Invalid use of axis_env %p.",
             self);
  axis_ASSERT(self->attach_to == axis_ENV_ATTACH_TO_ADDON, "Should not happen.");

  axis_addon_host_t *addon_host = axis_env_get_attached_addon(self);
  axis_ASSERT(addon_host && axis_addon_host_check_integrity(addon_host),
             "Should not happen.");
  axis_ASSERT(addon_host->type == axis_ADDON_TYPE_ADDON_LOADER,
             "Should not happen.");

  axis_addon_loader_t *addon_loader = instance;
  axis_ASSERT(addon_loader, "Should not happen.");

  addon_loader->addon_host = addon_host;

  axis_addon_context_t *addon_context = (axis_addon_context_t *)context;
  if (!addon_context) {
    return;
  }

  axis_env_t *caller_ten = addon_context->caller_axis_env;
  axis_ASSERT(caller_ten, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(caller_ten, true),
             "Invalid use of axis_env %p.", caller_ten);

  axis_ASSERT(addon_context->create_instance_done_cb, "Should not happen.");

  switch (caller_ten->attach_to) {
    case axis_ENV_ATTACH_TO_APP: {
      axis_app_t *app = axis_env_get_attached_app(caller_ten);
      axis_ASSERT(app && axis_app_check_integrity(app, true),
                 "Should not happen.");

      axis_app_thread_on_addon_create_addon_loader_done_ctx_t *ctx =
          axis_app_thread_on_addon_create_addon_loader_done_ctx_create();

      ctx->addon_loader = instance;
      ctx->addon_context = addon_context;

      axis_runloop_post_task_tail(
          axis_app_get_attached_runloop(app),
          axis_app_thread_on_addon_create_addon_loader_done, app, ctx);
      break;
    }

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }
}

void axis_addon_on_create_instance_done(axis_env_t *self, void *instance,
                                       void *context) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  axis_ASSERT(axis_env_check_integrity(self, false), "Invalid use of axis_env %p.",
             self);

  axis_ASSERT(self->attach_to == axis_ENV_ATTACH_TO_ADDON, "Should not happen.");

  axis_addon_host_t *addon_host = axis_env_get_attached_addon(self);
  axis_ASSERT(addon_host && axis_addon_host_check_integrity(addon_host),
             "Should not happen.");

  switch (addon_host->type) {
    case axis_ADDON_TYPE_EXTENSION:
      axis_extension_addon_on_create_instance_done(self, instance, context);
      break;
    case axis_ADDON_TYPE_EXTENSION_GROUP:
      axis_extension_group_addon_on_create_instance_done(self, instance,
                                                        context);
      break;
    case axis_ADDON_TYPE_PROTOCOL:
      axis_protocol_addon_on_create_instance_done(self, instance, context);
      break;
    case axis_ADDON_TYPE_ADDON_LOADER:
      axis_addon_loader_addon_on_create_instance_done(self, instance, context);
      break;

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }
}

void axis_addon_on_destroy_instance_done(axis_env_t *self, void *context) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  axis_ASSERT(axis_env_check_integrity(self, false), "Invalid use of axis_env %p.",
             self);

  axis_ASSERT(self->attach_to == axis_ENV_ATTACH_TO_ADDON, "Should not happen.");

  axis_addon_host_t *addon_host = axis_env_get_attached_addon(self);
  axis_ASSERT(addon_host && axis_addon_host_check_integrity(addon_host),
             "Should not happen.");

  axis_addon_context_t *addon_context = (axis_addon_context_t *)context;
  if (!addon_context) {
    // If the addon_context is NULL, it means that the result of destroy does
    // not need to be handled, so we can return directly.

    // TODO(xilin): For the destroy of the protocol, no `addon_context`
    // parameter is passed in, which means there’s also no `caller rte`
    // parameter. Since there’s no `caller rte` parameter, there’s no action to
    // enqueue a task to the thread where the `caller rte` is located. This is
    // because, on one hand, there’s currently no place that requires waiting
    // for protocol destroy to complete, and on the other hand, the app/engine
    // thread may have already exited at this point. Therefore, the
    // `on_destroy_instance_done` of the protocol is actually not called in the
    // current implementation.

    // TODO(xilin): It may be necessary to adjust the destruction process so
    // that the destroy of the protocol executes on the correct thread.
    return;
  }

  axis_ASSERT(addon_context, "Invalid argument.");

  axis_env_t *caller_ten = addon_context->caller_axis_env;
  axis_ASSERT(caller_ten, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  axis_ASSERT(axis_env_check_integrity(caller_ten, false),
             "Invalid use of axis_env %p.", caller_ten);

  axis_ASSERT(addon_context->destroy_instance_done_cb, "Should not happen.");

  switch (caller_ten->attach_to) {
    case axis_ENV_ATTACH_TO_ENGINE: {
      axis_engine_t *engine = axis_env_get_attached_engine(caller_ten);
      axis_ASSERT(engine &&
                     // axis_NOLINTNEXTLINE(thread-check)
                     // thread-check: Maybe in the thread other than the
                     // engine thread (ex: JS main thread), and all the
                     // function calls in this case are thread safe.
                     axis_engine_check_integrity(engine, false),
                 "Should not happen.");

      axis_runloop_post_task_tail(
          axis_engine_get_attached_runloop(engine),
          axis_engine_on_addon_destroy_extension_group_done, engine,
          addon_context);
      break;
    }

    case axis_ENV_ATTACH_TO_EXTENSION_GROUP: {
      axis_extension_group_t *extension_group =
          axis_env_get_attached_extension_group(caller_ten);
      axis_ASSERT(
          extension_group &&
              // axis_NOLINTNEXTLINE(thread-check)
              // thread-check: Maybe in the thread other than the engine
              // thread (ex: JS main thread), and all the function calls in
              // this case are thread safe.
              axis_extension_group_check_integrity(extension_group, false),
          "Should not happen.");

      axis_extension_thread_t *extension_thread =
          extension_group->extension_thread;
      axis_ASSERT(
          extension_thread &&
              // axis_NOLINTNEXTLINE(thread-check)
              // thread-check: Maybe in the thread other than the engine
              // thread (ex: JS main thread), and all the function calls in
              // this case are thread safe.
              axis_extension_thread_check_integrity(extension_thread, false),
          "Should not happen.");

      axis_runloop_post_task_tail(
          axis_extension_group_get_attached_runloop(extension_group),
          axis_extension_thread_on_addon_destroy_extension_done,
          extension_thread, addon_context);
      break;
    }

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }
}
