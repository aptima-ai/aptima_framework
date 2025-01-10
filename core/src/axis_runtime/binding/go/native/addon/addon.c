//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/binding/go/interface/aptima/addon.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/addon/extension/extension.h"
#include "include_internal/axis_runtime/binding/go/addon/addon.h"
#include "include_internal/axis_runtime/binding/go/extension/extension.h"
#include "include_internal/axis_runtime/binding/go/internal/common.h"
#include "include_internal/axis_runtime/binding/go/axis_env/axis_env.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/addon/addon.h"
#include "axis_runtime/addon/extension/extension.h"
#include "axis_runtime/binding/common.h"
#include "axis_runtime/binding/go/interface/aptima/common.h"
#include "axis_runtime/binding/go/interface/aptima/axis_env.h"
#include "axis_runtime/axis_env/internal/on_xxx_done.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"

void tenGoAddonOnInit(axis_go_handle_t go_addon, axis_go_handle_t go_axis_env);

void tenGoAddonOnDeinit(axis_go_handle_t go_addon, axis_go_handle_t go_axis_env);

void tenGoAddonCreateInstance(axis_go_handle_t go_addon,
                              axis_go_handle_t go_axis_env, const char *name,
                              void *context);

void tenGoAddonDestroyInstance(axis_go_handle_t go_instance);

bool axis_go_addon_check_integrity(axis_go_addon_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) != axis_GO_ADDON_SIGNATURE) {
    return false;
  }

  return true;
}

axis_go_handle_t axis_go_addon_go_handle(axis_go_addon_t *self) {
  axis_ASSERT(self && axis_go_addon_check_integrity(self), "Should not happen.");

  return self->bridge.go_instance;
}

static void axis_go_addon_destroy(axis_go_addon_t *self) {
  axis_ASSERT(self && axis_go_addon_check_integrity(self), "Should not happen.");

  axis_string_deinit(&self->addon_name);
  axis_FREE(self);
}

