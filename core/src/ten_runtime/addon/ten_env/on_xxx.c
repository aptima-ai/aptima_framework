//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/addon/ten_env/on_xxx.h"

#include <stdlib.h>

#include "include_internal/ten_runtime/addon/addon.h"
#include "include_internal/ten_runtime/addon/common/store.h"
#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/engine/engine.h"
#include "include_internal/ten_runtime/engine/on_xxx.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension_context/ten_env/on_xxx.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/extension_thread/extension_thread.h"
#include "include_internal/ten_runtime/extension_thread/on_xxx.h"
#include "include_internal/ten_runtime/metadata/metadata_info.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

void ten_addon_on_init_done(ten_env_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  TEN_ASSERT(ten_env_check_integrity(self, false), "Invalid use of ten_env %p.",
             self);

  ten_addon_host_t *addon_host = ten_env_get_attached_addon(self);
  TEN_ASSERT(addon_host && ten_addon_host_check_integrity(addon_host),
             "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  bool rc = ten_handle_manifest_info_when_on_configure_done(
      &addon_host->manifest_info, NULL, &addon_host->manifest, &err);
  if (!rc) {
    TEN_LOGW("Failed to load addon manifest data, FATAL ERROR.");
    exit(EXIT_FAILURE);
  }

  rc = ten_handle_property_info_when_on_configure_done(
      &addon_host->property_info, NULL, &addon_host->property, &err);
  if (!rc) {
    TEN_LOGW("Failed to load addon property data, FATAL ERROR.");
    exit(EXIT_FAILURE);
  }

  ten_error_deinit(&err);

  ten_value_t *manifest_name_value =
      ten_value_object_peek(&addon_host->manifest, TEN_STR_NAME);

  const char *manifest_name = NULL;
  if (manifest_name_value) {
    manifest_name = ten_value_peek_raw_str(manifest_name_value);
  }

  if (manifest_name) {
    TEN_ASSERT(manifest_name, "Should not happen.");

    if (ten_string_len(&addon_host->name) &&
        !ten_string_is_equal_c_str(&addon_host->name, manifest_name)) {
      TEN_LOGW(
          "The registered extension name (%s) is not equal to the name (%s) in "
          "the manifest.",
          ten_string_get_raw_str(&addon_host->name), manifest_name);

      // Get 'name' from manifest, and check the consistency between the name
      // specified in the argument, and the name specified in the manifest.
      //
      // The name in the manifest could be checked by the TEN store to ensure
      // the uniqueness of the name.
      TEN_ASSERT(0, "Should not happen.");
    }

    // If an extension defines an extension name in its manifest file, TEN
    // runtime would use that name instead of the name specified in the codes to
    // register it to the extension store.
    if (strlen(manifest_name)) {
      ten_string_init_from_c_str(&addon_host->name, manifest_name,
                                 strlen(manifest_name));
    }
  }

  rc = ten_addon_store_add(addon_host->store, addon_host);
  if (!rc) {
    TEN_ASSERT(0, "Should not happen.");
    exit(EXIT_FAILURE);
  }
}

void ten_addon_on_deinit_done(ten_env_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  TEN_ASSERT(ten_env_check_integrity(self, false), "Invalid use of ten_env %p.",
             self);

  TEN_ASSERT(self->attach_to == TEN_ENV_ATTACH_TO_ADDON, "Should not happen.");

  ten_addon_host_t *addon_host = ten_env_get_attached_addon(self);
  TEN_ASSERT(addon_host && ten_addon_host_check_integrity(addon_host),
             "Should not happen.");

  ten_addon_host_destroy(addon_host);
}

