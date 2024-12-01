//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_runtime/addon/addon.h"

#include "include_internal/ten_runtime/addon/addon.h"
#include "include_internal/ten_runtime/addon/common/store.h"
#include "include_internal/ten_runtime/addon/extension/extension.h"
#include "include_internal/ten_runtime/addon/extension_group/extension_group.h"
#include "include_internal/ten_runtime/addon/protocol/protocol.h"
#include "include_internal/ten_runtime/app/base_dir.h"
#include "include_internal/ten_runtime/common/base_dir.h"
#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/metadata/metadata_info.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_runtime/app/app.h"
#include "ten_runtime/binding/common.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/path.h"
#include "ten_utils/lib/signature.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

bool ten_addon_host_check_integrity(ten_addon_host_t *self) {
  TEN_ASSERT(self, "Should not happen.");
  if (ten_signature_get(&self->signature) != TEN_ADDON_HOST_SIGNATURE) {
    return false;
  }

  return true;
}

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

static void ten_addon_deinit(ten_addon_host_t *self) {
  TEN_ASSERT(self, "Should not happen.");
  TEN_ASSERT(self->addon, "Should not happen.");

  if (self->addon->on_deinit) {
    self->addon->on_deinit(self->addon, self->ten_env);
  } else {
    ten_env_on_deinit_done(self->ten_env, NULL);
  }
}

static void ten_addon_on_end_of_life(TEN_UNUSED ten_ref_t *ref,
                                     void *supervisee) {
  ten_addon_host_t *addon = supervisee;
  TEN_ASSERT(addon, "Invalid argument.");

  ten_addon_deinit(addon);
}

void ten_addon_host_init(ten_addon_host_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  ten_signature_set(&self->signature, TEN_ADDON_HOST_SIGNATURE);

  ten_string_init(&self->name);
  ten_string_init(&self->base_dir);

  ten_value_init_object_with_move(&self->manifest, NULL);
  ten_value_init_object_with_move(&self->property, NULL);

  ten_ref_init(&self->ref, self, ten_addon_on_end_of_life);
  self->ten_env = NULL;

  self->manifest_info = NULL;
  self->property_info = NULL;

  self->user_data = NULL;
}

void ten_addon_host_destroy(ten_addon_host_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  ten_signature_set(&self->signature, 0);

  ten_string_deinit(&self->name);
  ten_string_deinit(&self->base_dir);

  ten_value_deinit(&self->manifest);
  ten_value_deinit(&self->property);

  if (self->manifest_info) {
    ten_metadata_info_destroy(self->manifest_info);
    self->manifest_info = NULL;
  }
  if (self->property_info) {
    ten_metadata_info_destroy(self->property_info);
    self->property_info = NULL;
  }

  ten_env_destroy(self->ten_env);
  TEN_FREE(self);
}

static void ten_addon_load_metadata(ten_addon_host_t *self, ten_env_t *ten_env,
                                    ten_addon_on_init_func_t on_init) {
  TEN_ASSERT(self && ten_addon_host_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true) &&
                 ten_env_get_attached_addon(ten_env) == self,
             "Should not happen.");

  self->manifest_info =
      ten_metadata_info_create(TEN_METADATA_ATTACH_TO_MANIFEST, ten_env);
  self->property_info =
      ten_metadata_info_create(TEN_METADATA_ATTACH_TO_PROPERTY, ten_env);

  if (on_init) {
    on_init(self->addon, ten_env);
  } else {
    ten_env_on_init_done(ten_env, NULL);
  }
}

TEN_ADDON_TYPE ten_addon_type_from_string(const char *addon_type_str) {
  TEN_ASSERT(addon_type_str, "Invalid argument.");

  if (ten_c_string_is_equal(addon_type_str, TEN_STR_EXTENSION)) {
    return TEN_ADDON_TYPE_EXTENSION;
  } else if (ten_c_string_is_equal(addon_type_str, TEN_STR_EXTENSION_GROUP)) {
    return TEN_ADDON_TYPE_EXTENSION_GROUP;
  } else if (ten_c_string_is_equal(addon_type_str, TEN_STR_PROTOCOL)) {
    return TEN_ADDON_TYPE_PROTOCOL;
  } else {
    return TEN_ADDON_TYPE_INVALID;
  }
}

