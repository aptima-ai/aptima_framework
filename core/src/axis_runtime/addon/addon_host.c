//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/addon/addon_host.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/addon/addon_loader/addon_loader.h"
#include "include_internal/axis_runtime/addon/extension/extension.h"
#include "include_internal/axis_runtime/addon/extension_group/extension_group.h"
#include "include_internal/axis_runtime/addon/protocol/protocol.h"
#include "include_internal/axis_runtime/common/base_dir.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/macro/memory.h"

bool axis_addon_host_check_integrity(axis_addon_host_t *self) {
  axis_ASSERT(self, "Should not happen.");
  if (axis_signature_get(&self->signature) != axis_ADDON_HOST_SIGNATURE) {
    return false;
  }

  return true;
}

static void axis_addon_host_deinit(axis_addon_host_t *self) {
  axis_ASSERT(self, "Should not happen.");
  axis_ASSERT(self->addon, "Should not happen.");

  if (self->addon->on_deinit) {
    self->addon->on_deinit(self->addon, self->axis_env);
  } else {
    axis_env_on_deinit_done(self->axis_env, NULL);
  }
}

static void axis_addon_on_end_of_life(axis_UNUSED axis_ref_t *ref,
                                     void *supervisee) {
  axis_addon_host_t *addon = supervisee;
  axis_ASSERT(addon, "Invalid argument.");

  axis_addon_host_deinit(addon);
}

void axis_addon_host_init(axis_addon_host_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_signature_set(&self->signature, axis_ADDON_HOST_SIGNATURE);

  axis_string_init(&self->name);
  axis_string_init(&self->base_dir);

  axis_value_init_object_with_move(&self->manifest, NULL);
  axis_value_init_object_with_move(&self->property, NULL);

  axis_ref_init(&self->ref, self, axis_addon_on_end_of_life);
  self->axis_env = NULL;

  self->manifest_info = NULL;
  self->property_info = NULL;

  self->user_data = NULL;
}

void axis_addon_host_destroy(axis_addon_host_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_signature_set(&self->signature, 0);

  axis_string_deinit(&self->name);
  axis_string_deinit(&self->base_dir);

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

  axis_env_destroy(self->axis_env);
  axis_FREE(self);
}

bool axis_addon_host_destroy_instance(axis_addon_host_t *self, axis_env_t *axis_env,
                                     void *instance) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, false),
             "Should not happen.");
  axis_ASSERT(self && instance, "Should not happen.");
  axis_ASSERT(self->addon->on_destroy_instance, "Should not happen.");

  self->addon->on_destroy_instance(self->addon, self->axis_env, instance, NULL);

  return true;
}

const char *axis_addon_host_get_name(axis_addon_host_t *self) {
  axis_ASSERT(self && axis_addon_host_check_integrity(self), "Invalid argument.");
  return axis_string_get_raw_str(&self->name);
}

void axis_addon_host_find_and_set_base_dir(axis_addon_host_t *self,
                                          const char *start_path) {
  axis_ASSERT(start_path && self && axis_addon_host_check_integrity(self),
             "Should not happen.");

  axis_string_t *base_dir =
      axis_find_base_dir(start_path, axis_addon_type_to_string(self->type),
                        axis_string_get_raw_str(&self->name));
  if (base_dir) {
    axis_string_copy(&self->base_dir, base_dir);
    axis_string_destroy(base_dir);
  } else {
    // If the addon's base dir cannot be found by searching upward through the
    // parent folders, simply trust the passed-in parameter as the addon’s base
    // dir.
    axis_string_set_from_c_str(&self->base_dir, start_path, strlen(start_path));
  }
}

const char *axis_addon_host_get_base_dir(axis_addon_host_t *self) {
  axis_ASSERT(self && axis_addon_host_check_integrity(self), "Invalid argument.");
  return axis_string_get_raw_str(&self->base_dir);
}