static void ten_addon_extension_on_create_instance_done(ten_env_t *self,
                                                        void *instance,
                                                        void *context) {
  TEN_ASSERT(self, "Invalid argument.");
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  TEN_ASSERT(ten_env_check_integrity(self, false), "Invalid use of ten_env %p.",
             self);

  TEN_ASSERT(self->attach_to == TEN_ENV_ATTACH_TO_ADDON, "Should not happen.");

  ten_addon_host_t *addon_host = ten_env_get_attached_addon(self);
  TEN_ASSERT(addon_host && ten_addon_host_check_integrity(addon_host),
             "Should not happen.");

  ten_addon_context_t *addon_context = (ten_addon_context_t *)context;
  TEN_ASSERT(addon_context, "Invalid argument.");

  ten_env_t *caller_ten = addon_context->caller_ten;
  TEN_ASSERT(caller_ten, "Invalid argument.");
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  TEN_ASSERT(ten_env_check_integrity(caller_ten, false),
             "Invalid use of ten_env %p.", caller_ten);

  TEN_ASSERT(addon_context->addon_on_create_instance_async_cb,
             "Should not happen.");

  switch (caller_ten->attach_to) {
    case TEN_ENV_ATTACH_TO_EXTENSION_GROUP: {
      ten_extension_t *extension = instance;
      // TEN_NOLINTNEXTLINE(thread-check)
      // thread-check: Maybe in the thread other than the extension thread
      // (ex: JS main thread), and all the function calls in this case are
      // thread safe.
      TEN_ASSERT(extension && ten_extension_check_integrity(extension, false),
                 "Should not happen.");

      ten_extension_set_addon(extension, addon_host);

      ten_extension_group_t *extension_group =
          ten_env_get_attached_extension_group(caller_ten);
      TEN_ASSERT(
          extension_group &&
              // TEN_NOLINTNEXTLINE(thread-check)
              // thread-check: Maybe in the thread other than the extension
              // thread (ex: JS main thread), and all the function calls in
              // this case are thread safe.
              ten_extension_group_check_integrity(extension_group, false),
          "Invalid argument.");

      ten_extension_thread_t *extension_thread =
          extension_group->extension_thread;
      TEN_ASSERT(
          extension_thread &&
              // TEN_NOLINTNEXTLINE(thread-check)
              // thread-check: Maybe in the thread other than the extension
              // thread (ex: JS main thread), and all the function calls in
              // this case are thread safe.
              ten_extension_thread_check_integrity(extension_thread, false),
          "Should not happen.");

      ten_extension_thread_on_addon_create_extension_done_info_t *info =
          ten_extension_thread_on_addon_create_extension_done_info_create();

      info->extension = extension;
      info->addon_context = addon_context;

      ten_runloop_post_task_tail(
          ten_extension_group_get_attached_runloop(extension_group),
          ten_extension_thread_on_addon_create_extension_done, extension_thread,
          info);
      break;
    }

    default:
      TEN_ASSERT(0, "Should not happen.");
      break;
  }
}

static void ten_addon_extension_group_on_create_instance_done(ten_env_t *self,
                                                              void *instance,
                                                              void *context) {
  TEN_ASSERT(self, "Invalid argument.");
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  TEN_ASSERT(ten_env_check_integrity(self, false), "Invalid use of ten_env %p.",
             self);

  TEN_ASSERT(self->attach_to == TEN_ENV_ATTACH_TO_ADDON, "Should not happen.");

  ten_addon_host_t *addon_host = ten_env_get_attached_addon(self);
  TEN_ASSERT(addon_host && ten_addon_host_check_integrity(addon_host),
             "Should not happen.");

  ten_addon_context_t *addon_context = (ten_addon_context_t *)context;
  TEN_ASSERT(addon_context, "Invalid argument.");

  ten_env_t *caller_ten = addon_context->caller_ten;
  TEN_ASSERT(caller_ten, "Invalid argument.");
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  TEN_ASSERT(ten_env_check_integrity(caller_ten, false),
             "Invalid use of ten_env %p.", caller_ten);

  TEN_ASSERT(addon_context->addon_on_create_instance_async_cb,
             "Should not happen.");

  switch (caller_ten->attach_to) {
    case TEN_ENV_ATTACH_TO_ENGINE: {
      ten_extension_group_t *extension_group = instance;
      TEN_ASSERT(
          extension_group &&
              // TEN_NOLINTNEXTLINE(thread-check)
              // thread-check: The extension thread has not been created yet, so
              // it is thread safe at this time.
              ten_extension_group_check_integrity(extension_group, false),
          "Invalid argument.");
      ten_extension_group_set_addon(extension_group, addon_host);

      ten_engine_t *engine = ten_env_get_attached_engine(caller_ten);
      // TEN_NOLINTNEXTLINE(thread-check)
      // thread-check: Maybe in the thread other than the engine thread (ex: JS
      // main thread), and all the function calls in this case are thread safe.
      ten_engine_check_integrity(engine, false);

      ten_extension_context_on_addon_create_extension_group_done_info_t *info =
          ten_extension_context_on_addon_create_extension_group_done_info_create();

      info->extension_group = extension_group;
      info->addon_context = addon_context;

      ten_runloop_post_task_tail(
          ten_engine_get_attached_runloop(engine),
          ten_engine_on_addon_create_extension_group_done, engine, info);
      break;
    }

    default:
      TEN_ASSERT(0, "Should not happen.");
      break;
  }
}

