//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/addon/addon_autoload.h"
#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/app/close.h"
#include "include_internal/ten_runtime/app/endpoint.h"
#include "include_internal/ten_runtime/app/engine_interface.h"
#include "include_internal/ten_runtime/app/metadata.h"
#include "include_internal/ten_runtime/app/migration.h"
#include "include_internal/ten_runtime/app/predefined_graph.h"
#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/metadata/metadata.h"
#include "include_internal/ten_runtime/metadata/metadata_info.h"
#include "include_internal/ten_runtime/schema_store/store.h"
#include "include_internal/ten_runtime/ten_env/metadata_cb.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_utils/macro/check.h"
#include "ten_runtime/app/app.h"
#include "ten_runtime/ten_env/internal/on_xxx_done.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/lib/error.h"

void ten_app_on_init(ten_env_t *ten_env) {
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

  if (self->on_init) {
    self->on_init(self, self->ten_env);
  } else {
    ten_app_on_init_done(self->ten_env);
  }
}

static void ten_app_adjust_and_validate_property_on_init(ten_app_t *self) {
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

static void ten_app_on_init_done_internal(ten_app_t *self) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true) && self->loop,
             "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  bool rc = ten_handle_manifest_info_when_on_init_done(
      &self->manifest_info, ten_string_get_raw_str(ten_app_get_base_dir(self)),
      &self->manifest, &err);
  if (!rc) {
    TEN_LOGW("Failed to load app manifest data, FATAL ERROR.");
    exit(EXIT_FAILURE);
  }

  rc = ten_handle_property_info_when_on_init_done(
      &self->property_info, ten_string_get_raw_str(ten_app_get_base_dir(self)),
      &self->property, &err);
  if (!rc) {
    TEN_LOGW("Failed to load app property data, FATAL ERROR.");
    exit(EXIT_FAILURE);
  }

  if (!ten_app_handle_ten_namespace_properties(self)) {
    TEN_LOGW("App determine default prop failed! \n");
  }

  ten_metadata_init_schema_store(&self->manifest, &self->schema_store);
  ten_app_adjust_and_validate_property_on_init(self);

  if (ten_string_is_empty(&self->uri)) {
    ten_string_copy_c_str(&self->uri, TEN_STR_LOCALHOST,
                          strlen(TEN_STR_LOCALHOST));
  }

  ten_addon_load_all(&err);

  if (!ten_app_get_predefined_graphs_from_property(self)) {
    return;
  }

  // Create the protocol context store _BEFORE_ 'ten_app_create_endpoint()', the
  // context data of the endpoint protocol will depend on the context store.
  ten_app_create_protocol_context_store(self);

  if (!ten_string_is_equal_c_str(&self->uri, TEN_STR_LOCALHOST)) {
    // Create the app listening endpoint if specifying one.
    if (!ten_app_create_endpoint(self, &self->uri)) {
      ten_app_close(self, NULL);
    }
  }

  rc = ten_app_start_auto_start_predefined_graph(self, &err);
  TEN_ASSERT(rc, "Should not happen.");

  ten_error_deinit(&err);
}

void ten_app_on_init_done(ten_env_t *ten_env) {
  TEN_ASSERT(ten_env, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(ten_env, true),
             "Invalid use of ten_env %p.", ten_env);

  ten_app_t *self = ten_env_get_attached_app(ten_env);
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");

  ten_app_on_init_done_internal(self);
}

void ten_app_on_deinit(ten_app_t *self) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");

  // The world outside of TEN would do some operations after the app_run()
  // returns, so it's best to perform the on_deinit callback _before_ the
  // runloop is stopped.

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

  TEN_LOGD("app::on_deinit_done().");

  ten_env_close(self->ten_env);
  ten_runloop_stop(self->loop);
}
