//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/addon/addon_manager.h"

#include "include_internal/axis_runtime/addon/addon.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/memory.h"

axis_addon_manager_t *axis_addon_manager_get_instance(void) {
  static axis_addon_manager_t *instance = NULL;
  static axis_mutex_t *init_mutex = NULL;

  if (!init_mutex) {
    init_mutex = axis_mutex_create();
    axis_ASSERT(init_mutex, "Failed to create initialization mutex.");
  }

  axis_mutex_lock(init_mutex);

  if (!instance) {
    instance = (axis_addon_manager_t *)axis_malloc(sizeof(axis_addon_manager_t));
    axis_ASSERT(instance, "Failed to allocate memory for axis_addon_manager_t.");

    axis_list_init(&instance->registry);

    instance->mutex = axis_mutex_create();
    axis_ASSERT(instance->mutex, "Failed to create addon manager mutex.");
  }

  axis_mutex_unlock(init_mutex);

  return instance;
}

static void axis_addon_registration_destroy(void *ptr) {
  axis_addon_registration_t *reg = (axis_addon_registration_t *)ptr;
  if (reg) {
    axis_string_deinit(&reg->addon_name);

    axis_FREE(reg);
  }
}

bool axis_addon_manager_add_addon(axis_addon_manager_t *self,
                                 const char *addon_type_str,
                                 const char *addon_name,
                                 axis_addon_registration_func_t func) {
  axis_ASSERT(self && addon_name && func, "Invalid argument.");

  axis_ADDON_TYPE addon_type = axis_addon_type_from_string(addon_type_str);
  if (addon_type == axis_ADDON_TYPE_INVALID) {
    axis_LOGF("Invalid addon type: %s", addon_type_str);
    return false;
  }

  axis_mutex_lock(self->mutex);

  // Check if addon with the same name already exists.
  bool exists = false;

  axis_list_foreach (&self->registry, iter) {
    axis_addon_registration_t *reg =
        (axis_addon_registration_t *)axis_ptr_listnode_get(iter.node);
    if (reg) {
      // Compare both addon type and addon name.
      if (reg->addon_type == addon_type &&
          axis_string_is_equal_c_str(&reg->addon_name, addon_name)) {
        exists = true;
        break;
      }
    }
  }

  if (!exists) {
    // Create a new axis_addon_registration_t.
    axis_addon_registration_t *reg = (axis_addon_registration_t *)axis_MALLOC(
        sizeof(axis_addon_registration_t));
    axis_ASSERT(reg, "Failed to allocate memory for axis_addon_registration_t.");

    reg->addon_type = addon_type;
    axis_string_init_from_c_str(&reg->addon_name, addon_name,
                               strlen(addon_name));
    reg->func = func;

    // Add to the registry.
    axis_list_push_ptr_back(&self->registry, reg,
                           axis_addon_registration_destroy);
  } else {
    // Handle the case where the addon is already added.
    // For now, log a warning.
    axis_LOGW("Addon '%s:%s' is already registered.", addon_type_str,
             addon_name);
  }

  axis_mutex_unlock(self->mutex);

  return true;
}

void axis_addon_manager_register_all_addons(axis_addon_manager_t *self,
                                           void *register_ctx) {
  axis_ASSERT(self, "Invalid argument.");

  // Basically, the relationship between an app and a process is one-to-one,
  // meaning a process will only have one app. In this scenario, theoretically,
  // the mutex lock/unlock protection below would not be necessary. However, in
  // special cases, such as under gtest, a single gtest process may execute
  // multiple apps, with some apps starting in parallel. As a result, this
  // function could potentially be called multiple times in parallel. In such
  // cases, the mutex lock/unlock is indeed necessary. In normal circumstances,
  // where the relationship is one-to-one, performing the mutex lock/unlock
  // action causes no harm. Therefore, mutex lock/unlock is uniformly applied
  // here.
  axis_mutex_lock(self->mutex);

  axis_list_iterator_t iter = axis_list_begin(&self->registry);
  while (!axis_list_iterator_is_end(iter)) {
    axis_listnode_t *node = iter.node;
    axis_addon_registration_t *reg =
        (axis_addon_registration_t *)axis_ptr_listnode_get(node);

    if (reg && reg->func) {
      reg->func(register_ctx);
    }

    iter = axis_list_iterator_next(iter);
  }

  // Clear the registry after loading.
  axis_list_clear(&self->registry);

  axis_mutex_unlock(self->mutex);
}

bool axis_addon_manager_register_specific_addon(axis_addon_manager_t *self,
                                               axis_ADDON_TYPE addon_type,
                                               const char *addon_name,
                                               void *register_ctx) {
  axis_ASSERT(self && addon_name, "Invalid argument.");

  bool success = false;

  axis_mutex_lock(self->mutex);

  axis_listnode_t *found_node = NULL;
  axis_addon_registration_t *found_reg = NULL;

  // Iterate through the registry to find the specific addon.
  axis_list_foreach (&self->registry, iter) {
    axis_addon_registration_t *reg =
        (axis_addon_registration_t *)axis_ptr_listnode_get(iter.node);
    if (reg && reg->addon_type == addon_type &&
        axis_string_is_equal_c_str(&reg->addon_name, addon_name)) {
      found_node = iter.node;
      found_reg = reg;
      break;
    }
  }

  if (found_node) {
    // Remove the addon from the registry.
    axis_list_detach_node(&self->registry, found_node);
  }

  // Since the `register` function of the addon (i.e., the
  // `axis_addon_registration_func_t` function) is highly likely to call the API
  // of the addon manager. To avoid causing a deadlock, the addon manager's
  // mutex needs to be released first before calling the addon's `register`
  // function.
  axis_mutex_unlock(self->mutex);

  if (found_reg) {
    axis_ASSERT(found_node, "Should not happen.");

    // Register the specific addon.
    found_reg->func(register_ctx);
    success = true;

    // Prevent memory leak.
    axis_listnode_destroy(found_node);
  } else {
    axis_ASSERT(!found_node, "Should not happen.");

    axis_LOGI("Unable to find '%s:%s' in registry.",
             axis_addon_type_to_string(addon_type), addon_name);
  }

  return success;
}

axis_addon_register_ctx_t *axis_addon_register_ctx_create(void) {
  axis_addon_register_ctx_t *self = axis_MALLOC(sizeof(axis_addon_register_ctx_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  return self;
}

void axis_addon_register_ctx_destroy(axis_addon_register_ctx_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_FREE(self);
}