void ten_addon_protocol_on_create_instance_done(ten_env_t *self,
                                                TEN_UNUSED void *instance,
                                                TEN_UNUSED void *context) {
  TEN_ASSERT(self, "Invalid argument.");
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  TEN_ASSERT(ten_env_check_integrity(self, false), "Invalid use of ten_env %p.",
             self);

  TEN_ASSERT(self->attach_to == TEN_ENV_ATTACH_TO_ADDON, "Should not happen.");

  ten_addon_host_t *addon_host = ten_env_get_attached_addon(self);
  TEN_ASSERT(addon_host && ten_addon_host_check_integrity(addon_host),
             "Should not happen.");
  TEN_ASSERT(addon_host->type == TEN_ADDON_TYPE_PROTOCOL, "Should not happen.");

  if (context) {
    ten_addon_context_destroy(context);
  }
}

void ten_addon_on_create_instance_done(ten_env_t *self, void *instance,
                                       void *context) {
  TEN_ASSERT(self, "Invalid argument.");
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  TEN_ASSERT(ten_env_check_integrity(self, false), "Invalid use of ten_env %p.",
             self);

  TEN_ASSERT(self->attach_to == TEN_ENV_ATTACH_TO_ADDON, "Should not happen.");

  ten_addon_host_t *addon_host = ten_env_get_attached_addon(self);
  TEN_ASSERT(addon_host && ten_addon_host_check_integrity(addon_host),
             "Should not happen.");

  switch (addon_host->type) {
    case TEN_ADDON_TYPE_EXTENSION:
      ten_addon_extension_on_create_instance_done(self, instance, context);
      break;
    case TEN_ADDON_TYPE_EXTENSION_GROUP:
      ten_addon_extension_group_on_create_instance_done(self, instance,
                                                        context);
      break;
    case TEN_ADDON_TYPE_PROTOCOL:
      ten_addon_protocol_on_create_instance_done(self, instance, context);
      break;

    default:
      TEN_ASSERT(0, "Should not happen.");
      break;
  }
}

void ten_addon_on_destroy_instance_done(ten_env_t *self, void *context) {
  TEN_ASSERT(self, "Invalid argument.");
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  TEN_ASSERT(ten_env_check_integrity(self, false), "Invalid use of ten_env %p.",
             self);

  TEN_ASSERT(self->attach_to == TEN_ENV_ATTACH_TO_ADDON, "Should not happen.");

  ten_addon_host_t *addon_host = ten_env_get_attached_addon(self);
  TEN_ASSERT(addon_host && ten_addon_host_check_integrity(addon_host),
             "Should not happen.");

  ten_addon_context_t *addon_context = (ten_addon_context_t *)context;
  TEN_ASSERT(addon_context, "Invalid argument.");

  ten_env_t *caller_ten = addon_context->caller_ten;
  TEN_ASSERT(caller_ten, "Invalid argument.");
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  TEN_ASSERT(ten_env_check_integrity(caller_ten, false),
             "Invalid use of ten_env %p.", caller_ten);

  TEN_ASSERT(addon_context->addon_on_destroy_instance_async_cb,
             "Should not happen.");

  switch (caller_ten->attach_to) {
    case TEN_ENV_ATTACH_TO_ENGINE: {
      ten_engine_t *engine = ten_env_get_attached_engine(caller_ten);
      TEN_ASSERT(engine &&
                     // TEN_NOLINTNEXTLINE(thread-check)
                     // thread-check: Maybe in the thread other than the
                     // engine thread (ex: JS main thread), and all the
                     // function calls in this case are thread safe.
                     ten_engine_check_integrity(engine, false),
                 "Should not happen.");

      ten_runloop_post_task_tail(
          ten_engine_get_attached_runloop(engine),
          ten_engine_on_addon_destroy_extension_group_done, engine,
          addon_context);
      break;
    }

    case TEN_ENV_ATTACH_TO_EXTENSION_GROUP: {
      ten_extension_group_t *extension_group =
          ten_env_get_attached_extension_group(caller_ten);
      TEN_ASSERT(
          extension_group &&
              // TEN_NOLINTNEXTLINE(thread-check)
              // thread-check: Maybe in the thread other than the engine
              // thread (ex: JS main thread), and all the function calls in
              // this case are thread safe.
              ten_extension_group_check_integrity(extension_group, false),
          "Should not happen.");

      ten_extension_thread_t *extension_thread =
          extension_group->extension_thread;
      TEN_ASSERT(
          extension_thread &&
              // TEN_NOLINTNEXTLINE(thread-check)
              // thread-check: Maybe in the thread other than the engine
              // thread (ex: JS main thread), and all the function calls in
              // this case are thread safe.
              ten_extension_thread_check_integrity(extension_thread, false),
          "Should not happen.");

      ten_runloop_post_task_tail(
          ten_extension_group_get_attached_runloop(extension_group),
          ten_extension_thread_on_addon_destroy_extension_done,
          extension_thread, addon_context);
      break;
    }

    default:
      TEN_ASSERT(0, "Should not happen.");
      break;
  }
}
