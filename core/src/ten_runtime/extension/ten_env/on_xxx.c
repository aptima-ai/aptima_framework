//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/extension/on_xxx.h"

#include "include_internal/ten_runtime/common/loc.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension/metadata.h"
#include "include_internal/ten_runtime/extension/path_timer.h"
#include "include_internal/ten_runtime/extension_thread/extension_thread.h"
#include "include_internal/ten_runtime/extension_thread/msg_interface/common.h"
#include "include_internal/ten_runtime/extension_thread/on_xxx.h"
#include "include_internal/ten_runtime/metadata/metadata_info.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "include_internal/ten_runtime/timer/timer.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/macro/check.h"

static bool ten_extension_parse_interface_schema(ten_extension_t *self,
                                                 ten_value_t *api_definition,
                                                 ten_error_t *err) {
  TEN_ASSERT(self && ten_extension_check_integrity(self, true),
             "Invalid argument.");
  TEN_ASSERT(api_definition && ten_value_check_integrity(api_definition),
             "Invalid argument.");

  bool result = ten_schema_store_set_interface_schema_definition(
      &self->schema_store, api_definition,
      ten_string_get_raw_str(&self->base_dir), err);
  if (!result) {
    TEN_LOGW("[%s] Failed to set interface schema definition: %s.",
             ten_extension_get_name(self), ten_error_errmsg(err));
  }

  return result;
}

static void ten_extension_adjust_and_validate_property_on_configure_done(
    ten_extension_t *self) {
  TEN_ASSERT(self && ten_extension_check_integrity(self, true),
             "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  bool success = ten_schema_store_adjust_properties(&self->schema_store,
                                                    &self->property, &err);
  if (!success) {
    TEN_LOGW("[%s] Failed to adjust property type: %s.",
             ten_extension_get_name(self), ten_error_errmsg(&err));
    goto done;
  }

  success = ten_schema_store_validate_properties(&self->schema_store,
                                                 &self->property, &err);
  if (!success) {
    TEN_LOGW("[%s] Invalid property: %s.", ten_extension_get_name(self),
             ten_error_errmsg(&err));
    goto done;
  }

done:
  ten_error_deinit(&err);
  if (!success) {
    TEN_ASSERT(0, "Invalid property.");
  }
}

void ten_extension_on_configure_done(ten_env_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(self, true), "Invalid use of ten_env %p.",
             self);

  ten_extension_t *extension = ten_env_get_attached_extension(self);
  TEN_ASSERT(extension, "Invalid argument.");
  TEN_ASSERT(ten_extension_check_integrity(extension, true),
             "Invalid use of extension %p.", extension);

  TEN_LOGD("[%s] on_configure() done.", ten_extension_get_name(extension));

  extension->state = TEN_EXTENSION_STATE_ON_CONFIGURE_DONE;

  ten_extension_thread_t *extension_thread = extension->extension_thread;
  TEN_ASSERT(extension_thread &&
                 ten_extension_thread_check_integrity(extension_thread, true),
             "Should not happen.");

  if (extension_thread->is_close_triggered) {
    // Do not proceed with the subsequent init/start flow, as the extension
    // thread is about to shut down.
    return;
  }

  ten_error_t err;
  ten_error_init(&err);

  bool rc = ten_handle_manifest_info_when_on_configure_done(
      &extension->manifest_info,
      ten_string_get_raw_str(ten_extension_get_base_dir(extension)),
      &extension->manifest, &err);
  if (!rc) {
    TEN_LOGW("Failed to load extension manifest data, FATAL ERROR.");
    exit(EXIT_FAILURE);
  }

  rc = ten_handle_property_info_when_on_configure_done(
      &extension->property_info,
      ten_string_get_raw_str(ten_extension_get_base_dir(extension)),
      &extension->property, &err);
  if (!rc) {
    TEN_LOGW("Failed to load extension property data, FATAL ERROR.");
    exit(EXIT_FAILURE);
  }

  rc = ten_extension_resolve_properties_in_graph(extension, &err);
  TEN_ASSERT(rc, "Failed to resolve properties in graph.");

  ten_extension_merge_properties_from_graph(extension);

  rc = ten_extension_handle_ten_namespace_properties(
      extension, extension->extension_context);
  TEN_ASSERT(rc, "[%s] Failed to handle '_ten' properties.",
             ten_string_get_raw_str(&extension->name));

  ten_value_t *api_definition = ten_metadata_init_schema_store(
      &extension->manifest, &extension->schema_store);
  if (api_definition) {
    bool success =
        ten_extension_parse_interface_schema(extension, api_definition, &err);
    TEN_ASSERT(success, "Failed to parse interface schema.");
  }

  ten_extension_adjust_and_validate_property_on_configure_done(extension);

  // Create timers for automatically cleaning expired IN_PATHs and OUT_PATHs.
  ten_timer_t *in_path_timer =
      ten_extension_create_timer_for_in_path(extension);
  ten_list_push_ptr_back(&extension->path_timers, in_path_timer, NULL);
  ten_timer_enable(in_path_timer);

  ten_timer_t *out_path_timer =
      ten_extension_create_timer_for_out_path(extension);
  ten_list_push_ptr_back(&extension->path_timers, out_path_timer, NULL);
  ten_timer_enable(out_path_timer);

  // The interface info has been resolved, and extensions might send msg out
  // during `on_start()`, so it's the best time to merge the interface info to
  // the extension_info.
  rc =
      ten_extension_determine_and_merge_all_interface_dest_extension(extension);
  TEN_ASSERT(rc, "Should not happen.");

  // Trigger the extension on_init flow.
  ten_extension_on_init(extension->ten_env);

  ten_error_deinit(&err);
}