void axis_go_addon_unregister(uintptr_t bridge_addr) {
  axis_ASSERT(bridge_addr, "Invalid argument.");

  axis_go_addon_t *addon_bridge = (axis_go_addon_t *)bridge_addr;
  axis_ASSERT(addon_bridge && axis_go_addon_check_integrity(addon_bridge),
             "Invalid argument.");

  switch (addon_bridge->type) {
    case axis_ADDON_TYPE_EXTENSION:
      axis_addon_unregister_extension(
          axis_string_get_raw_str(&addon_bridge->addon_name));
      break;

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  // The C part is disappear, so decrement the reference count to reflect this
  // fact.
  axis_go_bridge_destroy_c_part(&addon_bridge->bridge);

  // The addon is not used by GO world any more, just destroy it.
  axis_go_bridge_destroy_go_part(&addon_bridge->bridge);
}

static void axis_go_addon_on_init_helper(axis_addon_t *addon,
                                        axis_env_t *axis_env) {
  axis_ASSERT(addon && axis_addon_check_integrity(addon), "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Invalid argument.");

  axis_go_addon_t *addon_bridge =
      (axis_go_addon_t *)addon->binding_handle.me_in_target_lang;
  axis_ASSERT(addon_bridge && axis_go_addon_check_integrity(addon_bridge),
             "Invalid argument.");

  axis_go_axis_env_t *axis_env_bridge = axis_go_axis_env_wrap(axis_env);

  tenGoAddonOnInit(axis_go_addon_go_handle(addon_bridge),
                   axis_go_axis_env_go_handle(axis_env_bridge));
}

static void axis_go_addon_on_deinit_helper(axis_addon_t *addon,
                                          axis_env_t *axis_env) {
  axis_ASSERT(addon && axis_addon_check_integrity(addon) && axis_env,
             "Invalid argument.");

  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  axis_ASSERT(axis_env_check_integrity(axis_env, false), "Invalid argument.");

  axis_go_addon_t *addon_bridge =
      (axis_go_addon_t *)addon->binding_handle.me_in_target_lang;
  axis_ASSERT(addon_bridge && axis_go_addon_check_integrity(addon_bridge),
             "Invalid argument.");

  axis_go_axis_env_t *axis_env_bridge = axis_go_axis_env_wrap(axis_env);
  tenGoAddonOnDeinit(axis_go_addon_go_handle(addon_bridge),
                     axis_go_axis_env_go_handle(axis_env_bridge));
}

static void axis_go_addon_create_extension_async_helper(axis_addon_t *addon,
                                                       axis_env_t *axis_env,
                                                       const char *name,
                                                       void *context) {
  axis_ASSERT(addon && axis_addon_check_integrity(addon) && name && axis_env,
             "Should not happen.");
  axis_ASSERT(axis_env->attach_to == axis_ENV_ATTACH_TO_ADDON,
             "Should not happen.");

  axis_go_addon_t *addon_bridge =
      (axis_go_addon_t *)addon->binding_handle.me_in_target_lang;
  axis_ASSERT(addon_bridge && axis_go_addon_check_integrity(addon_bridge),
             "Should not happen.");

  axis_go_axis_env_t *axis_env_bridge = axis_go_axis_env_wrap(axis_env);
  axis_ASSERT(axis_env_bridge && axis_go_axis_env_check_integrity(axis_env_bridge),
             "Invalid argument.");

  tenGoAddonCreateInstance(axis_go_addon_go_handle(addon_bridge),
                           axis_go_axis_env_go_handle(axis_env_bridge), name,
                           context);
}

static void axis_go_addon_create_extension_group_async_helper(axis_addon_t *addon,
                                                             axis_env_t *axis_env,
                                                             const char *name,
                                                             void *context) {
  axis_ASSERT(addon && axis_addon_check_integrity(addon) && name && axis_env,
             "Should not happen.");
  axis_ASSERT(axis_env->attach_to == axis_ENV_ATTACH_TO_ADDON,
             "Should not happen.");

  axis_go_addon_t *addon_bridge =
      (axis_go_addon_t *)addon->binding_handle.me_in_target_lang;
  axis_ASSERT(addon_bridge && axis_go_addon_check_integrity(addon_bridge),
             "Should not happen.");

  axis_go_axis_env_t *axis_env_bridge = axis_go_axis_env_wrap(axis_env);
  axis_ASSERT(axis_env_bridge && axis_go_axis_env_check_integrity(axis_env_bridge),
             "Invalid argument.");

  tenGoAddonCreateInstance(axis_go_addon_go_handle(addon_bridge),
                           axis_go_axis_env_go_handle(axis_env_bridge), name,
                           context);
}

static void axis_go_addon_destroy_instance_helper(axis_addon_t *addon,
                                                 axis_env_t *axis_env,
                                                 void *instance,
                                                 void *context) {
  axis_ASSERT(addon && axis_addon_check_integrity(addon) && instance && axis_env,
             "Should not happen.");
  axis_ASSERT(axis_env->attach_to == axis_ENV_ATTACH_TO_ADDON,
             "Should not happen.");

  axis_go_addon_t *addon_bridge =
      (axis_go_addon_t *)addon->binding_handle.me_in_target_lang;
  axis_ASSERT(addon_bridge && axis_go_addon_check_integrity(addon_bridge),
             "Should not happen.");

  axis_go_handle_t instance_handle = -1;
  switch (addon_bridge->type) {
    case axis_ADDON_TYPE_EXTENSION: {
      axis_extension_t *extension = (axis_extension_t *)instance;
      axis_go_extension_t *extension_bridge =
          axis_binding_handle_get_me_in_target_lang(
              (axis_binding_handle_t *)extension);
      axis_ASSERT(extension_bridge &&
                     axis_go_extension_check_integrity(extension_bridge),
                 "Invalid argument.");

      instance_handle = axis_go_extension_go_handle(extension_bridge);
      break;
    }

    default:
      axis_ASSERT(0, "Not support.");
      break;
  }

  if (instance_handle > 0) {
    tenGoAddonDestroyInstance(instance_handle);
  }

  axis_env_on_destroy_instance_done(axis_env, context, NULL);
}

static axis_go_addon_t *axis_go_addon_register(
    const void *addon_name, int addon_name_len, const void *base_dir,
    int base_dir_len, uintptr_t go_addon, axis_ADDON_TYPE addon_type,
    void *register_ctx) {
  axis_ASSERT(addon_name && addon_name_len > 0, "Invalid argument.");

  axis_go_addon_t *addon_bridge =
      (axis_go_addon_t *)axis_MALLOC(sizeof(axis_go_addon_t));
  axis_ASSERT(addon_bridge, "Failed to allocate memory.");

  axis_signature_set(&addon_bridge->signature, axis_GO_ADDON_SIGNATURE);

  addon_bridge->bridge.go_instance = go_addon;

  addon_bridge->bridge.sp_ref_by_go =
      axis_shared_ptr_create(addon_bridge, axis_go_addon_destroy);
  addon_bridge->bridge.sp_ref_by_c =
      axis_shared_ptr_clone(addon_bridge->bridge.sp_ref_by_go);

  addon_bridge->type = addon_type;
  axis_string_init_from_c_str(&addon_bridge->addon_name, addon_name,
                             addon_name_len);

  switch (addon_type) {
    case axis_ADDON_TYPE_EXTENSION:
      axis_addon_init(&addon_bridge->c_addon, axis_go_addon_on_init_helper,
                     axis_go_addon_on_deinit_helper,
                     axis_go_addon_create_extension_async_helper,
                     axis_go_addon_destroy_instance_helper, NULL);
      break;

    case axis_ADDON_TYPE_EXTENSION_GROUP:
      axis_addon_init(&addon_bridge->c_addon, axis_go_addon_on_init_helper,
                     axis_go_addon_on_deinit_helper,
                     axis_go_addon_create_extension_group_async_helper,
                     axis_go_addon_destroy_instance_helper, NULL);
      break;

    default:
      axis_ASSERT(0, "Not support.");
      break;
  }

  axis_binding_handle_set_me_in_target_lang(
      (axis_binding_handle_t *)&addon_bridge->c_addon, addon_bridge);

  axis_string_t base_dir_str;
  axis_string_init_from_c_str(&base_dir_str, base_dir, base_dir_len);

  switch (addon_type) {
    case axis_ADDON_TYPE_EXTENSION:
      axis_addon_register_extension(
          axis_string_get_raw_str(&addon_bridge->addon_name),
          axis_string_get_raw_str(&base_dir_str), &addon_bridge->c_addon,
          register_ctx);
      break;

    default:
      axis_ASSERT(0, "Not support.");
      break;
  }

  axis_string_deinit(&base_dir_str);

  return addon_bridge;
}

axis_go_error_t axis_go_addon_register_extension(
    const void *addon_name, int addon_name_len, const void *base_dir,
    int base_dir_len, uintptr_t go_addon, uintptr_t *register_ctx,
    uintptr_t *bridge_addr) {
  axis_ASSERT(addon_name && addon_name_len > 0 && go_addon && bridge_addr,
             "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_addon_t *addon_bridge =
      axis_go_addon_register(addon_name, addon_name_len, base_dir, base_dir_len,
                            go_addon, axis_ADDON_TYPE_EXTENSION, register_ctx);

  *bridge_addr = (uintptr_t)addon_bridge;

  return cgo_error;
}
