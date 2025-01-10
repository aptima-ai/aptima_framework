//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension/on_xxx.h"

#include "include_internal/axis_runtime/common/loc.h"
#include "include_internal/axis_runtime/extension/base_dir.h"
#include "include_internal/axis_runtime/extension/close.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension/metadata.h"
#include "include_internal/axis_runtime/extension/msg_handling.h"
#include "include_internal/axis_runtime/extension/path_timer.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/extension_store/extension_store.h"
#include "include_internal/axis_runtime/extension_thread/extension_thread.h"
#include "include_internal/axis_runtime/extension_thread/msg_interface/common.h"
#include "include_internal/axis_runtime/extension_thread/on_xxx.h"
#include "include_internal/axis_runtime/metadata/metadata_info.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "include_internal/axis_runtime/timer/timer.h"
#include "axis_utils/macro/check.h"

static bool axis_extension_parse_interface_schema(axis_extension_t *self,
                                                 axis_value_t *api_definition,
                                                 axis_error_t *err) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Invalid argument.");
  axis_ASSERT(api_definition && axis_value_check_integrity(api_definition),
             "Invalid argument.");

  bool result = axis_schema_store_set_interface_schema_definition(
      &self->schema_store, api_definition, axis_extension_get_base_dir(self),
      err);
  if (!result) {
    axis_LOGW("[%s] Failed to set interface schema definition: %s.",
             axis_extension_get_name(self, true), axis_error_errmsg(err));
  }

  return result;
}

static void axis_extension_adjust_and_validate_property_on_configure_done(
    axis_extension_t *self) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  bool success = axis_schema_store_adjust_properties(&self->schema_store,
                                                    &self->property, &err);
  if (!success) {
    axis_LOGW("[%s] Failed to adjust property type: %s.",
             axis_extension_get_name(self, true), axis_error_errmsg(&err));
    goto done;
  }

  success = axis_schema_store_validate_properties(&self->schema_store,
                                                 &self->property, &err);
  if (!success) {
    axis_LOGW("[%s] Invalid property: %s.", axis_extension_get_name(self, true),
             axis_error_errmsg(&err));
    goto done;
  }

done:
  axis_error_deinit(&err);
  if (!success) {
    axis_ASSERT(0, "Invalid property.");
  }
}

bool axis_extension_on_configure_done(axis_env_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(self, true), "Invalid use of axis_env %p.",
             self);

  axis_extension_t *extension = axis_env_get_attached_extension(self);
  axis_ASSERT(extension, "Invalid argument.");
  axis_ASSERT(axis_extension_check_integrity(extension, true),
             "Invalid use of extension %p.", extension);

  axis_LOGD("[%s] on_configure() done.",
           axis_extension_get_name(extension, true));

  if (extension->state != axis_EXTENSION_STATE_INIT) {
    axis_LOGI("[%s] Failed to on_configure_done() because of incorrect timing.",
             axis_extension_get_name(extension, true));
    return false;
  }

  extension->state = axis_EXTENSION_STATE_ON_CONFIGURE_DONE;

  axis_extension_thread_t *extension_thread = extension->extension_thread;
  axis_ASSERT(extension_thread &&
                 axis_extension_thread_check_integrity(extension_thread, true),
             "Should not happen.");

  if (extension_thread->is_close_triggered) {
    // Do not proceed with the subsequent init/start flow, as the extension
    // thread is about to shut down.
    return true;
  }

  axis_error_t err;
  axis_error_init(&err);

  bool rc = axis_handle_manifest_info_when_on_configure_done(
      &extension->manifest_info, axis_extension_get_base_dir(extension),
      &extension->manifest, &err);
  if (!rc) {
    axis_LOGW("Failed to load extension manifest data, FATAL ERROR.");
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    exit(EXIT_FAILURE);
  }

  rc = axis_handle_property_info_when_on_configure_done(
      &extension->property_info, axis_extension_get_base_dir(extension),
      &extension->property, &err);
  if (!rc) {
    axis_LOGW("Failed to load extension property data, FATAL ERROR.");
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    exit(EXIT_FAILURE);
  }

  rc = axis_extension_resolve_properties_in_graph(extension, &err);
  axis_ASSERT(rc, "Failed to resolve properties in graph.");

  axis_extension_merge_properties_from_graph(extension);

  rc = axis_extension_handle_axis_namespace_properties(
      extension, extension->extension_context);
  axis_ASSERT(rc, "[%s] Failed to handle '_ten' properties.",
             axis_string_get_raw_str(&extension->name));

  axis_value_t *api_definition = axis_metadata_init_schema_store(
      &extension->manifest, &extension->schema_store);
  if (api_definition) {
    bool success =
        axis_extension_parse_interface_schema(extension, api_definition, &err);
    axis_ASSERT(success, "Failed to parse interface schema.");
  }

  axis_extension_adjust_and_validate_property_on_configure_done(extension);

  // Create timers for automatically cleaning expired IN_PATHs and OUT_PATHs.
  axis_timer_t *in_path_timer =
      axis_extension_create_timer_for_in_path(extension);
  axis_list_push_ptr_back(&extension->path_timers, in_path_timer, NULL);
  axis_timer_enable(in_path_timer);

  axis_timer_t *out_path_timer =
      axis_extension_create_timer_for_out_path(extension);
  axis_list_push_ptr_back(&extension->path_timers, out_path_timer, NULL);
  axis_timer_enable(out_path_timer);

  // The interface info has been resolved, and extensions might send msg out
  // during `on_start()`, so it's the best time to merge the interface info to
  // the extension_info.
  rc =
      axis_extension_determine_and_merge_all_interface_dest_extension(extension);
  axis_ASSERT(rc, "Should not happen.");

  // Trigger the extension on_init flow.
  axis_extension_on_init(extension->axis_env);

  axis_error_deinit(&err);

  return true;
}

