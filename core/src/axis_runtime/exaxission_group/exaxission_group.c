//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension_group/extension_group.h"

#include <stdlib.h>
#include <time.h>

#include "include_internal/axis_runtime/addon/addon_host.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension_group/on_xxx.h"
#include "include_internal/axis_runtime/extension_thread/extension_thread.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "include_internal/axis_utils/log/log.h"
#include "include_internal/axis_utils/value/value.h"
#include "axis_runtime/binding/common.h"
#include "axis_runtime/msg/cmd_result/cmd_result.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/container/list_node.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/ref.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/sanitizer/thread_check.h"
#include "axis_utils/value/value.h"

bool axis_extension_group_check_integrity(axis_extension_group_t *self,
                                         bool check_thread) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_EXTENSION_GROUP_SIGNATURE) {
    return false;
  }
  if (self->binding_handle.me_in_target_lang == NULL) {
    return false;
  }

  if (check_thread) {
    // Note that the 'extension_thread' might be NULL when the extension group
    // is newly created.
    axis_extension_thread_t *extension_thread = self->extension_thread;
    if (extension_thread) {
      return axis_extension_thread_check_integrity(extension_thread, true);
    }

    return axis_sanitizer_thread_check_do_check(&self->thread_check);
  }

  return true;
}

