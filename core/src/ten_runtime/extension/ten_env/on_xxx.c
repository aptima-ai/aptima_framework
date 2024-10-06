//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/extension/on_xxx.h"

#include "include_internal/ten_runtime/common/loc.h"
#include "include_internal/ten_runtime/extension/base_dir.h"
#include "include_internal/ten_runtime/extension/close.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension/metadata.h"
#include "include_internal/ten_runtime/extension/msg_handling.h"
#include "include_internal/ten_runtime/extension/path_timer.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/extension_store/extension_store.h"
#include "include_internal/ten_runtime/extension_thread/extension_thread.h"
#include "include_internal/ten_runtime/extension_thread/msg_interface/common.h"
#include "include_internal/ten_runtime/extension_thread/on_xxx.h"
#include "include_internal/ten_runtime/metadata/metadata_info.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "include_internal/ten_runtime/timer/timer.h"
#include "ten_utils/macro/check.h"

static bool ten_extension_parse_interface_schema(ten_extension_t *self,
                                                 ten_value_t *api_definition,
                                                 ten_error_t *err) {
  TEN_ASSERT(self && ten_extension_check_integrity(self, true),
             "Invalid argument.");
  TEN_ASSERT(api_definition && ten_value_check_integrity(api_definition),
             "Invalid argument.");

  bool result = ten_schema_store_set_interface_schema_definition(
      &self->schema_store, api_definition, ten_extension_get_base_dir(self),
      err);
  if (!result) {
    TEN_LOGW("[%s] Failed to set interface schema definition: %s.",
             ten_extension_get_name(self, true), ten_error_errmsg(err));
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
             ten_extension_get_name(self, true), ten_error_errmsg(&err));
    goto done;
  }

  success = ten_schema_store_validate_properties(&self->schema_store,
                                                 &self->property, &err);
  if (!success) {
    TEN_LOGW("[%s] Invalid property: %s.", ten_extension_get_name(self, true),
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

  TEN_LOGD("[%s] on_configure() done.",
           ten_extension_get_name(extension, true));

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
      &extension->manifest_info, ten_extension_get_base_dir(extension),
      &extension->manifest, &err);
  if (!rc) {
    TEN_LOGW("Failed to load extension manifest data, FATAL ERROR.");
    exit(EXIT_FAILURE);
  }

  rc = ten_handle_property_info_when_on_configure_done(
      &extension->property_info, ten_extension_get_base_dir(extension),
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

  TEN_LOGD("[%s] on_init() done.", ten_extension_get_name(extension, true));

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

static void ten_extension_flush_all_pending_msgs(ten_extension_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  // Flush the previously got messages, which are received before
  // on_init_done(), into the extension.
  ten_extension_thread_t *extension_thread = self->extension_thread;
  ten_list_foreach (&extension_thread->pending_msgs, iter) {
    ten_shared_ptr_t *msg = ten_smart_ptr_listnode_get(iter.node);
    TEN_ASSERT(msg, "Should not happen.");

    ten_loc_t *dest_loc = ten_msg_get_first_dest_loc(msg);
    TEN_ASSERT(dest_loc, "Should not happen.");

    if (ten_string_is_equal(&dest_loc->extension_name, &self->name)) {
      ten_extension_handle_in_msg(self, msg);
      ten_list_remove_node(&extension_thread->pending_msgs, iter.node);
    }
  }

  // Flush the previously got messages, which are received before
  // on_init_done(), into the extension.
  ten_list_foreach (&self->pending_msgs, iter) {
    ten_shared_ptr_t *msg = ten_smart_ptr_listnode_get(iter.node);
    TEN_ASSERT(msg, "Should not happen.");

    ten_extension_handle_in_msg(self, msg);
  }
  ten_list_clear(&self->pending_msgs);
}

void ten_extension_on_start_done(ten_env_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(self, true), "Invalid use of ten_env %p.",
             self);

  ten_extension_t *extension = ten_env_get_attached_extension(self);
  TEN_ASSERT(extension, "Invalid argument.");
  TEN_ASSERT(ten_extension_check_integrity(extension, true),
             "Invalid use of extension %p.", extension);

  TEN_LOGI("[%s] on_start() done.", ten_extension_get_name(extension, true));

  extension->state = TEN_EXTENSION_STATE_ON_START_DONE;

  ten_extension_flush_all_pending_msgs(extension);
}

void ten_extension_on_stop_done(ten_env_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(self, true), "Invalid use of ten_env %p.",
             self);

  ten_extension_t *extension = ten_env_get_attached_extension(self);
  TEN_ASSERT(extension, "Invalid argument.");
  TEN_ASSERT(ten_extension_check_integrity(extension, true),
             "Invalid use of extension %p.", extension);

  TEN_LOGI("[%s] on_stop() done.", ten_extension_get_name(extension, true));

  extension->state = TEN_EXTENSION_STATE_ON_STOP_DONE;

  ten_extension_do_pre_close_action(extension);
}

static void ten_extension_thread_del_extension(ten_extension_thread_t *self,
                                               ten_extension_t *extension) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);
  TEN_ASSERT(extension, "Invalid argument.");

  ten_extension_inherit_thread_ownership(extension, self);
  TEN_ASSERT(ten_extension_check_integrity(extension, true),
             "Invalid use of extension %p.", extension);

  TEN_LOGD("[%s] Deleted from extension thread (%s).",
           ten_extension_get_name(extension, true),
           ten_string_get_raw_str(&self->extension_group->name));

  // Delete the extension from the extension store of the extension thread, so
  // that no more messages could be routed to this extension in the future.
  ten_extension_store_del_extension(self->extension_store, extension);

  self->extensions_cnt_of_deleted++;
  if (self->extensions_cnt_of_deleted == ten_list_size(&self->extensions)) {
    ten_extension_group_destroy_extensions(self->extension_group,
                                           self->extensions);
  }
}

static void ten_extension_thread_on_extension_on_deinit_done(
    ten_extension_thread_t *self, ten_extension_t *deinit_extension) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);
  TEN_ASSERT(
      deinit_extension && ten_extension_check_integrity(deinit_extension, true),
      "Should not happen.");
  TEN_ASSERT(deinit_extension->extension_thread == self, "Should not happen.");

  // Notify the 'ten' object of this extension that we are closing.
  TEN_ASSERT(deinit_extension->ten_env &&
                 ten_env_check_integrity(deinit_extension->ten_env, true),
             "Should not happen.");
  ten_env_close(deinit_extension->ten_env);

  ten_extension_thread_del_extension(self, deinit_extension);
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
        ten_extension_get_name(extension, true));
    return;
  }

  TEN_ASSERT(extension->state >= TEN_EXTENSION_STATE_ON_DEINIT,
             "Should not happen.");

  if (extension->state == TEN_EXTENSION_STATE_ON_DEINIT_DONE) {
    return;
  }

  extension->state = TEN_EXTENSION_STATE_ON_DEINIT_DONE;

  TEN_LOGD("[%s] on_deinit() done.", ten_extension_get_name(extension, true));

  ten_extension_thread_on_extension_on_deinit_done(extension->extension_thread,
                                                   extension);
}
