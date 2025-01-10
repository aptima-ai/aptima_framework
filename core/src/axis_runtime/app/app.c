//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/app/app.h"

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/app/close.h"
#include "include_internal/axis_runtime/app/engine_interface.h"
#include "include_internal/axis_runtime/app/migration.h"
#include "include_internal/axis_runtime/connection/connection.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/global/global.h"
#include "include_internal/axis_runtime/global/signal.h"
#include "include_internal/axis_runtime/protocol/protocol.h"
#include "include_internal/axis_runtime/schema_store/store.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/binding/common.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_str.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/event.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/lib/ref.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/sanitizer/thread_check.h"
#include "axis_utils/value/value.h"

static void axis_app_inherit_thread_ownership(axis_app_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: The correct threading ownership will be setup soon, so we can
  // _not_ check thread safety here.
  axis_ASSERT(axis_app_check_integrity(self, false), "Invalid use of app %p.",
             self);

  // Move the ownership of the app relevant resources to the belonging app
  // thread.
  axis_sanitizer_thread_check_set_belonging_thread_to_current_thread(
      &self->thread_check);
  axis_sanitizer_thread_check_inherit_from(&self->axis_env->thread_check,
                                          &self->thread_check);
}

static void *axis_app_routine(void *args) {
  axis_app_t *self = (axis_app_t *)args;
  axis_ASSERT(self, "Invalid argument.");
  if (!self) {
    axis_LOGF("Invalid app pointer.");
    return NULL;
  }

  // This is the app thread, so transfer the thread ownership of the app to this
  // thread no matter where the app is created.
  axis_app_inherit_thread_ownership(self);

  if (self->run_in_background) {
    axis_event_set(self->belonging_thread_is_set);
  }

  axis_ASSERT(axis_app_check_integrity(self, true), "Should not happen.");

  axis_LOGI("[%s] App is created.", axis_app_get_uri(self));

  self->loop = axis_runloop_create(NULL);
  axis_ASSERT(self->loop, "Should not happen.");
  if (!self->loop) {
    axis_LOGF("Failed to create runloop.");
    return NULL;
  }

  // The runloop has been created, and now we can push runloop tasks. Therefore,
  // we check if the app has already been triggered to close here.
  if (axis_app_is_closing(self)) {
    axis_app_close(self, NULL);
  }

  axis_global_add_app(self);

  axis_app_start(self);

  return NULL;
}

axis_app_t *axis_app_create(axis_app_on_configure_func_t on_configure,
                          axis_app_on_init_func_t on_init,
                          axis_app_on_deinit_func_t on_deinit,
                          axis_UNUSED axis_error_t *err) {
  axis_app_t *self = (axis_app_t *)axis_MALLOC(sizeof(axis_app_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  self->on_configure = on_configure;
  self->on_init = on_init;
  self->on_deinit = on_deinit;

  self->binding_handle.me_in_target_lang = self;

  axis_signature_set(&self->signature, (axis_signature_t)axis_APP_SIGNATURE);

  axis_sanitizer_thread_check_init_with_current_thread(&self->thread_check);
  self->belonging_thread_is_set = axis_event_create(0, 0);

  self->state_lock = axis_mutex_create();
  self->state = axis_APP_STATE_INIT;

  self->endpoint_protocol = NULL;

  axis_list_init(&self->engines);
  axis_list_init(&self->orphan_connections);

  axis_list_init(&self->predefined_graph_infos);

  axis_value_init_object_with_move(&self->manifest, NULL);
  axis_value_init_object_with_move(&self->property, NULL);
  axis_schema_store_init(&self->schema_store);

  axis_string_init(&self->uri);

  self->in_msgs_lock = axis_mutex_create();
  axis_list_init(&self->in_msgs);

  self->axis_env = axis_env_create_for_app(self);
  axis_ASSERT(self->axis_env, "Should not happen.");

  axis_string_init(&self->base_dir);
  axis_list_init(&self->axis_package_base_dirs);

  self->manifest_info = NULL;
  self->property_info = NULL;

  self->user_data = NULL;

  return self;
}

void axis_app_destroy(axis_app_t *self) {
  axis_ASSERT(self && axis_app_check_integrity(self, false),
             "Should not happen.");

  axis_LOGD("[%s] Destroy a App", axis_app_get_uri(self));

  axis_global_del_app(self);

  axis_signature_set(&self->signature, 0);

  axis_env_destroy(self->axis_env);
  axis_mutex_destroy(self->state_lock);

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

  axis_schema_store_deinit(&self->schema_store);

  axis_mutex_destroy(self->in_msgs_lock);
  axis_list_clear(&self->in_msgs);

  axis_runloop_destroy(self->loop);
  self->loop = NULL;

  axis_ASSERT(axis_list_is_empty(&self->engines), "Should not happen.");
  axis_ASSERT(axis_list_is_empty(&self->orphan_connections),
             "Should not happen.");

  axis_list_clear(&self->predefined_graph_infos);
  axis_string_deinit(&self->uri);

  axis_sanitizer_thread_check_deinit(&self->thread_check);
  axis_event_destroy(self->belonging_thread_is_set);

  axis_string_deinit(&self->base_dir);
  axis_list_clear(&self->axis_package_base_dirs);

  axis_FREE(self);
}

void axis_app_add_axis_package_base_dir(axis_app_t *self, const char *base_dir) {
  axis_ASSERT(self && axis_app_check_integrity(self, false), "Invalid argument.");

  axis_list_push_str_back(&self->axis_package_base_dirs, base_dir);
}

bool axis_app_run(axis_app_t *self, bool run_in_background,
                 axis_UNUSED axis_error_t *error_ctx) {
  axis_ASSERT(self, "Invalid argument.");

  self->run_in_background = run_in_background;

  // APTIMA app might be closed before running.
  if (axis_app_is_closing(self)) {
    axis_LOGW("Failed to run APTIMA app, because it has been closed.");
    return false;
  }

  if (run_in_background) {
    axis_thread_create(axis_string_get_raw_str(&self->uri), axis_app_routine,
                      self);
    axis_event_wait(self->belonging_thread_is_set, -1);
  } else {
    axis_app_routine(self);
  }

  return true;
}

bool axis_app_wait(axis_app_t *self, axis_UNUSED axis_error_t *err) {
  axis_ASSERT(self && axis_app_check_integrity(self, false),
             "Should not happen.");

  axis_LOGD("Wait app thread ends.");

  if (self->run_in_background &&
      axis_sanitizer_thread_check_get_belonging_thread(&self->thread_check)) {
    int rc = axis_thread_join(
        axis_sanitizer_thread_check_get_belonging_thread(&self->thread_check),
        -1);

    return rc == 0 ? true : false;
  }

  return true;
}

bool axis_app_thread_call_by_me(axis_app_t *self) {
  axis_ASSERT(self && axis_app_check_integrity(self, false),
             "Should not happen.");

  return axis_thread_equal(NULL, axis_sanitizer_thread_check_get_belonging_thread(
                                    &self->thread_check));
}