static const char *ten_addon_type_to_string(TEN_ADDON_TYPE type) {
  switch (type) {
    case TEN_ADDON_TYPE_EXTENSION:
      return TEN_STR_EXTENSION;
    case TEN_ADDON_TYPE_EXTENSION_GROUP:
      return TEN_STR_EXTENSION_GROUP;
    case TEN_ADDON_TYPE_PROTOCOL:
      return TEN_STR_PROTOCOL;
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
void ten_addon_register(ten_addon_store_t *addon_store,
                        ten_addon_host_t *addon_host, const char *name,
                        const char *base_dir, ten_addon_t *addon) {
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
    ten_addon_find_and_set_base_dir(addon_host, base_dir);
  }
  TEN_LOGI("Register addon: %s as %s", name,
           ten_addon_type_to_string(addon_host->type));

  ten_addon_load_metadata(addon_host, addon_host->ten_env,
                          addon_host->addon->on_init);
}

ten_addon_on_create_instance_info_t *ten_addon_on_create_instance_info_create(
    const char *addon_name, const char *instance_name,
    TEN_ADDON_TYPE addon_type, ten_env_addon_create_instance_done_cb_t cb,
    void *cb_data) {
  TEN_ASSERT(addon_name && instance_name, "Should not happen.");

  ten_addon_on_create_instance_info_t *self =
      (ten_addon_on_create_instance_info_t *)TEN_MALLOC(
          sizeof(ten_addon_on_create_instance_info_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  ten_string_init_formatted(&self->addon_name, "%s", addon_name);
  ten_string_init_formatted(&self->instance_name, "%s", instance_name);
  self->addon_type = addon_type;
  self->cb = cb;
  self->cb_data = cb_data;

  return self;
}

void ten_addon_on_create_instance_info_destroy(
    ten_addon_on_create_instance_info_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  ten_string_deinit(&self->addon_name);
  ten_string_deinit(&self->instance_name);

  TEN_FREE(self);
}

ten_addon_on_destroy_instance_info_t *
ten_addon_host_on_destroy_instance_info_create(
    ten_addon_host_t *self, void *instance,
    ten_env_addon_destroy_instance_done_cb_t cb, void *cb_data) {
  TEN_ASSERT(self && instance, "Should not happen.");

  ten_addon_on_destroy_instance_info_t *info =
      (ten_addon_on_destroy_instance_info_t *)TEN_MALLOC(
          sizeof(ten_addon_on_destroy_instance_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  info->addon_host = self;
  info->instance = instance;
  info->cb = cb;
  info->cb_data = cb_data;

  return info;
}

void ten_addon_on_destroy_instance_info_destroy(
    ten_addon_on_destroy_instance_info_t *self) {
  TEN_ASSERT(self && self->addon_host && self->instance, "Should not happen.");
  TEN_FREE(self);
}

ten_addon_host_t *ten_addon_host_create(TEN_ADDON_TYPE type) {
  ten_addon_host_t *self =
      (ten_addon_host_t *)TEN_MALLOC(sizeof(ten_addon_host_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  self->type = type;
  ten_addon_host_init(self);

  return self;
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

ten_addon_host_t *ten_addon_host_find(const char *addon_name,
                                      TEN_ADDON_TYPE type) {
  TEN_ASSERT(addon_name, "Should not happen.");

  switch (type) {
    case TEN_ADDON_TYPE_EXTENSION:
      return ten_addon_store_find(ten_extension_get_global_store(), addon_name);

    case TEN_ADDON_TYPE_EXTENSION_GROUP:
      return ten_addon_store_find(ten_extension_group_get_global_store(),
                                  addon_name);

    case TEN_ADDON_TYPE_PROTOCOL:
      return ten_addon_store_find(ten_protocol_get_global_store(), addon_name);

    default:
      TEN_ASSERT(0, "Should not happen.");
      break;
  }

  return NULL;
}

static ten_addon_context_t *ten_addon_context_create(void) {
  ten_addon_context_t *self = TEN_MALLOC(sizeof(ten_addon_context_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  return self;
}

void ten_addon_context_destroy(ten_addon_context_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  TEN_FREE(self);
}

/**
 * @param ten Might be the ten of the 'engine', or the ten of an extension
 * thread (group).
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
static void ten_addon_host_create_instance_async(
    ten_addon_host_t *self, ten_env_t *ten_env, const char *name,
    ten_env_addon_create_instance_done_cb_t cb, void *cb_data) {
  TEN_ASSERT(self && ten_addon_host_check_integrity(self) && name,
             "Should not happen.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_addon_context_t *addon_context = ten_addon_context_create();
  addon_context->caller_ten = ten_env;
  addon_context->create_instance_done_cb = cb;
  addon_context->create_instance_done_cb_data = cb_data;

  if (self->addon->on_create_instance) {
    TEN_ASSERT(self->addon->on_create_instance, "Should not happen.");
    self->addon->on_create_instance(self->addon, self->ten_env, name,
                                    addon_context);
  } else {
    TEN_ASSERT(0,
               "Failed to create instance from %s, because it does not define "
               "create() function.",
               name);
  }
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
bool ten_addon_create_instance_async(ten_env_t *ten_env, const char *addon_name,
                                     const char *instance_name,
                                     TEN_ADDON_TYPE type,
                                     ten_env_addon_create_instance_done_cb_t cb,
                                     void *cb_data) {
  // We increase the refcount of the 'addon' here, and will decrease the
  // refcount in "ten_(extension/extension_group)_set_addon" after the
  // extension/extension_group instance has been created.
  TEN_LOGD("Try to find addon for %s", addon_name);

  ten_addon_host_t *addon = ten_addon_host_find(addon_name, type);
  if (!addon) {
    return false;
  }

  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_addon_host_create_instance_async(addon, ten_env, instance_name, cb,
                                       cb_data);

  return true;
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
bool ten_addon_host_destroy_instance_async(
    ten_addon_host_t *self, ten_env_t *ten_env, void *instance,
    ten_env_addon_destroy_instance_done_cb_t cb, void *cb_data) {
  TEN_ASSERT(self && ten_addon_host_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");
  TEN_ASSERT(instance, "Should not happen.");

  ten_addon_context_t *addon_context = ten_addon_context_create();
  addon_context->caller_ten = ten_env;
  addon_context->destroy_instance_done_cb = cb;
  addon_context->destroy_instance_done_cb_data = cb_data;

  if (self->addon->on_destroy_instance) {
    TEN_ASSERT(self->addon->on_destroy_instance, "Should not happen.");
    self->addon->on_destroy_instance(self->addon, self->ten_env, instance,
                                     addon_context);
  } else {
    TEN_ASSERT(0,
               "Failed to destroy an instance from %s, because it does not "
               "define a destroy() function.",
               ten_string_get_raw_str(&self->name));
  }

  return true;
}

bool ten_addon_host_destroy_instance(ten_addon_host_t *self, ten_env_t *ten_env,
                                     void *instance) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, false),
             "Should not happen.");
  TEN_ASSERT(self && instance, "Should not happen.");
  TEN_ASSERT(self->addon->on_destroy_instance, "Should not happen.");

  self->addon->on_destroy_instance(self->addon, self->ten_env, instance, NULL);

  return true;
}

const char *ten_addon_host_get_name(ten_addon_host_t *self) {
  TEN_ASSERT(self && ten_addon_host_check_integrity(self), "Invalid argument.");
  return ten_string_get_raw_str(&self->name);
}

ten_string_t *ten_addon_find_base_dir_from_app(const char *addon_type,
                                               const char *addon_name) {
  TEN_ASSERT(
      addon_type && strlen(addon_type) && addon_name && strlen(addon_name),
      "Invalid argument.");

  ten_string_t *base_dir = ten_find_app_base_dir();
  if (!base_dir || ten_string_is_empty(base_dir)) {
    return NULL;
  }

  ten_string_set_formatted(base_dir, "%s/%s/%s/%s",
                           ten_string_get_raw_str(base_dir),
                           TEN_STR_TEN_PACKAGES, addon_type, addon_name);
  ten_path_to_system_flavor(base_dir);

  if (!ten_path_exists(ten_string_get_raw_str(base_dir))) {
    // Clear the `base_dir` to avoid users to use an invalid path.
    ten_string_destroy(base_dir);
  }

  return NULL;
}

void ten_addon_find_and_set_base_dir(ten_addon_host_t *self,
                                     const char *start_path) {
  TEN_ASSERT(start_path && self && ten_addon_host_check_integrity(self),
             "Should not happen.");

  ten_string_t *base_dir =
      ten_find_base_dir(start_path, ten_addon_type_to_string(self->type),
                        ten_string_get_raw_str(&self->name));
  if (base_dir) {
    ten_string_copy(&self->base_dir, base_dir);
    ten_string_destroy(base_dir);
  } else {
    // If the addon's base dir cannot be found by searching upward through the
    // parent folders, simply trust the passed-in parameter as the addon’s base
    // dir.
    ten_string_set_from_c_str(&self->base_dir, start_path, strlen(start_path));
  }
}

const char *ten_addon_host_get_base_dir(ten_addon_host_t *self) {
  TEN_ASSERT(self && ten_addon_host_check_integrity(self), "Invalid argument.");
  return ten_string_get_raw_str(&self->base_dir);
}
