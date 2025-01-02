//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/addon_loader/addon_loader.h"

#include "include_internal/ten_runtime/addon/addon_host.h"
#include "include_internal/ten_runtime/addon/addon_loader/addon_loader.h"
#include "include_internal/ten_runtime/app/on_xxx.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_runtime/app/app.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/container/list.h"
#include "ten_utils/lib/signature.h"
#include "ten_utils/macro/memory.h"

static ten_list_t g_addon_loaders = TEN_LIST_INIT_VAL;

ten_list_t *ten_addon_loader_get_all(void) { return &g_addon_loaders; }

bool ten_addon_loader_check_integrity(ten_addon_loader_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  if (ten_signature_get(&self->signature) !=
      (ten_signature_t)TEN_ADDON_LOADER_SIGNATURE) {
    return false;
  }

  return true;
}

ten_addon_loader_t *ten_addon_loader_create(
    ten_addon_loader_on_init_func_t on_init,
    ten_addon_loader_on_deinit_func_t on_deinit,
    ten_addon_loader_on_load_addon_func_t on_load_addon) {
  ten_addon_loader_t *self =
      (ten_addon_loader_t *)TEN_MALLOC(sizeof(ten_addon_loader_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  ten_signature_set(&self->signature, TEN_ADDON_LOADER_SIGNATURE);

  self->addon_host = NULL;

  self->on_init = on_init;
  self->on_deinit = on_deinit;
  self->on_load_addon = on_load_addon;

  return self;
}

void ten_addon_loader_destroy(ten_addon_loader_t *self) {
  TEN_ASSERT(self && ten_addon_loader_check_integrity(self),
             "Invalid argument.");

  TEN_FREE(self);
}

void ten_addon_loader_init(ten_addon_loader_t *self) {
  TEN_ASSERT(self && ten_addon_loader_check_integrity(self),
             "Invalid argument.");

  if (self->on_init) {
    self->on_init(self);
  }
}

void ten_addon_loader_deinit(ten_addon_loader_t *self) {
  TEN_ASSERT(self && ten_addon_loader_check_integrity(self),
             "Invalid argument.");

  if (self->on_deinit) {
    self->on_deinit(self);
  }
}

void ten_addon_loader_load_addon(ten_addon_loader_t *self,
                                 TEN_ADDON_TYPE addon_type,
                                 const char *addon_name) {
  TEN_ASSERT(self && ten_addon_loader_check_integrity(self),
             "Invalid argument.");

  if (self->on_load_addon) {
    self->on_load_addon(self, addon_type, addon_name);
  }
}

static void create_addon_loader_done(ten_env_t *ten_env,
                                     ten_addon_loader_t *addon_loader,
                                     void *cb_data) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Invalid argument.");
  TEN_ASSERT(addon_loader && ten_addon_loader_check_integrity(addon_loader),
             "Invalid argument.");
  TEN_ASSERT(ten_env_get_attach_to(ten_env) == TEN_ENV_ATTACH_TO_APP,
             "Invalid argument.");

  ten_app_t *app = ten_env_get_attached_target(ten_env);
  TEN_ASSERT(app && ten_app_check_integrity(app, true), "Invalid argument.");

  size_t desired_count = (size_t)cb_data;

  // Call `on_init` of the addon_loader to initialize the addon_loader.
  addon_loader->on_init(addon_loader);

  ten_list_push_ptr_back(&g_addon_loaders, addon_loader, NULL);

  if (ten_list_size(&g_addon_loaders) == desired_count) {
    ten_app_on_all_addon_loaders_created(app);
  }
}

bool ten_addon_loader_addons_create_singleton_instance(ten_env_t *ten_env) {
  bool need_to_wait_all_addon_loaders_created = true;

  ten_addon_store_t *addon_loader_store = ten_addon_loader_get_global_store();
  TEN_ASSERT(addon_loader_store, "Should not happen.");

  size_t desired_count = ten_list_size(&addon_loader_store->store);
  if (!desired_count) {
    need_to_wait_all_addon_loaders_created = false;
  }

  ten_list_foreach (&addon_loader_store->store, iter) {
    ten_addon_host_t *loader_addon_host = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(loader_addon_host, "Should not happen.");

    bool res = ten_addon_create_addon_loader(
        ten_env, ten_string_get_raw_str(&loader_addon_host->name),
        ten_string_get_raw_str(&loader_addon_host->name),
        (ten_env_addon_create_instance_done_cb_t)create_addon_loader_done,
        (void *)desired_count, NULL);

    if (!res) {
      TEN_LOGE("Failed to create addon_loader instance %s",
               ten_string_get_raw_str(&loader_addon_host->name));
#if defined(_DEBUG)
      TEN_ASSERT(0, "Should not happen.");
#endif
    }
  }

  return need_to_wait_all_addon_loaders_created;
}

// Destroy all addon loader instances in the system.
void ten_addon_loader_addons_destroy_singleton_instance(void) {
  ten_list_foreach (&g_addon_loaders, iter) {
    ten_addon_loader_t *addon_loader = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(addon_loader && ten_addon_loader_check_integrity(addon_loader),
               "Should not happen.");

    ten_addon_host_t *addon_host = addon_loader->addon_host;
    TEN_ASSERT(addon_host && ten_addon_host_check_integrity(addon_host),
               "Should not happen.");

    ten_addon_t *addon = addon_host->addon;
    TEN_ASSERT(addon && ten_addon_check_integrity(addon), "Should not happen.");

    addon->on_destroy_instance(addon, addon_host->ten_env, addon_loader, NULL);
  }

  ten_list_clear(&g_addon_loaders);
}
