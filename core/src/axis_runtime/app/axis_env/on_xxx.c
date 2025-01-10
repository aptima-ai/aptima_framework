//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/addon/addon_autoload.h"
#include "include_internal/axis_runtime/addon/addon_loader/addon_loader.h"
#include "include_internal/axis_runtime/addon/addon_manager.h"
#include "include_internal/axis_runtime/addon/extension/extension.h"
#include "include_internal/axis_runtime/addon/extension_group/extension_group.h"
#include "include_internal/axis_runtime/addon/protocol/protocol.h"
#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/app/base_dir.h"
#include "include_internal/axis_runtime/app/close.h"
#include "include_internal/axis_runtime/app/endpoint.h"
#include "include_internal/axis_runtime/app/engine_interface.h"
#include "include_internal/axis_runtime/app/metadata.h"
#include "include_internal/axis_runtime/app/migration.h"
#include "include_internal/axis_runtime/app/predefined_graph.h"
#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/extension_group/builtin/builtin_extension_group.h"
#include "include_internal/axis_runtime/metadata/manifest.h"
#include "include_internal/axis_runtime/metadata/metadata.h"
#include "include_internal/axis_runtime/metadata/metadata_info.h"
#include "include_internal/axis_runtime/protocol/close.h"
#include "include_internal/axis_runtime/protocol/protocol.h"
#include "include_internal/axis_runtime/schema_store/store.h"
#include "include_internal/axis_runtime/axis_env/log.h"
#include "include_internal/axis_runtime/axis_env/metadata_cb.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "include_internal/axis_runtime/test/test_extension.h"
#include "axis_runtime/app/app.h"
#include "axis_runtime/axis_env/internal/on_xxx_done.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"

static void axis_app_adjust_and_validate_property_on_configure_done(
    axis_app_t *self) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  bool success = axis_schema_store_adjust_properties(&self->schema_store,
                                                    &self->property, &err);
  if (!success) {
    axis_LOGW("Failed to adjust property type, %s.", axis_error_errmsg(&err));
    goto done;
  }

  success = axis_schema_store_validate_properties(&self->schema_store,
                                                 &self->property, &err);
  if (!success) {
    axis_LOGW("Invalid property, %s.", axis_error_errmsg(&err));
    goto done;
  }

done:
  axis_error_deinit(&err);
  if (!success) {
    axis_ASSERT(0, "Invalid property.");
  }
}

static void axis_app_start_auto_start_predefined_graph_and_trigger_on_init(
    axis_app_t *self) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");
  axis_ASSERT(self->axis_env && axis_env_check_integrity(self->axis_env, true),
             "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  bool rc = axis_app_start_auto_start_predefined_graph(self, &err);
  axis_ASSERT(rc, "Should not happen, %s.", axis_error_errmsg(&err));

  axis_error_deinit(&err);

  // Trigger on_init.
  axis_app_on_init(self->axis_env);
}

static void axis_app_on_endpoint_protocol_created(axis_env_t *axis_env,
                                                 axis_protocol_t *protocol,
                                                 void *cb_data) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_app_t *self = axis_env_get_attached_app(axis_env);
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");

  if (!protocol) {
    axis_LOGE("Failed to create app endpoint protocol, FATAL ERROR.");
    axis_app_close(self, NULL);
    return;
  }

  axis_ASSERT(axis_protocol_check_integrity(protocol, true),
             "Should not happen, %p.", protocol);

  self->endpoint_protocol = protocol;

  axis_protocol_attach_to_app(self->endpoint_protocol, self);
  axis_protocol_set_on_closed(self->endpoint_protocol,
                             axis_app_on_protocol_closed, self);

  if (!axis_app_endpoint_listen(self)) {
    axis_app_close(self, NULL);
    return;
  }

  axis_app_start_auto_start_predefined_graph_and_trigger_on_init(self);
}

