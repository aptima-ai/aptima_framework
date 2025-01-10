//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/addon_loader/addon_loader.h"

#include "include_internal/axis_runtime/addon/addon_host.h"
#include "include_internal/axis_runtime/addon/addon_loader/addon_loader.h"
#include "include_internal/axis_runtime/app/on_xxx.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/app/app.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/container/list.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/macro/memory.h"

static axis_list_t g_addon_loaders = axis_LIST_INIT_VAL;

axis_list_t *axis_addon_loader_get_all(void) { return &g_addon_loaders; }

bool axis_addon_loader_check_integrity(axis_addon_loader_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_ADDON_LOADER_SIGNATURE) {
    return false;
  }

  return true;
}

axis_addon_loader_t *axis_addon_loader_create(
    axis_addon_loader_on_init_func_t on_init,
    axis_addon_loader_on_deinit_func_t on_deinit,
    axis_addon_loader_on_load_addon_func_t on_load_addon) {
  axis_addon_loader_t *self =
      (axis_addon_loader_t *)axis_MALLOC(sizeof(axis_addon_loader_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_signature_set(&self->signature, axis_ADDON_LOADER_SIGNATURE);

  self->addon_host = NULL;

  self->on_init = on_init;
  self->on_deinit = on_deinit;
  self->on_load_addon = on_load_addon;

  return self;
}

void axis_addon_loader_destroy(axis_addon_loader_t *self) {
  axis_ASSERT(self && axis_addon_loader_check_integrity(self),
             "Invalid argument.");

  axis_FREE(self);
}

static void axis_addon_loader_init(axis_addon_loader_t *self) {
  axis_ASSERT(self && axis_addon_loader_check_integrity(self),
             "Invalid argument.");

  if (self->on_init) {
    self->on_init(self);
  }
}

static void axis_addon_loader_deinit(axis_addon_loader_t *self) {
  axis_ASSERT(self && axis_addon_loader_check_integrity(self),
             "Invalid argument.");

  if (self->on_deinit) {
    self->on_deinit(self);
  }
}

void axis_addon_loader_load_addon(axis_addon_loader_t *self,
                                 axis_ADDON_TYPE addon_type,
                                 const char *addon_name) {
  axis_ASSERT(self && axis_addon_loader_check_integrity(self),
             "Invalid argument.");

  if (self->on_load_addon) {
    self->on_load_addon(self, addon_type, addon_name);
  }
}

static void create_addon_loader_done(axis_env_t *axis_env,
                                     axis_addon_loader_t *addon_loader,
                                     void *cb_data) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Invalid argument.");
  axis_ASSERT(addon_loader && axis_addon_loader_check_integrity(addon_loader),
             "Invalid argument.");
  axis_ASSERT(axis_env_get_attach_to(axis_env) == axis_ENV_ATTACH_TO_APP,
             "Invalid argument.");

  axis_app_t *app = axis_env_get_attached_target(axis_env);
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Invalid argument.");

  size_t desired_count = (size_t)cb_data;

  // Call `on_init` of the addon_loader to initialize the addon_loader.
  axis_addon_loader_init(addon_loader);

  axis_list_push_ptr_back(&g_addon_loaders, addon_loader, NULL);

  if (axis_list_size(&g_addon_loaders) == desired_count) {
    axis_app_on_all_addon_loaders_created(app);
  }
}

bool axis_addon_loader_addons_create_singleton_instance(axis_env_t *axis_env) {
  bool need_to_wait_all_addon_loaders_created = true;

  axis_addon_store_t *addon_loader_store = axis_addon_loader_get_global_store();
  axis_ASSERT(addon_loader_store, "Should not happen.");

  size_t desired_count = axis_list_size(&addon_loader_store->store);
  if (!desired_count) {
    need_to_wait_all_addon_loaders_created = false;
  }

  axis_list_foreach (&addon_loader_store->store, iter) {
    axis_addon_host_t *loader_addon_host = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(loader_addon_host, "Should not happen.");

    bool res = axis_addon_create_addon_loader(
        axis_env, axis_string_get_raw_str(&loader_addon_host->name),
        axis_string_get_raw_str(&loader_addon_host->name),
        (axis_env_addon_create_instance_done_cb_t)create_addon_loader_done,
        (void *)desired_count, NULL);

    if (!res) {
      axis_LOGE("Failed to create addon_loader instance %s",
               axis_string_get_raw_str(&loader_addon_host->name));
#if defined(_DEBUG)
      axis_ASSERT(0, "Should not happen.");
#endif
    }
  }

  return need_to_wait_all_addon_loaders_created;
}

// Destroy all addon loader instances in the system.
void axis_addon_loader_addons_destroy_singleton_instance(void) {
  axis_list_foreach (&g_addon_loaders, iter) {
    axis_addon_loader_t *addon_loader = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(addon_loader && axis_addon_loader_check_integrity(addon_loader),
               "Should not happen.");

    axis_addon_host_t *addon_host = addon_loader->addon_host;
    axis_ASSERT(addon_host && axis_addon_host_check_integrity(addon_host),
               "Should not happen.");

    axis_addon_loader_deinit(addon_loader);

    axis_addon_t *addon = addon_host->addon;
    axis_ASSERT(addon && axis_addon_check_integrity(addon), "Should not happen.");

    addon->on_destroy_instance(addon, addon_host->axis_env, addon_loader, NULL);
  }

  axis_list_clear(&g_addon_loaders);
}
