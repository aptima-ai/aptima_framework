//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_runtime/addon/addon.h"

#include "include_internal/ten_runtime/addon/addon.h"
#include "include_internal/ten_runtime/addon/addon_autoload.h"
#include "include_internal/ten_runtime/addon/addon_host.h"
#include "include_internal/ten_runtime/addon/addon_loader/addon_loader.h"
#include "include_internal/ten_runtime/addon/addon_manager.h"
#include "include_internal/ten_runtime/addon/common/store.h"
#include "include_internal/ten_runtime/addon/extension/extension.h"
#include "include_internal/ten_runtime/addon/extension_group/extension_group.h"
#include "include_internal/ten_runtime/addon/protocol/protocol.h"
#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/app/base_dir.h"
#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/metadata/metadata_info.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_runtime/app/app.h"
#include "ten_runtime/binding/common.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/signature.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"

bool ten_addon_check_integrity(ten_addon_t *self) {
  TEN_ASSERT(self, "Should not happen.");
  if (ten_signature_get(&self->signature) != TEN_ADDON_SIGNATURE) {
    return false;
  }

  return true;
}

void ten_addon_init(ten_addon_t *self, ten_addon_on_init_func_t on_init,
                    ten_addon_on_deinit_func_t on_deinit,
                    ten_addon_on_create_instance_func_t on_create_instance,
                    ten_addon_on_destroy_instance_func_t on_destroy_instance,
                    ten_addon_on_destroy_func_t on_destroy) {
  ten_binding_handle_set_me_in_target_lang((ten_binding_handle_t *)self, NULL);
  ten_signature_set(&self->signature, TEN_ADDON_SIGNATURE);

  self->on_init = on_init;
  self->on_deinit = on_deinit;
  self->on_create_instance = on_create_instance;
  self->on_destroy_instance = on_destroy_instance;
  self->on_destroy = on_destroy;

  self->user_data = NULL;
}