void axis_app_on_all_addon_loaders_created(axis_app_t *self) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  if (!axis_app_get_predefined_graphs_from_property(self)) {
    goto error;
  }

  if (!axis_string_is_equal_c_str(&self->uri, axis_STR_LOCALHOST) &&
      !axis_string_starts_with(&self->uri, axis_STR_CLIENT)) {
    // Create the app listening endpoint protocol if specifying one.
    bool rc = axis_addon_create_protocol_with_uri(
        self->axis_env, axis_string_get_raw_str(&self->uri),
        axis_PROTOCOL_ROLE_LISTEN, axis_app_on_endpoint_protocol_created, NULL,
        &err);
    if (!rc) {
      axis_LOGW("Failed to create app endpoint protocol, %s.",
               axis_error_errmsg(&err));
      goto error;
    }
  } else {
    axis_app_start_auto_start_predefined_graph_and_trigger_on_init(self);
  }

  goto done;

error:
  axis_app_close(self, NULL);
done:
  axis_error_deinit(&err);
}

void axis_app_on_configure_done(axis_env_t *axis_env) {
  axis_ASSERT(axis_env, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(axis_env, true),
             "Invalid use of axis_env %p.", axis_env);

  axis_app_t *self = axis_env_get_attached_app(axis_env);
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");
  axis_ASSERT(self->loop, "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  bool rc = axis_handle_manifest_info_when_on_configure_done(
      &self->manifest_info, axis_app_get_base_dir(self), &self->manifest, &err);
  if (!rc) {
    axis_LOGW("Failed to load app manifest data, FATAL ERROR.");
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    exit(EXIT_FAILURE);
  }

  rc = axis_handle_property_info_when_on_configure_done(
      &self->property_info, axis_app_get_base_dir(self), &self->property, &err);
  if (!rc) {
    axis_LOGW("Failed to load app property data, FATAL ERROR.");
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    exit(EXIT_FAILURE);
  }

  if (!axis_app_handle_axis_namespace_properties(self)) {
    axis_LOGW("Failed to determine app default property.");
  }

  axis_metadata_init_schema_store(&self->manifest, &self->schema_store);
  axis_app_adjust_and_validate_property_on_configure_done(self);

  if (axis_string_is_empty(&self->uri)) {
    axis_string_set_from_c_str(&self->uri, axis_STR_LOCALHOST,
                              strlen(axis_STR_LOCALHOST));
  }

  axis_addon_load_all_from_app_base_dir(axis_string_get_raw_str(&self->base_dir),
                                       &err);
  axis_addon_load_all_from_axis_package_base_dirs(&self->axis_package_base_dirs,
                                                &err);

  // @{
  // Register all addons.
  axis_builtin_extension_group_addon_register();
  axis_builtin_test_extension_addon_register();

  axis_addon_manager_t *manager = axis_addon_manager_get_instance();
  axis_addon_register_ctx_t *register_ctx = axis_addon_register_ctx_create();
  register_ctx->app = self;
  axis_addon_manager_register_all_addons(manager, (void *)register_ctx);
  axis_addon_register_ctx_destroy(register_ctx);
  // @}

  // Create addon loader singleton instances.
  bool need_to_wait_all_addon_loaders_created =
      axis_addon_loader_addons_create_singleton_instance(axis_env);
  if (!need_to_wait_all_addon_loaders_created) {
    axis_app_on_all_addon_loaders_created(self);
  }
}

void axis_app_on_configure(axis_env_t *axis_env) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");
  axis_ASSERT(axis_env_get_attach_to(axis_env) == axis_ENV_ATTACH_TO_APP,
             "Should not happen.");

  axis_app_t *self = axis_env_get_attached_app(axis_env);
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");

  self->manifest_info =
      axis_metadata_info_create(axis_METADATA_ATTACH_TO_MANIFEST, self->axis_env);
  self->property_info =
      axis_metadata_info_create(axis_METADATA_ATTACH_TO_PROPERTY, self->axis_env);

  if (self->on_configure) {
    self->on_configure(self, self->axis_env);
  } else {
    axis_app_on_configure_done(self->axis_env);
  }
}

