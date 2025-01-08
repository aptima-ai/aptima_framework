//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/addon/addon.h"

#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/addon/addon_autoload.h"
#include "include_internal/axis_runtime/addon/addon_host.h"
#include "include_internal/axis_runtime/addon/addon_loader/addon_loader.h"
#include "include_internal/axis_runtime/addon/addon_manager.h"
#include "include_internal/axis_runtime/addon/common/store.h"
#include "include_internal/axis_runtime/addon/extension/extension.h"
#include "include_internal/axis_runtime/addon/extension_group/extension_group.h"
#include "include_internal/axis_runtime/addon/protocol/protocol.h"
#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/app/base_dir.h"
#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/metadata/metadata_info.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/app/app.h"
#include "axis_runtime/binding/common.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"

bool axis_addon_check_integrity(axis_addon_t *self) {
  axis_ASSERT(self, "Should not happen.");
  if (axis_signature_get(&self->signature) != axis_ADDON_SIGNATURE) {
    return false;
  }

  return true;
}

void axis_addon_init(axis_addon_t *self, axis_addon_on_init_func_t on_init,
                    axis_addon_on_deinit_func_t on_deinit,
                    axis_addon_on_create_instance_func_t on_create_instance,
                    axis_addon_on_destroy_instance_func_t on_destroy_instance,
                    axis_addon_on_destroy_func_t on_destroy) {
  axis_binding_handle_set_me_in_target_lang((axis_binding_handle_t *)self, NULL);
  axis_signature_set(&self->signature, axis_ADDON_SIGNATURE);

  self->on_init = on_init;
  self->on_deinit = on_deinit;
  self->on_create_instance = on_create_instance;
  self->on_destroy_instance = on_destroy_instance;
  self->on_destroy = on_destroy;

  self->user_data = NULL;
}

