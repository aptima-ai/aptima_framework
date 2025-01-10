//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/addon/addon_loader/addon_loader.h"

#include "include_internal/ten_runtime/addon/addon.h"
#include "include_internal/ten_runtime/addon/addon_host.h"
#include "include_internal/ten_runtime/addon/common/store.h"
#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_runtime/addon/addon.h"
#include "ten_runtime/app/app.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_node.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/lib/uri.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/value/value.h"
#include "ten_utils/value/value_get.h"
#include "ten_utils/value/value_object.h"

static ten_addon_store_t g_addon_loader_store = {
    false,
    NULL,
    TEN_LIST_INIT_VAL,
};

ten_addon_store_t *ten_addon_loader_get_global_store(void) {
  ten_addon_store_init(&g_addon_loader_store);
  return &g_addon_loader_store;
}

ten_addon_t *ten_addon_unregister_addon_loader(const char *name) {
  TEN_ASSERT(name, "Should not happen.");

  return ten_addon_unregister(ten_addon_loader_get_global_store(), name);
}

ten_addon_host_t *ten_addon_register_addon_loader(const char *name,
                                                  const char *base_dir,
                                                  ten_addon_t *addon,
                                                  void *register_ctx) {
  return ten_addon_register(TEN_ADDON_TYPE_ADDON_LOADER, name, base_dir, addon,
                            register_ctx);
}

static bool ten_addon_addon_loader_match_addon_loader(
    ten_addon_host_t *self, const char *addon_loader) {
  TEN_ASSERT(self && self->type == TEN_ADDON_TYPE_PROTOCOL && addon_loader,
             "Should not happen.");

  ten_value_t *manifest = &self->manifest;
  TEN_ASSERT(manifest, "Invalid argument.");
  TEN_ASSERT(ten_value_check_integrity(manifest), "Invalid use of manifest %p.",
             manifest);

  bool found = false;
  ten_list_t *addon_addon_loaders =
      ten_value_object_peek_array(manifest, TEN_STR_PROTOCOL);
  TEN_ASSERT(addon_addon_loaders, "Should not happen.");

  ten_list_foreach (addon_addon_loaders, iter) {
    ten_value_t *addon_addon_loader_value = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(addon_addon_loader_value &&
                   ten_value_check_integrity(addon_addon_loader_value),
               "Should not happen.");

    const char *addon_addon_loader =
        ten_value_peek_raw_str(addon_addon_loader_value, NULL);
    if (!strcmp(addon_addon_loader, addon_loader)) {
      found = true;
      break;
    }
  }

  return found;
}

ten_addon_host_t *ten_addon_addon_loader_find(const char *addon_loader) {
  TEN_ASSERT(addon_loader, "Should not happen.");

  ten_addon_host_t *result = NULL;

  ten_addon_store_t *store = ten_addon_loader_get_global_store();
  TEN_ASSERT(store, "Should not happen.");

  ten_mutex_lock(store->lock);

  ten_list_foreach (&store->store, iter) {
    ten_addon_host_t *addon_host = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(addon_host && addon_host->type == TEN_ADDON_TYPE_PROTOCOL,
               "Should not happen.");

    if (!ten_addon_addon_loader_match_addon_loader(addon_host, addon_loader)) {
      continue;
    }

    result = addon_host;
    break;
  }

  ten_mutex_unlock(store->lock);

  return result;
}

void ten_addon_unregister_all_addon_loader(void) {
  ten_addon_store_del_all(ten_addon_loader_get_global_store());
}

// This function is called in the app thread, so `ten_env` is the app's
// `ten_env`.
bool ten_addon_create_addon_loader(ten_env_t *ten_env, const char *addon_name,
                                   const char *instance_name,
                                   ten_env_addon_create_instance_done_cb_t cb,
                                   void *cb_data, ten_error_t *err) {
  TEN_ASSERT(addon_name && instance_name, "Should not happen.");

  TEN_ASSERT(ten_env, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(ten_env, true),
             "Invalid use of ten_env %p.", ten_env);

  TEN_ASSERT(ten_env->attach_to == TEN_ENV_ATTACH_TO_APP, "Should not happen.");

  ten_app_t *app = ten_env_get_attached_app(ten_env);
  TEN_ASSERT(app && ten_app_check_integrity(app, true), "Should not happen.");

  // This function is called in the app thread, so `ten_env` is the app's
  // `ten_env`.
  if (ten_app_thread_call_by_me(app)) {
    return ten_addon_create_instance_async(ten_env, TEN_ADDON_TYPE_ADDON_LOADER,
                                           addon_name, instance_name, cb,
                                           cb_data);
  } else {
    TEN_ASSERT(0, "Should not happen.");
    return true;
  }
}
