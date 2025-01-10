//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/addon/addon_autoload.h"
#include "include_internal/ten_runtime/addon/addon_loader/addon_loader.h"
#include "include_internal/ten_runtime/addon/addon_manager.h"
#include "include_internal/ten_runtime/addon/extension/extension.h"
#include "include_internal/ten_runtime/addon/extension_group/extension_group.h"
#include "include_internal/ten_runtime/addon/protocol/protocol.h"
#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/app/base_dir.h"
#include "include_internal/ten_runtime/app/close.h"
#include "include_internal/ten_runtime/app/endpoint.h"
#include "include_internal/ten_runtime/app/engine_interface.h"
#include "include_internal/ten_runtime/app/metadata.h"
#include "include_internal/ten_runtime/app/migration.h"
#include "include_internal/ten_runtime/app/predefined_graph.h"
#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/extension_group/builtin/builtin_extension_group.h"
#include "include_internal/ten_runtime/metadata/manifest.h"
#include "include_internal/ten_runtime/metadata/metadata.h"
#include "include_internal/ten_runtime/metadata/metadata_info.h"
#include "include_internal/ten_runtime/protocol/close.h"
#include "include_internal/ten_runtime/protocol/protocol.h"
#include "include_internal/ten_runtime/schema_store/store.h"
#include "include_internal/ten_runtime/ten_env/log.h"
#include "include_internal/ten_runtime/ten_env/metadata_cb.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "include_internal/ten_runtime/test/test_extension.h"
#include "ten_runtime/app/app.h"
#include "ten_runtime/ten_env/internal/on_xxx_done.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"

static void ten_app_adjust_and_validate_property_on_configure_done(
    ten_app_t *self) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  bool success = ten_schema_store_adjust_properties(&self->schema_store,
                                                    &self->property, &err);
  if (!success) {
    TEN_LOGW("Failed to adjust property type, %s.", ten_error_errmsg(&err));
    goto done;
  }

  success = ten_schema_store_validate_properties(&self->schema_store,
                                                 &self->property, &err);
  if (!success) {
    TEN_LOGW("Invalid property, %s.", ten_error_errmsg(&err));
    goto done;
  }

done:
  ten_error_deinit(&err);
  if (!success) {
    TEN_ASSERT(0, "Invalid property.");
  }
}

static void ten_app_start_auto_start_predefined_graph_and_trigger_on_init(
    ten_app_t *self) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");
  TEN_ASSERT(self->ten_env && ten_env_check_integrity(self->ten_env, true),
             "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  bool rc = ten_app_start_auto_start_predefined_graph(self, &err);
  TEN_ASSERT(rc, "Should not happen, %s.", ten_error_errmsg(&err));

  ten_error_deinit(&err);

  // Trigger on_init.
  ten_app_on_init(self->ten_env);
}

static void ten_app_on_endpoint_protocol_created(ten_env_t *ten_env,
                                                 ten_protocol_t *protocol,
                                                 void *cb_data) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_app_t *self = ten_env_get_attached_app(ten_env);
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");

  if (!protocol) {
    TEN_LOGE("Failed to create app endpoint protocol, FATAL ERROR.");
    ten_app_close(self, NULL);
    return;
  }

  TEN_ASSERT(ten_protocol_check_integrity(protocol, true),
             "Should not happen, %p.", protocol);

  self->endpoint_protocol = protocol;

  ten_protocol_attach_to_app(self->endpoint_protocol, self);
  ten_protocol_set_on_closed(self->endpoint_protocol,
                             ten_app_on_protocol_closed, self);

  if (!ten_app_endpoint_listen(self)) {
    ten_app_close(self, NULL);
    return;
  }

  ten_app_start_auto_start_predefined_graph_and_trigger_on_init(self);
}

void ten_app_on_all_addon_loaders_created(ten_app_t *self) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  if (!ten_app_get_predefined_graphs_from_property(self)) {
    goto error;
  }

  if (!ten_string_is_equal_c_str(&self->uri, TEN_STR_LOCALHOST) &&
      !ten_string_starts_with(&self->uri, TEN_STR_CLIENT)) {
    // Create the app listening endpoint protocol if specifying one.
    bool rc = ten_addon_create_protocol_with_uri(
        self->ten_env, ten_string_get_raw_str(&self->uri),
        TEN_PROTOCOL_ROLE_LISTEN, ten_app_on_endpoint_protocol_created, NULL,
        &err);
    if (!rc) {
      TEN_LOGW("Failed to create app endpoint protocol, %s.",
               ten_error_errmsg(&err));
      goto error;
    }
  } else {
    ten_app_start_auto_start_predefined_graph_and_trigger_on_init(self);
  }

  goto done;

error:
  ten_app_close(self, NULL);
done:
  ten_error_deinit(&err);
}