void axis_app_on_init(axis_env_t *axis_env) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");
  axis_ASSERT(axis_env_get_attach_to(axis_env) == axis_ENV_ATTACH_TO_APP,
             "Should not happen.");

  axis_app_t *self = axis_env_get_attached_app(axis_env);
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");

  if (self->on_init) {
    self->on_init(self, self->axis_env);
  } else {
    axis_app_on_init_done(self->axis_env);
  }
}

static void axis_app_on_init_done_internal(axis_app_t *self) {
  axis_ASSERT(self && axis_app_check_integrity(self, true) && self->loop,
             "Should not happen.");
}

void axis_app_on_init_done(axis_env_t *axis_env) {
  axis_ASSERT(axis_env, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(axis_env, true),
             "Invalid use of axis_env %p.", axis_env);

  axis_app_t *self = axis_env_get_attached_app(axis_env);
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");

  axis_app_on_init_done_internal(self);
}

static void axis_app_unregister_addons_after_app_close(axis_app_t *self) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");

  const char *disabled = getenv("axis_DISABLE_ADDON_UNREGISTER_AFTER_APP_CLOSE");
  if (disabled && !strcmp(disabled, "true")) {
    return;
  }

  axis_unregister_all_addons_and_cleanup();
}

void axis_app_on_deinit(axis_app_t *self) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");

  // The world outside of APTIMA would do some operations after the app_run()
  // returns, so it's best to perform the on_deinit callback _before_ the
  // runloop is stopped.

  // @{
  // **Note:** The two functions below will invoke functions like `on_deinit`,
  // which may call into different language environments, such as the
  // `on_deinit` function of a Python addon. Therefore, these two functions must
  // not be called within the call flow of the C API initiated by those
  // languages. In other words, these two functions cannot be invoked within the
  // call flow of functions like `on_deinit_done`. Instead, they must be called
  // within the call flow of a purely C-native thread; otherwise, it may
  // potentially lead to a deadlock.

  // The `on_deinit` of the protocol instance needs to call the `on_deinit_done`
  // of the addon host, so this logic must be performed before unregistering the
  // protocol addons.
  if (self->endpoint_protocol) {
    axis_ref_dec_ref(&self->endpoint_protocol->ref);
  }

  // At the final stage of addon deinitialization, `axis_env_t::on_deinit_done`
  // is required, which in turn depends on the runloop. Therefore, the addon
  // deinitialization process must be performed _before_ the app's runloop
  // ends.
  axis_app_unregister_addons_after_app_close(self);
  // @}

  if (self->on_deinit) {
    // Call the registered on_deinit callback if exists.
    self->on_deinit(self, self->axis_env);
  } else {
    axis_env_on_deinit_done(self->axis_env, NULL);
  }
}

void axis_app_on_deinit_done(axis_env_t *axis_env) {
  axis_ASSERT(axis_env, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(axis_env, true),
             "Invalid use of axis_env %p.", axis_env);

  axis_app_t *self = axis_env_get_attached_app(axis_env);
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");

  if (!axis_list_is_empty(&axis_env->axis_proxy_list)) {
    // There is still the presence of axis_env_proxy, so the closing process
    // cannot continue.
    axis_LOGI("App %s cannot on_deinit_done() because of existed axis_env_proxy.",
             axis_string_get_raw_str(&self->uri));
    return;
  }

  axis_mutex_lock(self->state_lock);
  axis_ASSERT(self->state >= axis_APP_STATE_CLOSING, "Should not happen.");
  if (self->state == axis_APP_STATE_CLOSED) {
    axis_mutex_unlock(self->state_lock);
    return;
  }
  self->state = axis_APP_STATE_CLOSED;
  axis_mutex_unlock(self->state_lock);

  axis_ENV_LOG_DEBUG_INTERNAL(axis_env, "app on_deinit_done().");

  axis_env_close(self->axis_env);
  axis_runloop_stop(self->loop);
}