static axis_addon_context_t *axis_addon_context_create(void) {
  axis_addon_context_t *self = axis_MALLOC(sizeof(axis_addon_context_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  return self;
}

void axis_addon_context_destroy(axis_addon_context_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_FREE(self);
}

/**
 * @param aptima Might be the aptima of the 'engine', or the aptima of an extension
 * thread (group).
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
void axis_addon_host_create_instance_async(
    axis_addon_host_t *self, axis_env_t *axis_env, const char *name,
    axis_env_addon_create_instance_done_cb_t cb, void *cb_data) {
  axis_ASSERT(self && axis_addon_host_check_integrity(self) && name,
             "Should not happen.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_addon_context_t *addon_context = axis_addon_context_create();
  addon_context->caller_axis_env = axis_env;
  addon_context->create_instance_done_cb = cb;
  addon_context->create_instance_done_cb_data = cb_data;

  if (self->addon->on_create_instance) {
    axis_ASSERT(self->addon->on_create_instance, "Should not happen.");
    self->addon->on_create_instance(self->addon, self->axis_env, name,
                                    addon_context);
  } else {
    axis_ASSERT(0,
               "Failed to create instance from %s, because it does not define "
               "create() function.",
               name);
  }
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
bool axis_addon_host_destroy_instance_async(
    axis_addon_host_t *self, axis_env_t *axis_env, void *instance,
    axis_env_addon_destroy_instance_done_cb_t cb, void *cb_data) {
  axis_ASSERT(self && axis_addon_host_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");
  axis_ASSERT(instance, "Should not happen.");

  axis_addon_context_t *addon_context = axis_addon_context_create();
  addon_context->caller_axis_env = axis_env;
  addon_context->destroy_instance_done_cb = cb;
  addon_context->destroy_instance_done_cb_data = cb_data;

  if (self->addon->on_destroy_instance) {
    axis_ASSERT(self->addon->on_destroy_instance, "Should not happen.");
    self->addon->on_destroy_instance(self->addon, self->axis_env, instance,
                                     addon_context);
  } else {
    axis_ASSERT(0,
               "Failed to destroy an instance from %s, because it does not "
               "define a destroy() function.",
               axis_string_get_raw_str(&self->name));
  }

  return true;
}

axis_addon_host_on_destroy_instance_ctx_t *
axis_addon_host_on_destroy_instance_ctx_create(
    axis_addon_host_t *self, void *instance,
    axis_env_addon_destroy_instance_done_cb_t cb, void *cb_data) {
  axis_ASSERT(self && instance, "Should not happen.");

  axis_addon_host_on_destroy_instance_ctx_t *ctx =
      (axis_addon_host_on_destroy_instance_ctx_t *)axis_MALLOC(
          sizeof(axis_addon_host_on_destroy_instance_ctx_t));
  axis_ASSERT(ctx, "Failed to allocate memory.");

  ctx->addon_host = self;
  ctx->instance = instance;
  ctx->cb = cb;
  ctx->cb_data = cb_data;

  return ctx;
}

void axis_addon_host_on_destroy_instance_ctx_destroy(
    axis_addon_host_on_destroy_instance_ctx_t *self) {
  axis_ASSERT(self && self->addon_host && self->instance, "Should not happen.");
  axis_FREE(self);
}

axis_addon_host_t *axis_addon_host_create(axis_ADDON_TYPE type) {
  axis_addon_host_t *self =
      (axis_addon_host_t *)axis_MALLOC(sizeof(axis_addon_host_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  self->type = type;
  axis_addon_host_init(self);

  return self;
}

axis_addon_host_t *axis_addon_host_find(axis_ADDON_TYPE addon_type,
                                      const char *addon_name) {
  axis_ASSERT(addon_name, "Should not happen.");

  switch (addon_type) {
    case axis_ADDON_TYPE_EXTENSION:
      return axis_addon_store_find(axis_extension_get_global_store(), addon_name);

    case axis_ADDON_TYPE_EXTENSION_GROUP:
      return axis_addon_store_find(axis_extension_group_get_global_store(),
                                  addon_name);

    case axis_ADDON_TYPE_PROTOCOL:
      return axis_addon_store_find(axis_protocol_get_global_store(), addon_name);

    case axis_ADDON_TYPE_ADDON_LOADER:
      return axis_addon_store_find(axis_addon_loader_get_global_store(),
                                  addon_name);

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  return NULL;
}

axis_addon_host_t *axis_addon_host_find_or_create_one_if_not_found(
    axis_ADDON_TYPE addon_type, const char *addon_name, bool *newly_created) {
  axis_ASSERT(addon_name, "Should not happen.");

  switch (addon_type) {
    case axis_ADDON_TYPE_EXTENSION:
      return axis_addon_store_find_or_create_one_if_not_found(
          axis_extension_get_global_store(), addon_type, addon_name,
          newly_created);

    case axis_ADDON_TYPE_EXTENSION_GROUP:
      return axis_addon_store_find_or_create_one_if_not_found(
          axis_extension_group_get_global_store(), addon_type, addon_name,
          newly_created);

    case axis_ADDON_TYPE_PROTOCOL:
      return axis_addon_store_find_or_create_one_if_not_found(
          axis_protocol_get_global_store(), addon_type, addon_name,
          newly_created);

    case axis_ADDON_TYPE_ADDON_LOADER:
      return axis_addon_store_find_or_create_one_if_not_found(
          axis_addon_loader_get_global_store(), addon_type, addon_name,
          newly_created);

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  return NULL;
}

void axis_addon_host_load_metadata(axis_addon_host_t *self, axis_env_t *axis_env,
                                  axis_addon_on_init_func_t on_init) {
  axis_ASSERT(self && axis_addon_host_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true) &&
                 axis_env_get_attached_addon(axis_env) == self,
             "Should not happen.");

  self->manifest_info =
      axis_metadata_info_create(axis_METADATA_ATTACH_TO_MANIFEST, axis_env);
  self->property_info =
      axis_metadata_info_create(axis_METADATA_ATTACH_TO_PROPERTY, axis_env);

  if (on_init) {
    on_init(self->addon, axis_env);
  } else {
    axis_env_on_init_done(axis_env, NULL);
  }
}