ten_addon_t *ten_addon_create(
    ten_addon_on_init_func_t on_init, ten_addon_on_deinit_func_t on_deinit,
    ten_addon_on_create_instance_func_t on_create_instance,
    ten_addon_on_destroy_instance_func_t on_destroy_instance,
    ten_addon_on_destroy_func_t on_destroy) {
  ten_addon_t *self = TEN_MALLOC(sizeof(ten_addon_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  ten_addon_init(self, on_init, on_deinit, on_create_instance,
                 on_destroy_instance, on_destroy);

  return self;
}

void ten_addon_destroy(ten_addon_t *self) {
  TEN_ASSERT(self && ten_addon_check_integrity(self), "Invalid argument.");
  TEN_FREE(self);
}

TEN_ADDON_TYPE ten_addon_type_from_string(const char *addon_type_str) {
  TEN_ASSERT(addon_type_str, "Invalid argument.");

  if (ten_c_string_is_equal(addon_type_str, TEN_STR_EXTENSION)) {
    return TEN_ADDON_TYPE_EXTENSION;
  } else if (ten_c_string_is_equal(addon_type_str, TEN_STR_EXTENSION_GROUP)) {
    return TEN_ADDON_TYPE_EXTENSION_GROUP;
  } else if (ten_c_string_is_equal(addon_type_str, TEN_STR_PROTOCOL)) {
    return TEN_ADDON_TYPE_PROTOCOL;
  } else if (ten_c_string_is_equal(addon_type_str, TEN_STR_ADDON_LOADER)) {
    return TEN_ADDON_TYPE_ADDON_LOADER;
  } else {
    return TEN_ADDON_TYPE_INVALID;
  }
}

const char *ten_addon_type_to_string(TEN_ADDON_TYPE type) {
  switch (type) {
    case TEN_ADDON_TYPE_EXTENSION:
      return TEN_STR_EXTENSION;
    case TEN_ADDON_TYPE_EXTENSION_GROUP:
      return TEN_STR_EXTENSION_GROUP;
    case TEN_ADDON_TYPE_PROTOCOL:
      return TEN_STR_PROTOCOL;
    case TEN_ADDON_TYPE_ADDON_LOADER:
      return TEN_STR_ADDON_LOADER;
    default:
      TEN_ASSERT(0, "Should not happen.");
      return NULL;
  }
}

/**
 * @brief The registration flow of an addon is as follows.
 *
 *   register -> on_init --> on_init_done --> add to the addon store
 *
 * Developers could override the 'on_init' function to perform user-defined
 * operations the addon needs.
 */
static void ten_addon_register_internal(ten_addon_store_t *addon_store,
                                        ten_addon_host_t *addon_host,
                                        const char *name, const char *base_dir,
                                        ten_addon_t *addon) {
  TEN_ASSERT(addon_host && ten_addon_host_check_integrity(addon_host),
             "Should not happen.");
  TEN_ASSERT(name, "Should not happen.");

  addon_host->addon = addon;
  addon_host->store = addon_store;

  TEN_ASSERT(!addon_host->ten_env, "Should not happen.");
  addon_host->ten_env = ten_env_create_for_addon(addon_host);

  ten_string_set_formatted(&addon_host->name, "%s", name);

  // In some special cases, such as built-in addons, their logic does not
  // require a base directory at all, so `NULL` might be passed as the base_dir
  // parameter value.
  if (base_dir) {
    TEN_LOGD("Addon %s base_dir: %s", name, base_dir);
    ten_addon_host_find_and_set_base_dir(addon_host, base_dir);
  }
  TEN_LOGI("Register addon: %s as %s", name,
           ten_addon_type_to_string(addon_host->type));

  ten_addon_host_load_metadata(addon_host, addon_host->ten_env,
                               addon_host->addon->on_init);
}

/**
 * @param aptima Might be the aptima of the 'engine', or the aptima of an extension
 * thread(group).
 * @param cb The callback when the creation is completed. Because there might be
 * more than one extension threads to create extensions from the corresponding
 * extension addons simultaneously. So we can _not_ save the function pointer
 * of @a cb into @a aptima, instead we need to pass the function pointer of @a cb
 * through a parameter.
 * @param cb_data The user data of @a cb. Refer the comments of @a cb for the
 * reason why we pass the pointer of @a cb_data through a parameter rather than
 * saving it into @a aptima.
 *
 * @note We will save the pointers of @a cb and @a cb_data into a 'aptima' object
 * later in the call flow when the 'aptima' object at that time belongs to a more
 * specific scope, so that we can minimize the parameters count then.
 */
bool ten_addon_create_instance_async(ten_env_t *ten_env,
                                     TEN_ADDON_TYPE addon_type,
                                     const char *addon_name,
                                     const char *instance_name,
                                     ten_env_addon_create_instance_done_cb_t cb,
                                     void *cb_data) {
  // We increase the refcount of the 'addon' here, and will decrease the
  // refcount in "aptima_(extension/extension_group)_set_addon" after the
  // extension/extension_group instance has been created.
  TEN_LOGD("Try to find addon for %s", addon_name);

  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_addon_host_t *addon_host = ten_addon_host_find(addon_type, addon_name);
  if (!addon_host) {
    ten_app_t *app = ten_env_get_belonging_app(ten_env);
    TEN_ASSERT(app && ten_app_check_integrity(
                          app,
                          // This function only needs to use the `base_dir`
                          // field of the app, and this field does not change
                          // after the app starts. Therefore, this function can
                          // be called outside of the app thread.
                          false),
               "Should not happen.");

    if (!ten_addon_try_load_specific_addon_from_app_base_dir(
            ten_string_get_raw_str(&app->base_dir), addon_type, addon_name)) {
      return false;
    }

    if (!ten_addon_try_load_specific_addon_using_all_addon_loaders(
            addon_type, addon_name)) {
      return false;
    }

    // Find again.
    addon_host = ten_addon_host_find(addon_type, addon_name);
  }

  if (!addon_host) {
    return false;
  }

  ten_addon_host_create_instance_async(addon_host, ten_env, instance_name, cb,
                                       cb_data);

  return true;
}

ten_addon_host_t *ten_addon_register(TEN_ADDON_TYPE addon_type,
                                     const char *addon_name,
                                     const char *base_dir, ten_addon_t *addon,
                                     void *register_ctx) {
  TEN_ASSERT(addon_type != TEN_ADDON_TYPE_INVALID, "Invalid argument.");

  if (!addon_name || strlen(addon_name) == 0) {
    TEN_LOGE("The addon name is required.");
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    exit(EXIT_FAILURE);
  }

  // Since addons can be dynamically loaded during the app's runtime, such as
  // extension addons, the action of checking for unregistered addons and adding
  // a new addon needs to be atomic. This ensures that the same addon is not
  // loaded multiple times.
  bool newly_created = false;
  ten_addon_host_t *addon_host = ten_addon_host_find_or_create_one_if_not_found(
      addon_type, addon_name, &newly_created);
  TEN_ASSERT(addon_host, "Should not happen.");

  if (!newly_created) {
    goto done;
  }

  if (register_ctx) {
    // If `register_ctx` exists, its content will be used to assist in the addon
    // registration process.
    ten_addon_register_ctx_t *register_ctx_ =
        (ten_addon_register_ctx_t *)register_ctx;
    addon_host->user_data = register_ctx_->app;
  }

  ten_addon_store_t *addon_store = NULL;
  switch (addon_type) {
    case TEN_ADDON_TYPE_EXTENSION:
      addon_store = ten_extension_get_global_store();
      break;
    case TEN_ADDON_TYPE_EXTENSION_GROUP:
      addon_store = ten_extension_group_get_global_store();
      break;
    case TEN_ADDON_TYPE_PROTOCOL:
      addon_store = ten_protocol_get_global_store();
      break;
    case TEN_ADDON_TYPE_ADDON_LOADER:
      addon_store = ten_addon_loader_get_global_store();
      break;
    default:
      break;
  }
  TEN_ASSERT(addon_store, "Should not happen.");

  ten_addon_register_internal(addon_store, addon_host, addon_name, base_dir,
                              addon);

done:
  return addon_host;
}

/**
 * @brief The unregistration flow of an addon is as follows.
 *
 *   unregister -> remove from the addon store -> on_deinit -> on_deinit_done
 *
 * Remove the 'addon' from the store first, so that we can ensure that there
 * are no more usages of this 'addon' anymore, and could be safe to perform
 * the on_init operation.
 *
 * The default behavior of the 'on_deinit' stage is to perform nothing but
 * 'on_deinit_done'. However, developers could override this through providing a
 * user-defined 'on_deinit' function.
 */
ten_addon_t *ten_addon_unregister(ten_addon_store_t *store,
                                  const char *addon_name) {
  TEN_ASSERT(store && addon_name, "Should not happen.");

  TEN_LOGV("Unregistered addon '%s'", addon_name);

  return ten_addon_store_del(store, addon_name);
}

static void ten_addon_unregister_all(void) {
  ten_addon_unregister_all_extension();
  ten_addon_unregister_all_extension_group();
  ten_addon_unregister_all_addon_loader();
  ten_addon_unregister_all_protocol();
}

void ten_unregister_all_addons_and_cleanup(void) {
  // Destroy all addon loaders' singleton to avoid memory leak.
  ten_addon_loader_addons_destroy_singleton_instance();

  ten_addon_unregister_all();
}
