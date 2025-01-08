//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/addon/common/store.h"

#include <stdlib.h>
#include <string.h>

#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/addon/addon_host.h"
#include "axis_utils/container/list.h"
#include "axis_utils/lib/atomic.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/lib/ref.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"

void axis_addon_store_init(axis_addon_store_t *store) {
  axis_ASSERT(store, "Can not init empty addon store.");

  // The addon store should be initted only once.
  if (axis_atomic_test_set(&store->valid, 1) == 1) {
    return;
  }

  store->lock = axis_mutex_create();
  axis_list_init(&store->store);
}

static void axis_addon_host_remove_from_store(axis_addon_host_t *addon_host) {
  axis_ASSERT(addon_host, "Invalid argument.");

  axis_ref_dec_ref(&addon_host->ref);
}

static axis_addon_host_t *axis_addon_store_find_internal(axis_addon_store_t *store,
                                                       const char *name) {
  axis_ASSERT(store, "Invalid argument.");
  axis_ASSERT(name, "Invalid argument.");

  axis_addon_host_t *result = NULL;

  axis_list_foreach (&store->store, iter) {
    axis_addon_host_t *addon = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(addon, "Should not happen.");

    if (axis_string_is_equal_c_str(&addon->name, name)) {
      result = addon;
      break;
    }
  }

  return result;
}

static void axis_addon_store_add(axis_addon_store_t *store,
                                axis_addon_host_t *addon) {
  axis_ASSERT(store, "Invalid argument.");
  axis_ASSERT(addon, "Invalid argument.");

  axis_list_push_ptr_back(
      &store->store, addon,
      (axis_ptr_listnode_destroy_func_t)axis_addon_host_remove_from_store);
}

axis_addon_t *axis_addon_store_del(axis_addon_store_t *store, const char *name) {
  axis_ASSERT(store, "Invalid argument.");
  axis_ASSERT(name, "Invalid argument.");

  axis_addon_t *addon = NULL;

  axis_mutex_lock(store->lock);

  axis_list_foreach (&store->store, iter) {
    axis_addon_host_t *addon_host = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(addon_host, "Should not happen.");

    if (axis_string_is_equal_c_str(&addon_host->name, name)) {
      addon = addon_host->addon;
      axis_list_remove_node(&store->store, iter.node);
      break;
    }
  }

  axis_mutex_unlock(store->lock);

  return addon;
}

void axis_addon_store_del_all(axis_addon_store_t *store) {
  axis_ASSERT(store, "Invalid argument.");

  axis_mutex_lock(store->lock);

  // Clear the store's list, which will call the destroy function for each node,
  // properly decreasing the refcount of each addon.
  axis_list_clear(&store->store);

  axis_mutex_unlock(store->lock);
}

axis_addon_host_t *axis_addon_store_find(axis_addon_store_t *store,
                                       const char *name) {
  axis_ASSERT(store, "Invalid argument.");
  axis_ASSERT(name, "Invalid argument.");

  axis_addon_host_t *result = NULL;

  axis_mutex_lock(store->lock);
  result = axis_addon_store_find_internal(store, name);
  axis_mutex_unlock(store->lock);

  return result;
}

axis_addon_host_t *axis_addon_store_find_or_create_one_if_not_found(
    axis_addon_store_t *store, axis_ADDON_TYPE addon_type, const char *addon_name,
    bool *newly_created) {
  axis_ASSERT(store, "Invalid argument.");
  axis_ASSERT(addon_name, "Invalid argument.");
  axis_ASSERT(newly_created, "Invalid argument.");

  axis_addon_host_t *result = NULL;
  *newly_created = false;

  axis_mutex_lock(store->lock);

  result = axis_addon_store_find_internal(store, addon_name);

  if (!result) {
    result = axis_addon_host_create(addon_type);
    axis_ASSERT(result, "Should not happen.");

    axis_addon_store_add(store, result);

    *newly_created = true;
  }

  axis_mutex_unlock(store->lock);

  return result;
}