axis_extension_group_t *axis_extension_group_create_internal(
    const char *name, axis_extension_group_on_configure_func_t on_configure,
    axis_extension_group_on_init_func_t on_init,
    axis_extension_group_on_deinit_func_t on_deinit,
    axis_extension_group_on_create_extensions_func_t on_create_extensions,
    axis_extension_group_on_destroy_extensions_func_t on_destroy_extensions) {
  axis_extension_group_t *self =
      (axis_extension_group_t *)axis_MALLOC(sizeof(axis_extension_group_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_signature_set(&self->signature,
                    (axis_signature_t)axis_EXTENSION_GROUP_SIGNATURE);
  axis_sanitizer_thread_check_init_with_current_thread(&self->thread_check);

  self->addon_host = NULL;
  if (name) {
    axis_string_init_formatted(&self->name, "%s", name);
  }

  self->on_configure = on_configure;
  self->on_init = on_init;
  self->on_deinit = on_deinit;
  self->on_create_extensions = on_create_extensions;
  self->on_destroy_extensions = on_destroy_extensions;

  // This variable might be replaced later by the target language world.
  self->binding_handle.me_in_target_lang = self;

  self->extension_group_info = NULL;
  self->extension_thread = NULL;
  self->axis_env = NULL;

  axis_list_init(&self->extension_addon_and_instance_name_pairs);
  axis_error_init(&self->err_before_ready);

  axis_value_init_object_with_move(&self->manifest, NULL);
  axis_value_init_object_with_move(&self->property, NULL);

  self->manifest_info = NULL;
  self->property_info = NULL;

  self->app = NULL;
  self->extension_context = NULL;
  self->state = axis_EXTENSION_GROUP_STATE_INIT;
  self->extensions_cnt_of_being_destroyed = 0;

  return self;
}

axis_extension_group_t *axis_extension_group_create(
    const char *name, axis_extension_group_on_configure_func_t on_configure,
    axis_extension_group_on_init_func_t on_init,
    axis_extension_group_on_deinit_func_t on_deinit,
    axis_extension_group_on_create_extensions_func_t on_create_extensions,
    axis_extension_group_on_destroy_extensions_func_t on_destroy_extensions) {
  axis_ASSERT(name && on_create_extensions && on_destroy_extensions,
             "Should not happen.");

  axis_extension_group_t *self = axis_extension_group_create_internal(
      name, on_configure, on_init, on_deinit, on_create_extensions,
      on_destroy_extensions);
  self->axis_env = axis_env_create_for_extension_group(self);

  return self;
}

void axis_extension_group_destroy(axis_extension_group_t *self) {
  // TODO(Wei): It's strange that JS main thread would call this function, need
  // further investigation.
  axis_ASSERT(self && axis_extension_group_check_integrity(self, false),
             "Should not happen.");
  axis_ASSERT(self->extension_thread == NULL, "Should not happen.");
  axis_ASSERT(self->extensions_cnt_of_being_destroyed == 0,
             "Should not happen.");

  axis_signature_set(&self->signature, 0);

  if (self->axis_env) {
    axis_env_destroy(self->axis_env);
  }

  axis_error_deinit(&self->err_before_ready);
  axis_list_clear(&self->extension_addon_and_instance_name_pairs);

  axis_value_deinit(&self->manifest);
  axis_value_deinit(&self->property);

  if (self->manifest_info) {
    axis_metadata_info_destroy(self->manifest_info);
    self->manifest_info = NULL;
  }

  if (self->property_info) {
    axis_metadata_info_destroy(self->property_info);
    self->property_info = NULL;
  }

  axis_string_deinit(&self->name);

  if (self->addon_host) {
    // Since the extension has already been destroyed, there is no need to
    // release its resources through the corresponding addon anymore. Therefore,
    // decrement the reference count of the corresponding addon.
    axis_ref_dec_ref(&self->addon_host->ref);
    self->addon_host = NULL;
  }

  axis_sanitizer_thread_check_deinit(&self->thread_check);

  axis_FREE(self);
}

void axis_extension_group_create_extensions(axis_extension_group_t *self) {
  axis_ASSERT(self && axis_extension_group_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(self->on_create_extensions, "Should not happen.");
  axis_ASSERT(self->axis_env && axis_env_check_integrity(self->axis_env, true),
             "Should not happen.");

  axis_LOGD("[%s] create_extensions.", axis_extension_group_get_name(self, true));

  axis_extension_thread_t *extension_thread = self->extension_thread;
  axis_ASSERT(extension_thread, "Should not happen.");
  axis_ASSERT(axis_extension_thread_check_integrity(extension_thread, true),
             "Should not happen.");

  axis_extension_thread_set_state(
      extension_thread, axis_EXTENSION_THREAD_STATE_CREATING_EXTENSIONS);

  self->on_create_extensions(self, self->axis_env);
}

void axis_extension_group_destroy_extensions(axis_extension_group_t *self,
                                            axis_list_t extensions) {
  axis_ASSERT(self && axis_extension_group_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(self->on_destroy_extensions, "Should not happen.");
  axis_ASSERT(self->axis_env && axis_env_check_integrity(self->axis_env, true),
             "Should not happen.");

  axis_LOGD("[%s] destroy_extensions.",
           axis_extension_group_get_name(self, true));

  self->on_destroy_extensions(self, self->axis_env, extensions);
}

void axis_extension_group_set_addon(axis_extension_group_t *self,
                                   axis_addon_host_t *addon_host) {
  axis_ASSERT(self, "Should not happen.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: in the case of JS binding, the extension group would
  // initially created in the JS main thread, and and engine thread will
  // call this function. However, these operations are all occurred
  // before the whole extension system is running, so it's thread safe.
  axis_ASSERT(axis_extension_group_check_integrity(self, false),
             "Should not happen.");

  axis_ASSERT(addon_host, "Should not happen.");
  axis_ASSERT(axis_addon_host_check_integrity(addon_host), "Should not happen.");

  // Since the extension requires the corresponding addon to release
  // its resources, therefore, hold on to a reference count of the corresponding
  // addon.
  axis_ASSERT(!self->addon_host, "Should not happen.");
  self->addon_host = addon_host;
  axis_ref_inc_ref(&addon_host->ref);
}

axis_shared_ptr_t *axis_extension_group_create_invalid_dest_status(
    axis_shared_ptr_t *origin_cmd, axis_string_t *target_group_name) {
  axis_ASSERT(origin_cmd && axis_msg_is_cmd_and_result(origin_cmd),
             "Should not happen.");
  axis_ASSERT(target_group_name, "Should not happen.");

  axis_shared_ptr_t *status =
      axis_cmd_result_create_from_cmd(axis_STATUS_CODE_ERROR, origin_cmd);
  axis_msg_set_property(
      status, "detail",
      axis_value_create_vstring("The extension group[%s] is invalid.",
                               axis_string_get_raw_str(target_group_name)),
      NULL);

  return status;
}

axis_runloop_t *axis_extension_group_get_attached_runloop(
    axis_extension_group_t *self) {
  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in
                 // other threads.
                 axis_extension_group_check_integrity(self, false),
             "Should not happen.");

  return self->extension_thread->runloop;
}

axis_list_t *axis_extension_group_get_extension_addon_and_instance_name_pairs(
    axis_extension_group_t *self) {
  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in
                 // other threads.
                 axis_extension_group_check_integrity(self, false),
             "Should not happen.");

  return &self->extension_addon_and_instance_name_pairs;
}

axis_env_t *axis_extension_group_get_axis_env(axis_extension_group_t *self) {
  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in
                 // other threads.
                 axis_extension_group_check_integrity(self, false),
             "Should not happen.");

  return self->axis_env;
}

void axis_extension_group_set_extension_cnt_of_being_destroyed(
    axis_extension_group_t *self, size_t new_cnt) {
  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in
                 // other threads.
                 axis_extension_group_check_integrity(self, false),
             "Should not happen.");

  self->extensions_cnt_of_being_destroyed = new_cnt;
}

size_t axis_extension_group_decrement_extension_cnt_of_being_destroyed(
    axis_extension_group_t *self) {
  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in
                 // other threads.
                 axis_extension_group_check_integrity(self, false),
             "Should not happen.");

  return --self->extensions_cnt_of_being_destroyed;
}
