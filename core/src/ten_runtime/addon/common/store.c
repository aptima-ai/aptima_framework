//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/addon/common/store.h"

#include <stdlib.h>
#include <string.h>

#include "include_internal/ten_runtime/addon/addon.h"
#include "ten_utils/container/list.h"
#include "ten_utils/lib/atomic.h"
#include "ten_utils/lib/mutex.h"
#include "ten_utils/lib/ref.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"

void ten_addon_store_init(ten_addon_store_t *store) {
  TEN_ASSERT(store, "Can not init empty addon store.");

  // The addon store should be initted only once.
  if (ten_atomic_test_set(&store->valid, 1) == 1) {
    return;
  }

  store->lock = ten_mutex_create();
  ten_list_init(&store->store);
}

static void ten_addon_remove_from_store(ten_addon_host_t *addon) {
  TEN_ASSERT(addon, "Invalid argument.");

  ten_ref_dec_ref(&addon->ref);
}

static ten_addon_host_t *ten_addon_store_find_internal(ten_addon_store_t *store,
                                                       const char *name) {
  TEN_ASSERT(store, "Invalid argument.");
  TEN_ASSERT(name, "Invalid argument.");

  ten_addon_host_t *result = NULL;

  ten_list_foreach (&store->store, iter) {
    ten_addon_host_t *addon = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(addon, "Should not happen.");

    if (ten_string_is_equal_c_str(&addon->name, name)) {
      result = addon;
      break;
    }
  }

  return result;
}

bool ten_addon_store_add(ten_addon_store_t *store, ten_addon_host_t *addon) {
  TEN_ASSERT(store, "Invalid argument.");
  TEN_ASSERT(addon, "Invalid argument.");

  ten_mutex_lock(store->lock);

  // Check if there is an addon with the same name in the addon store. If there
  // is, it is considered an error.
  if (ten_addon_store_find_internal(store,
                                    ten_string_get_raw_str(&addon->name))) {
    return false;
  }

  ten_list_push_ptr_back(
      &store->store, addon,
      (ten_ptr_listnode_destroy_func_t)ten_addon_remove_from_store);

  ten_mutex_unlock(store->lock);

  return true;
}

ten_addon_t *ten_addon_store_del(ten_addon_store_t *store, const char *name) {
  TEN_ASSERT(store, "Invalid argument.");
  TEN_ASSERT(name, "Invalid argument.");

  ten_addon_t *addon = NULL;

  ten_mutex_lock(store->lock);

  ten_list_foreach (&store->store, iter) {
    ten_addon_host_t *addon_host = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(addon_host, "Should not happen.");

    if (ten_string_is_equal_c_str(&addon_host->name, name)) {
      addon = addon_host->addon;
      ten_list_remove_node(&store->store, iter.node);
      break;
    }
  }

  ten_mutex_unlock(store->lock);

  return addon;
}

void ten_addon_store_del_all(ten_addon_store_t *store) {
  TEN_ASSERT(store, "Invalid argument.");

  ten_mutex_lock(store->lock);

  // Clear the store's list, which will call the destroy function for each node,
  // properly decreasing the refcount of each addon.
  ten_list_clear(&store->store);

  ten_mutex_unlock(store->lock);
}

ten_addon_host_t *ten_addon_store_find(ten_addon_store_t *store,
                                       const char *name) {
  TEN_ASSERT(store, "Invalid argument.");
  TEN_ASSERT(name, "Invalid argument.");

  ten_addon_host_t *result = NULL;

  ten_mutex_lock(store->lock);
  result = ten_addon_store_find_internal(store, name);
  ten_mutex_unlock(store->lock);

  return result;
}