bool axis_extension_on_init_done(axis_env_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(self, true), "Invalid use of axis_env %p.",
             self);

  axis_extension_t *extension = axis_env_get_attached_extension(self);
  axis_ASSERT(extension, "Invalid argument.");
  axis_ASSERT(axis_extension_check_integrity(extension, true),
             "Invalid use of extension %p.", extension);

  axis_LOGD("[%s] on_init() done.", axis_extension_get_name(extension, true));

  if (extension->state != axis_EXTENSION_STATE_ON_CONFIGURE_DONE) {
    // `on_init_done` can only be called at specific times.
    axis_LOGI("[%s] Failed to on_init_done() because of incorrect timing.",
             axis_extension_get_name(extension, true));
    return false;
  }

  extension->state = axis_EXTENSION_STATE_ON_INIT_DONE;

  axis_extension_thread_t *extension_thread = extension->extension_thread;
  axis_ASSERT(extension_thread &&
                 axis_extension_thread_check_integrity(extension_thread, true),
             "Should not happen.");

  if (extension_thread->is_close_triggered) {
    return true;
  }

  // Trigger on_start of extension.
  axis_extension_on_start(extension);

  return true;
}

static void axis_extension_flush_all_pending_msgs(axis_extension_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_check_integrity(self, true),
             "Invalid use of extension %p.", self);

  // Flush the previously got messages, which are received before
  // on_init_done(), into the extension.
  axis_extension_thread_t *extension_thread = self->extension_thread;
  axis_list_foreach (&extension_thread->pending_msgs, iter) {
    axis_shared_ptr_t *msg = axis_smart_ptr_listnode_get(iter.node);
    axis_ASSERT(msg, "Should not happen.");

    axis_loc_t *dest_loc = axis_msg_get_first_dest_loc(msg);
    axis_ASSERT(dest_loc, "Should not happen.");

    if (axis_string_is_equal(&dest_loc->extension_name, &self->name)) {
      axis_extension_handle_in_msg(self, msg);
      axis_list_remove_node(&extension_thread->pending_msgs, iter.node);
    }
  }

  // Flush the previously got messages, which are received before
  // on_init_done(), into the extension.
  axis_list_foreach (&self->pending_msgs, iter) {
    axis_shared_ptr_t *msg = axis_smart_ptr_listnode_get(iter.node);
    axis_ASSERT(msg, "Should not happen.");

    axis_extension_handle_in_msg(self, msg);
  }
  axis_list_clear(&self->pending_msgs);
}

