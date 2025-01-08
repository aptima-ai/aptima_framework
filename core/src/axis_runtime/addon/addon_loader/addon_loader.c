//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/addon/addon_loader/addon_loader.h"

#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/addon/addon_host.h"
#include "include_internal/axis_runtime/addon/common/store.h"
#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/addon/addon.h"
#include "axis_runtime/app/app.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/lib/uri.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_get.h"
#include "axis_utils/value/value_object.h"

static axis_addon_store_t g_addon_loader_store = {
    false,
    NULL,
    axis_LIST_INIT_VAL,
};

axis_addon_store_t *axis_addon_loader_get_global_store(void) {
  axis_addon_store_init(&g_addon_loader_store);
  return &g_addon_loader_store;
}

axis_addon_t *axis_addon_unregister_addon_loader(const char *name) {
  axis_ASSERT(name, "Should not happen.");

  return axis_addon_unregister(axis_addon_loader_get_global_store(), name);
}

axis_addon_host_t *axis_addon_register_addon_loader(const char *name,
                                                  const char *base_dir,
                                                  axis_addon_t *addon,
                                                  void *register_ctx) {
  return axis_addon_register(axis_ADDON_TYPE_ADDON_LOADER, name, base_dir, addon,
                            register_ctx);
}

static bool axis_addon_addon_loader_match_addon_loader(
    axis_addon_host_t *self, const char *addon_loader) {
  axis_ASSERT(self && self->type == axis_ADDON_TYPE_PROTOCOL && addon_loader,
             "Should not happen.");

  axis_value_t *manifest = &self->manifest;
  axis_ASSERT(manifest, "Invalid argument.");
  axis_ASSERT(axis_value_check_integrity(manifest), "Invalid use of manifest %p.",
             manifest);

  bool found = false;
  axis_list_t *addon_addon_loaders =
      axis_value_object_peek_array(manifest, axis_STR_PROTOCOL);
  axis_ASSERT(addon_addon_loaders, "Should not happen.");

  axis_list_foreach (addon_addon_loaders, iter) {
    axis_value_t *addon_addon_loader_value = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(addon_addon_loader_value &&
                   axis_value_check_integrity(addon_addon_loader_value),
               "Should not happen.");

    const char *addon_addon_loader =
        axis_value_peek_raw_str(addon_addon_loader_value, NULL);
    if (!strcmp(addon_addon_loader, addon_loader)) {
      found = true;
      break;
    }
  }

  return found;
}

axis_addon_host_t *axis_addon_addon_loader_find(const char *addon_loader) {
  axis_ASSERT(addon_loader, "Should not happen.");

  axis_addon_host_t *result = NULL;

  axis_addon_store_t *store = axis_addon_loader_get_global_store();
  axis_ASSERT(store, "Should not happen.");

  axis_mutex_lock(store->lock);

  axis_list_foreach (&store->store, iter) {
    axis_addon_host_t *addon_host = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(addon_host && addon_host->type == axis_ADDON_TYPE_PROTOCOL,
               "Should not happen.");

    if (!axis_addon_addon_loader_match_addon_loader(addon_host, addon_loader)) {
      continue;
    }

    result = addon_host;
    break;
  }

  axis_mutex_unlock(store->lock);

  return result;
}

void axis_addon_unregister_all_addon_loader(void) {
  axis_addon_store_del_all(axis_addon_loader_get_global_store());
}

// This function is called in the app thread, so `axis_env` is the app's
// `axis_env`.
bool axis_addon_create_addon_loader(axis_env_t *axis_env, const char *addon_name,
                                   const char *instance_name,
                                   axis_env_addon_create_instance_done_cb_t cb,
                                   void *cb_data, axis_error_t *err) {
  axis_ASSERT(addon_name && instance_name, "Should not happen.");

  axis_ASSERT(axis_env, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(axis_env, true),
             "Invalid use of axis_env %p.", axis_env);

  axis_ASSERT(axis_env->attach_to == axis_ENV_ATTACH_TO_APP, "Should not happen.");

  axis_app_t *app = axis_env_get_attached_app(axis_env);
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Should not happen.");

  // This function is called in the app thread, so `axis_env` is the app's
  // `axis_env`.
  if (axis_app_thread_call_by_me(app)) {
    return axis_addon_create_instance_async(axis_env, axis_ADDON_TYPE_ADDON_LOADER,
                                           addon_name, instance_name, cb,
                                           cb_data);
  } else {
    axis_ASSERT(0, "Should not happen.");
    return true;
  }
}