void ten_app_on_configure_done(ten_env_t *ten_env) {
  TEN_ASSERT(ten_env, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(ten_env, true),
             "Invalid use of ten_env %p.", ten_env);

  ten_app_t *self = ten_env_get_attached_app(ten_env);
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");
  TEN_ASSERT(self->loop, "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  bool rc = ten_handle_manifest_info_when_on_configure_done(
      &self->manifest_info, ten_app_get_base_dir(self), &self->manifest, &err);
  if (!rc) {
    TEN_LOGW("Failed to load app manifest data, FATAL ERROR.");
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    exit(EXIT_FAILURE);
  }

  rc = ten_handle_property_info_when_on_configure_done(
      &self->property_info, ten_app_get_base_dir(self), &self->property, &err);
  if (!rc) {
    TEN_LOGW("Failed to load app property data, FATAL ERROR.");
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    exit(EXIT_FAILURE);
  }

  if (!ten_app_handle_ten_namespace_properties(self)) {
    TEN_LOGW("Failed to determine app default property.");
  }

  ten_metadata_init_schema_store(&self->manifest, &self->schema_store);
  ten_app_adjust_and_validate_property_on_configure_done(self);

  if (ten_string_is_empty(&self->uri)) {
    ten_string_set_from_c_str(&self->uri, TEN_STR_LOCALHOST,
                              strlen(TEN_STR_LOCALHOST));
  }

  ten_addon_load_all_from_app_base_dir(ten_string_get_raw_str(&self->base_dir),
                                       &err);
  ten_addon_load_all_from_ten_package_base_dirs(&self->ten_package_base_dirs,
                                                &err);

  // @{
  // Register all addons.
  ten_builtin_extension_group_addon_register();
  ten_builtin_test_extension_addon_register();

  ten_addon_manager_t *manager = ten_addon_manager_get_instance();
  ten_addon_register_ctx_t *register_ctx = ten_addon_register_ctx_create();
  register_ctx->app = self;
  ten_addon_manager_register_all_addons(manager, (void *)register_ctx);
  ten_addon_register_ctx_destroy(register_ctx);
  // @}

  // Create addon loader singleton instances.
  bool need_to_wait_all_addon_loaders_created =
      ten_addon_loader_addons_create_singleton_instance(ten_env);
  if (!need_to_wait_all_addon_loaders_created) {
    ten_app_on_all_addon_loaders_created(self);
  }
}

void ten_app_on_configure(ten_env_t *ten_env) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");
  TEN_ASSERT(ten_env_get_attach_to(ten_env) == TEN_ENV_ATTACH_TO_APP,
             "Should not happen.");

  ten_app_t *self = ten_env_get_attached_app(ten_env);
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");

  self->manifest_info =
      ten_metadata_info_create(TEN_METADATA_ATTACH_TO_MANIFEST, self->ten_env);
  self->property_info =
      ten_metadata_info_create(TEN_METADATA_ATTACH_TO_PROPERTY, self->ten_env);

  if (self->on_configure) {
    self->on_configure(self, self->ten_env);
  } else {
    ten_app_on_configure_done(self->ten_env);
  }
}

void ten_app_on_init(ten_env_t *ten_env) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");
  TEN_ASSERT(ten_env_get_attach_to(ten_env) == TEN_ENV_ATTACH_TO_APP,
             "Should not happen.");

  ten_app_t *self = ten_env_get_attached_app(ten_env);
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");

  if (self->on_init) {
    self->on_init(self, self->ten_env);
  } else {
    ten_app_on_init_done(self->ten_env);
  }
}

static void ten_app_on_init_done_internal(ten_app_t *self) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true) && self->loop,
             "Should not happen.");
}

void ten_app_on_init_done(ten_env_t *ten_env) {
  TEN_ASSERT(ten_env, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(ten_env, true),
             "Invalid use of ten_env %p.", ten_env);

  ten_app_t *self = ten_env_get_attached_app(ten_env);
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");

  ten_app_on_init_done_internal(self);
}

static void ten_app_unregister_addons_after_app_close(ten_app_t *self) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");

  const char *disabled = getenv("TEN_DISABLE_ADDON_UNREGISTER_AFTER_APP_CLOSE");
  if (disabled && !strcmp(disabled, "true")) {
    return;
  }

  ten_unregister_all_addons_and_cleanup();
}

void ten_app_on_deinit(ten_app_t *self) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");

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
    ten_ref_dec_ref(&self->endpoint_protocol->ref);
  }

  // At the final stage of addon deinitialization, `ten_env_t::on_deinit_done`
  // is required, which in turn depends on the runloop. Therefore, the addon
  // deinitialization process must be performed _before_ the app's runloop
  // ends.
  ten_app_unregister_addons_after_app_close(self);
  // @}

  if (self->on_deinit) {
    // Call the registered on_deinit callback if exists.
    self->on_deinit(self, self->ten_env);
  } else {
    ten_env_on_deinit_done(self->ten_env, NULL);
  }
}

void ten_app_on_deinit_done(ten_env_t *ten_env) {
  TEN_ASSERT(ten_env, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(ten_env, true),
             "Invalid use of ten_env %p.", ten_env);

  ten_app_t *self = ten_env_get_attached_app(ten_env);
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");

  if (!ten_list_is_empty(&ten_env->ten_proxy_list)) {
    // There is still the presence of ten_env_proxy, so the closing process
    // cannot continue.
    TEN_LOGI("App %s cannot on_deinit_done() because of existed ten_env_proxy.",
             ten_string_get_raw_str(&self->uri));
    return;
  }

  ten_mutex_lock(self->state_lock);
  TEN_ASSERT(self->state >= TEN_APP_STATE_CLOSING, "Should not happen.");
  if (self->state == TEN_APP_STATE_CLOSED) {
    ten_mutex_unlock(self->state_lock);
    return;
  }
  self->state = TEN_APP_STATE_CLOSED;
  ten_mutex_unlock(self->state_lock);

  TEN_ENV_LOG_DEBUG_INTERNAL(ten_env, "app on_deinit_done().");

  ten_env_close(self->ten_env);
  ten_runloop_stop(self->loop);
}