bool axis_extension_on_start_done(axis_env_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(self, true), "Invalid use of axis_env %p.",
             self);

  axis_extension_t *extension = axis_env_get_attached_extension(self);
  axis_ASSERT(extension, "Invalid argument.");
  axis_ASSERT(axis_extension_check_integrity(extension, true),
             "Invalid use of extension %p.", extension);

  axis_LOGI("[%s] on_start() done.", axis_extension_get_name(extension, true));

  if (extension->state != axis_EXTENSION_STATE_ON_START) {
    axis_LOGI("[%s] Failed to on_start_done() because of incorrect timing.",
             axis_extension_get_name(extension, true));
    return false;
  }

  extension->state = axis_EXTENSION_STATE_ON_START_DONE;

  axis_extension_flush_all_pending_msgs(extension);

  return true;
}

bool axis_extension_on_stop_done(axis_env_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(self, true), "Invalid use of axis_env %p.",
             self);

  axis_extension_t *extension = axis_env_get_attached_extension(self);
  axis_ASSERT(extension, "Invalid argument.");
  axis_ASSERT(axis_extension_check_integrity(extension, true),
             "Invalid use of extension %p.", extension);

  axis_LOGI("[%s] on_stop() done.", axis_extension_get_name(extension, true));

  if (extension->state != axis_EXTENSION_STATE_ON_START_DONE) {
    axis_LOGI("[%s] Failed to on_stop_done() because of incorrect timing.",
             axis_extension_get_name(extension, true));
    return false;
  }

  extension->state = axis_EXTENSION_STATE_ON_STOP_DONE;

  axis_extension_do_pre_close_action(extension);

  return true;
}

static void axis_extension_thread_del_extension(axis_extension_thread_t *self,
                                               axis_extension_t *extension) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);
  axis_ASSERT(extension, "Invalid argument.");

  axis_extension_inherit_thread_ownership(extension, self);
  axis_ASSERT(axis_extension_check_integrity(extension, true),
             "Invalid use of extension %p.", extension);

  axis_LOGD("[%s] Deleted from extension thread (%s).",
           axis_extension_get_name(extension, true),
           axis_string_get_raw_str(&self->extension_group->name));

  // Delete the extension from the extension store of the extension thread, so
  // that no more messages could be routed to this extension in the future.
  axis_extension_store_del_extension(self->extension_store, extension);

  self->extensions_cnt_of_deleted++;
  if (self->extensions_cnt_of_deleted == axis_list_size(&self->extensions)) {
    axis_extension_group_destroy_extensions(self->extension_group,
                                           self->extensions);
  }
}

static void axis_extension_thread_on_extension_on_deinit_done(
    axis_extension_thread_t *self, axis_extension_t *deinit_extension) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);
  axis_ASSERT(
      deinit_extension && axis_extension_check_integrity(deinit_extension, true),
      "Should not happen.");
  axis_ASSERT(deinit_extension->extension_thread == self, "Should not happen.");

  // Notify the 'aptima' object of this extension that we are closing.
  axis_ASSERT(deinit_extension->axis_env &&
                 axis_env_check_integrity(deinit_extension->axis_env, true),
             "Should not happen.");
  axis_env_close(deinit_extension->axis_env);

  axis_extension_thread_del_extension(self, deinit_extension);
}

bool axis_extension_on_deinit_done(axis_env_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(self, true), "Invalid use of axis_env %p.",
             self);

  axis_extension_t *extension = axis_env_get_attached_extension(self);
  axis_ASSERT(extension, "Invalid argument.");
  axis_ASSERT(axis_extension_check_integrity(extension, true),
             "Invalid use of extension %p.", extension);

  if (extension->state != axis_EXTENSION_STATE_ON_DEINIT) {
    axis_LOGI("[%s] Failed to on_deinit_done() because of incorrect timing.",
             axis_extension_get_name(extension, true));
    return false;
  }

  if (!axis_list_is_empty(&self->axis_proxy_list)) {
    // There is still the presence of axis_env_proxy, so the closing process
    // cannot continue.
    axis_LOGI(
        "[%s] Failed to on_deinit_done() because of existed axis_env_proxy.",
        axis_extension_get_name(extension, true));
    return true;
  }

  axis_ASSERT(extension->state >= axis_EXTENSION_STATE_ON_DEINIT,
             "Should not happen.");

  if (extension->state == axis_EXTENSION_STATE_ON_DEINIT_DONE) {
    return false;
  }

  extension->state = axis_EXTENSION_STATE_ON_DEINIT_DONE;

  axis_LOGD("[%s] on_deinit() done.", axis_extension_get_name(extension, true));

  axis_extension_thread_on_extension_on_deinit_done(extension->extension_thread,
                                                   extension);

  return true;
}