void ten_extension_on_init_done(ten_env_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(self, true), "Invalid use of ten_env %p.",
             self);

  ten_extension_t *extension = ten_env_get_attached_extension(self);
  TEN_ASSERT(extension, "Invalid argument.");
  TEN_ASSERT(ten_extension_check_integrity(extension, true),
             "Invalid use of extension %p.", extension);

  TEN_LOGD("[%s] on_init() done.", ten_extension_get_name(extension));

  extension->state = TEN_EXTENSION_STATE_ON_INIT_DONE;

  ten_extension_thread_t *extension_thread = extension->extension_thread;
  TEN_ASSERT(extension_thread &&
                 ten_extension_thread_check_integrity(extension_thread, true),
             "Should not happen.");

  if (extension_thread->is_close_triggered) {
    return;
  }

  // Trigger on_start of extension.
  ten_extension_on_start(extension);
}

void ten_extension_on_start_done(ten_env_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(self, true), "Invalid use of ten_env %p.",
             self);

  ten_extension_t *extension = ten_env_get_attached_extension(self);
  TEN_ASSERT(extension, "Invalid argument.");
  TEN_ASSERT(ten_extension_check_integrity(extension, true),
             "Invalid use of extension %p.", extension);

  TEN_LOGI("[%s] on_start() done.", ten_extension_get_name(extension));

  extension->state = TEN_EXTENSION_STATE_ON_START_DONE;
}

void ten_extension_on_stop_done(ten_env_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(self, true), "Invalid use of ten_env %p.",
             self);

  ten_extension_t *extension = ten_env_get_attached_extension(self);
  TEN_ASSERT(extension, "Invalid argument.");
  TEN_ASSERT(ten_extension_check_integrity(extension, true),
             "Invalid use of extension %p.", extension);

  TEN_LOGI("[%s] on_stop() done.", ten_extension_get_name(extension));

  // When we reach here, it means the extension has been stopped completely.
  // Notify the extension thread about this fact.

  ten_extension_on_start_stop_deinit_done_t *on_stop_done =
      ten_extension_on_start_stop_deinit_done_create(extension);

  // Use the runloop task mechanism to ensure the operations afterwards will be
  // executed in the extension thread.
  ten_runloop_post_task_tail(ten_extension_get_attached_runloop(extension),
                             ten_extension_thread_on_extension_on_stop_done,
                             extension->extension_thread, on_stop_done);
}

void ten_extension_on_deinit_done(ten_env_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(self, true), "Invalid use of ten_env %p.",
             self);

  ten_extension_t *extension = ten_env_get_attached_extension(self);
  TEN_ASSERT(extension, "Invalid argument.");
  TEN_ASSERT(ten_extension_check_integrity(extension, true),
             "Invalid use of extension %p.", extension);

  if (!ten_list_is_empty(&self->ten_proxy_list)) {
    // There is still the presence of ten_env_proxy, so the closing process
    // cannot continue.
    TEN_LOGI(
        "[%s] Failed to on_deinit_done() because of existed ten_env_proxy.",
        ten_extension_get_name(extension));
    return;
  }

  TEN_ASSERT(extension->state >= TEN_EXTENSION_STATE_ON_DEINIT,
             "Should not happen.");

  if (extension->state == TEN_EXTENSION_STATE_ON_DEINIT_DONE) {
    return;
  }

  extension->state = TEN_EXTENSION_STATE_ON_DEINIT_DONE;

  TEN_LOGD("[%s] on_deinit() done.", ten_extension_get_name(extension));

  ten_extension_on_start_stop_deinit_done_t *on_deinit_done =
      ten_extension_on_start_stop_deinit_done_create(extension);

  ten_runloop_post_task_tail(ten_extension_get_attached_runloop(extension),
                             ten_extension_thread_on_extension_on_deinit_done,
                             extension->extension_thread, on_deinit_done);
}

ten_extension_on_start_stop_deinit_done_t *
ten_extension_on_start_stop_deinit_done_create(ten_extension_t *extension) {
  TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
             "Should not happen.");

  ten_extension_on_start_stop_deinit_done_t *self =
      TEN_MALLOC(sizeof(ten_extension_on_start_stop_deinit_done_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  self->extension = extension;

  return self;
}

void ten_extension_on_start_stop_deinit_done_destroy(
    ten_extension_on_start_stop_deinit_done_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  TEN_FREE(self);
}
