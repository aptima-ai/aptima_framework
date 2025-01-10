//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/addon/addon_manager.h"

#include "include_internal/ten_runtime/addon/addon.h"
#include "ten_utils/lib/mutex.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/log/log.h"
#include "ten_utils/macro/memory.h"

ten_addon_manager_t *ten_addon_manager_get_instance(void) {
  static ten_addon_manager_t *instance = NULL;
  static ten_mutex_t *init_mutex = NULL;

  if (!init_mutex) {
    init_mutex = ten_mutex_create();
    TEN_ASSERT(init_mutex, "Failed to create initialization mutex.");
  }

  ten_mutex_lock(init_mutex);

  if (!instance) {
    instance = (ten_addon_manager_t *)ten_malloc(sizeof(ten_addon_manager_t));
    TEN_ASSERT(instance, "Failed to allocate memory for ten_addon_manager_t.");

    ten_list_init(&instance->registry);

    instance->mutex = ten_mutex_create();
    TEN_ASSERT(instance->mutex, "Failed to create addon manager mutex.");
  }

  ten_mutex_unlock(init_mutex);

  return instance;
}

static void ten_addon_registration_destroy(void *ptr) {
  ten_addon_registration_t *reg = (ten_addon_registration_t *)ptr;
  if (reg) {
    ten_string_deinit(&reg->addon_name);

    TEN_FREE(reg);
  }
}

bool ten_addon_manager_add_addon(ten_addon_manager_t *self,
                                 const char *addon_type_str,
                                 const char *addon_name,
                                 ten_addon_registration_func_t func) {
  TEN_ASSERT(self && addon_name && func, "Invalid argument.");

  TEN_ADDON_TYPE addon_type = ten_addon_type_from_string(addon_type_str);
  if (addon_type == TEN_ADDON_TYPE_INVALID) {
    TEN_LOGF("Invalid addon type: %s", addon_type_str);
    return false;
  }

  ten_mutex_lock(self->mutex);

  // Check if addon with the same name already exists.
  bool exists = false;

  ten_list_foreach (&self->registry, iter) {
    ten_addon_registration_t *reg =
        (ten_addon_registration_t *)ten_ptr_listnode_get(iter.node);
    if (reg) {
      // Compare both addon type and addon name.
      if (reg->addon_type == addon_type &&
          ten_string_is_equal_c_str(&reg->addon_name, addon_name)) {
        exists = true;
        break;
      }
    }
  }

  if (!exists) {
    // Create a new ten_addon_registration_t.
    ten_addon_registration_t *reg = (ten_addon_registration_t *)TEN_MALLOC(
        sizeof(ten_addon_registration_t));
    TEN_ASSERT(reg, "Failed to allocate memory for ten_addon_registration_t.");

    reg->addon_type = addon_type;
    ten_string_init_from_c_str(&reg->addon_name, addon_name,
                               strlen(addon_name));
    reg->func = func;

    // Add to the registry.
    ten_list_push_ptr_back(&self->registry, reg,
                           ten_addon_registration_destroy);
  } else {
    // Handle the case where the addon is already added.
    // For now, log a warning.
    TEN_LOGW("Addon '%s:%s' is already registered.", addon_type_str,
             addon_name);
  }

  ten_mutex_unlock(self->mutex);

  return true;
}

void ten_addon_manager_register_all_addons(ten_addon_manager_t *self,
                                           void *register_ctx) {
  TEN_ASSERT(self, "Invalid argument.");

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
  ten_mutex_lock(self->mutex);

  ten_list_iterator_t iter = ten_list_begin(&self->registry);
  while (!ten_list_iterator_is_end(iter)) {
    ten_listnode_t *node = iter.node;
    ten_addon_registration_t *reg =
        (ten_addon_registration_t *)ten_ptr_listnode_get(node);

    if (reg && reg->func) {
      reg->func(register_ctx);
    }

    iter = ten_list_iterator_next(iter);
  }

  // Clear the registry after loading.
  ten_list_clear(&self->registry);

  ten_mutex_unlock(self->mutex);
}

bool ten_addon_manager_register_specific_addon(ten_addon_manager_t *self,
                                               TEN_ADDON_TYPE addon_type,
                                               const char *addon_name,
                                               void *register_ctx) {
  TEN_ASSERT(self && addon_name, "Invalid argument.");

  bool success = false;

  ten_mutex_lock(self->mutex);

  ten_listnode_t *found_node = NULL;
  ten_addon_registration_t *found_reg = NULL;

  // Iterate through the registry to find the specific addon.
  ten_list_foreach (&self->registry, iter) {
    ten_addon_registration_t *reg =
        (ten_addon_registration_t *)ten_ptr_listnode_get(iter.node);
    if (reg && reg->addon_type == addon_type &&
        ten_string_is_equal_c_str(&reg->addon_name, addon_name)) {
      found_node = iter.node;
      found_reg = reg;
      break;
    }
  }

  if (found_node) {
    // Remove the addon from the registry.
    ten_list_detach_node(&self->registry, found_node);
  }

  // Since the `register` function of the addon (i.e., the
  // `ten_addon_registration_func_t` function) is highly likely to call the API
  // of the addon manager. To avoid causing a deadlock, the addon manager's
  // mutex needs to be released first before calling the addon's `register`
  // function.
  ten_mutex_unlock(self->mutex);

  if (found_reg) {
    TEN_ASSERT(found_node, "Should not happen.");

    // Register the specific addon.
    found_reg->func(register_ctx);
    success = true;

    // Prevent memory leak.
    ten_listnode_destroy(found_node);
  } else {
    TEN_ASSERT(!found_node, "Should not happen.");

    TEN_LOGW("Addon '%s:%s' not found in registry.",
             ten_addon_type_to_string(addon_type), addon_name);
  }

  return success;
}

ten_addon_register_ctx_t *ten_addon_register_ctx_create(void) {
  ten_addon_register_ctx_t *self = TEN_MALLOC(sizeof(ten_addon_register_ctx_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  return self;
}

void ten_addon_register_ctx_destroy(ten_addon_register_ctx_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  TEN_FREE(self);
}