axis_addon_t *axis_addon_create(
    axis_addon_on_init_func_t on_init, axis_addon_on_deinit_func_t on_deinit,
    axis_addon_on_create_instance_func_t on_create_instance,
    axis_addon_on_destroy_instance_func_t on_destroy_instance,
    axis_addon_on_destroy_func_t on_destroy) {
  axis_addon_t *self = axis_MALLOC(sizeof(axis_addon_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_addon_init(self, on_init, on_deinit, on_create_instance,
                 on_destroy_instance, on_destroy);

  return self;
}

void axis_addon_destroy(axis_addon_t *self) {
  axis_ASSERT(self && axis_addon_check_integrity(self), "Invalid argument.");
  axis_FREE(self);
}

axis_ADDON_TYPE axis_addon_type_from_string(const char *addon_type_str) {
  axis_ASSERT(addon_type_str, "Invalid argument.");

  if (axis_c_string_is_equal(addon_type_str, axis_STR_EXTENSION)) {
    return axis_ADDON_TYPE_EXTENSION;
  } else if (axis_c_string_is_equal(addon_type_str, axis_STR_EXTENSION_GROUP)) {
    return axis_ADDON_TYPE_EXTENSION_GROUP;
  } else if (axis_c_string_is_equal(addon_type_str, axis_STR_PROTOCOL)) {
    return axis_ADDON_TYPE_PROTOCOL;
  } else if (axis_c_string_is_equal(addon_type_str, axis_STR_ADDON_LOADER)) {
    return axis_ADDON_TYPE_ADDON_LOADER;
  } else {
    return axis_ADDON_TYPE_INVALID;
  }
}

const char *axis_addon_type_to_string(axis_ADDON_TYPE type) {
  switch (type) {
    case axis_ADDON_TYPE_EXTENSION:
      return axis_STR_EXTENSION;
    case axis_ADDON_TYPE_EXTENSION_GROUP:
      return axis_STR_EXTENSION_GROUP;
    case axis_ADDON_TYPE_PROTOCOL:
      return axis_STR_PROTOCOL;
    case axis_ADDON_TYPE_ADDON_LOADER:
      return axis_STR_ADDON_LOADER;
    default:
      axis_ASSERT(0, "Should not happen.");
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
static void axis_addon_register_internal(axis_addon_store_t *addon_store,
                                        axis_addon_host_t *addon_host,
                                        const char *name, const char *base_dir,
                                        axis_addon_t *addon) {
  axis_ASSERT(addon_host && axis_addon_host_check_integrity(addon_host),
             "Should not happen.");
  axis_ASSERT(name, "Should not happen.");

  addon_host->addon = addon;
  addon_host->store = addon_store;

  axis_ASSERT(!addon_host->axis_env, "Should not happen.");
  addon_host->axis_env = axis_env_create_for_addon(addon_host);

  axis_string_set_formatted(&addon_host->name, "%s", name);

  // In some special cases, such as built-in addons, their logic does not
  // require a base directory at all, so `NULL` might be passed as the base_dir
  // parameter value.
  if (base_dir) {
    axis_LOGD("Addon %s base_dir: %s", name, base_dir);
    axis_addon_host_find_and_set_base_dir(addon_host, base_dir);
  }
  axis_LOGI("Register addon: %s as %s", name,
           axis_addon_type_to_string(addon_host->type));

  axis_addon_host_load_metadata(addon_host, addon_host->axis_env,
                               addon_host->addon->on_init);
}

/**
 * @param ten Might be the ten of the 'engine', or the ten of an extension
 * thread(group).
 * @param cb The callback when the creation is completed. Because there might be
 * more than one extension threads to create extensions from the corresponding
 * extension addons simultaneously. So we can _not_ save the function pointer
 * of @a cb into @a ten, instead we need to pass the function pointer of @a cb
 * through a parameter.
 * @param cb_data The user data of @a cb. Refer the comments of @a cb for the
 * reason why we pass the pointer of @a cb_data through a parameter rather than
 * saving it into @a ten.
 *
 * @note We will save the pointers of @a cb and @a cb_data into a 'ten' object
 * later in the call flow when the 'ten' object at that time belongs to a more
 * specific scope, so that we can minimize the parameters count then.
 */
bool axis_addon_create_instance_async(axis_env_t *axis_env,
                                     axis_ADDON_TYPE addon_type,
                                     const char *addon_name,
                                     const char *instance_name,
                                     axis_env_addon_create_instance_done_cb_t cb,
                                     void *cb_data) {
  // We increase the refcount of the 'addon' here, and will decrease the
  // refcount in "axis_(extension/extension_group)_set_addon" after the
  // extension/extension_group instance has been created.
  axis_LOGD("Try to find addon for %s", addon_name);

  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_addon_host_t *addon_host = axis_addon_host_find(addon_type, addon_name);
  if (!addon_host) {
    axis_app_t *app = axis_env_get_belonging_app(axis_env);
    axis_ASSERT(app && axis_app_check_integrity(
                          app,
                          // This function only needs to use the `base_dir`
                          // field of the app, and this field does not change
                          // after the app starts. Therefore, this function can
                          // be called outside of the app thread.
                          false),
               "Should not happen.");

    // First, try to load it using the built-in native addon loader (i.e.,
    // `dlopen`).
    if (!axis_addon_try_load_specific_addon_using_native_addon_loader(
            axis_string_get_raw_str(&app->base_dir), addon_type, addon_name)) {
      axis_LOGI(
          "Unable to load addon %s:%s using native addon loader, will try "
          "other methods.",
          axis_addon_type_to_string(addon_type), addon_name);
    }

    if (!axis_addon_try_load_specific_addon_using_all_addon_loaders(
            addon_type, addon_name)) {
      axis_LOGI("Unable to load addon %s:%s using all installed addon loaders.",
               axis_addon_type_to_string(addon_type), addon_name);
    }

    // Find again.
    addon_host = axis_addon_host_find(addon_type, addon_name);
  }

  if (!addon_host) {
    axis_LOGE(
        "Failed to find addon %s:%s, please make sure the addon is installed.",
        axis_addon_type_to_string(addon_type), addon_name);
    return false;
  }

  axis_addon_host_create_instance_async(addon_host, axis_env, instance_name, cb,
                                       cb_data);

  return true;
}

axis_addon_host_t *axis_addon_register(axis_ADDON_TYPE addon_type,
                                     const char *addon_name,
                                     const char *base_dir, axis_addon_t *addon,
                                     void *register_ctx) {
  axis_ASSERT(addon_type != axis_ADDON_TYPE_INVALID, "Invalid argument.");

  if (!addon_name || strlen(addon_name) == 0) {
    axis_LOGE("The addon name is required.");
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    exit(EXIT_FAILURE);
  }

  // Since addons can be dynamically loaded during the app's runtime, such as
  // extension addons, the action of checking for unregistered addons and adding
  // a new addon needs to be atomic. This ensures that the same addon is not
  // loaded multiple times.
  bool newly_created = false;
  axis_addon_host_t *addon_host = axis_addon_host_find_or_create_one_if_not_found(
      addon_type, addon_name, &newly_created);
  axis_ASSERT(addon_host, "Should not happen.");

  if (!newly_created) {
    goto done;
  }

  if (register_ctx) {
    // If `register_ctx` exists, its content will be used to assist in the addon
    // registration process.
    axis_addon_register_ctx_t *register_ctx_ =
        (axis_addon_register_ctx_t *)register_ctx;
    addon_host->user_data = register_ctx_->app;
  }

  axis_addon_store_t *addon_store = NULL;
  switch (addon_type) {
    case axis_ADDON_TYPE_EXTENSION:
      addon_store = axis_extension_get_global_store();
      break;
    case axis_ADDON_TYPE_EXTENSION_GROUP:
      addon_store = axis_extension_group_get_global_store();
      break;
    case axis_ADDON_TYPE_PROTOCOL:
      addon_store = axis_protocol_get_global_store();
      break;
    case axis_ADDON_TYPE_ADDON_LOADER:
      addon_store = axis_addon_loader_get_global_store();
      break;
    default:
      break;
  }
  axis_ASSERT(addon_store, "Should not happen.");

  axis_addon_register_internal(addon_store, addon_host, addon_name, base_dir,
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
axis_addon_t *axis_addon_unregister(axis_addon_store_t *store,
                                  const char *addon_name) {
  axis_ASSERT(store && addon_name, "Should not happen.");

  axis_LOGV("Unregistered addon '%s'", addon_name);

  return axis_addon_store_del(store, addon_name);
}

static void axis_addon_unregister_all_except_addon_loader_addon(void) {
  axis_addon_unregister_all_extension();
  axis_addon_unregister_all_extension_group();
  axis_addon_unregister_all_protocol();
}

void axis_unregister_all_addons_and_cleanup(void) {
  // Since Python addons (e.g., Python extension addons) require access to the
  // Python VM when performing `addon_t` deinitialization, and the Python addon
  // loader will destroy the Python VM during its own destruction, the Python
  // addon loader must only be unloaded after all other non-addon-loader types
  // of addons have been fully unloaded. Only then can the addon loader itself
  // be unloaded.

  axis_addon_unregister_all_except_addon_loader_addon();

  // Destroy all addon loaders' singleton to avoid memory leak.
  axis_addon_loader_addons_destroy_singleton_instance();

  axis_addon_unregister_all_addon_loader();
}
