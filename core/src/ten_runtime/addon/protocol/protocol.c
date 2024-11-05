//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/addon/protocol/protocol.h"

#include "include_internal/ten_runtime/addon/addon.h"
#include "include_internal/ten_runtime/addon/common/store.h"
#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/protocol/protocol.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_node.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/lib/uri.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/value/value.h"
#include "ten_utils/value/value_get.h"
#include "ten_utils/value/value_object.h"

static ten_addon_store_t g_protocol_store = {
    false,
    NULL,
    TEN_LIST_INIT_VAL,
};

ten_addon_store_t *ten_protocol_get_store(void) { return &g_protocol_store; }

ten_addon_t *ten_addon_unregister_protocol(const char *name) {
  TEN_ASSERT(name, "Should not happen.");

  return ten_addon_unregister(ten_protocol_get_store(), name);
}

void ten_addon_register_protocol(const char *name, const char *base_dir,
                                 ten_addon_t *addon) {
  ten_addon_store_init(ten_protocol_get_store());

  ten_addon_host_t *addon_host =
      (ten_addon_host_t *)TEN_MALLOC(sizeof(ten_addon_host_t));
  TEN_ASSERT(addon_host, "Failed to allocate memory.");

  addon_host->type = TEN_ADDON_TYPE_PROTOCOL;
  ten_addon_host_init(addon_host);

  ten_addon_register(ten_protocol_get_store(), addon_host, name, base_dir,
                     addon);
}

static bool ten_addon_protocol_match_protocol(ten_addon_host_t *self,
                                              const char *protocol) {
  TEN_ASSERT(self && self->type == TEN_ADDON_TYPE_PROTOCOL && protocol,
             "Should not happen.");

  ten_value_t *manifest = &self->manifest;
  TEN_ASSERT(manifest, "Invalid argument.");
  TEN_ASSERT(ten_value_check_integrity(manifest), "Invalid use of manifest %p.",
             manifest);

  bool found = false;
  ten_list_t *addon_protocols =
      ten_value_object_peek_array(manifest, TEN_STR_PROTOCOL);
  TEN_ASSERT(addon_protocols, "Should not happen.");

  ten_list_foreach (addon_protocols, iter) {
    ten_value_t *addon_protocol_value = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(
        addon_protocol_value && ten_value_check_integrity(addon_protocol_value),
        "Should not happen.");

    const char *addon_protocol = ten_value_peek_raw_str(addon_protocol_value);
    if (!strcmp(addon_protocol, protocol)) {
      found = true;
      break;
    }
  }

  return found;
}

ten_addon_host_t *ten_addon_protocol_find(const char *protocol) {
  TEN_ASSERT(protocol, "Should not happen.");

  ten_addon_host_t *result = NULL;

  ten_addon_store_t *store = ten_protocol_get_store();
  TEN_ASSERT(store, "Should not happen.");

  ten_mutex_lock(store->lock);

  ten_list_foreach (&store->store, iter) {
    ten_addon_host_t *addon = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(addon && addon->type == TEN_ADDON_TYPE_PROTOCOL,
               "Should not happen.");

    if (!ten_addon_protocol_match_protocol(addon, protocol)) {
      continue;
    }

    result = addon;
    break;
  }

  ten_mutex_unlock(store->lock);

  return result;
}

static ten_addon_create_protocol_info_t *ten_addon_create_protocol_info_create(
    const char *uri, TEN_PROTOCOL_ROLE role,
    ten_env_addon_on_create_protocol_async_cb_t cb, void *user_data) {
  TEN_ASSERT(role > TEN_PROTOCOL_ROLE_INVALID, "Should not happen.");

  ten_addon_create_protocol_info_t *info =
      (ten_addon_create_protocol_info_t *)TEN_MALLOC(
          sizeof(ten_addon_create_protocol_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  if (!uri || strlen(uri) == 0) {
    ten_string_init(&info->uri);
  } else {
    ten_string_init_formatted(&info->uri, "%s", uri);
  }

  info->role = role;
  info->cb = cb;
  info->user_data = user_data;

  return info;
}

static void ten_addon_create_protocol_info_destroy(
    ten_addon_create_protocol_info_t *info) {
  TEN_ASSERT(info, "Should not happen.");

  ten_string_deinit(&info->uri);

  TEN_FREE(info);
}

static void proxy_on_addon_protocol_created(ten_env_t *ten_env, void *instance,
                                            void *cb_data) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_addon_create_protocol_info_t *info =
      (ten_addon_create_protocol_info_t *)cb_data;
  TEN_ASSERT(info, "Should not happen.");

  ten_protocol_t *protocol = instance;
  if (protocol) {
    ten_protocol_determine_default_property_value(protocol);

    if (!ten_string_is_empty(&info->uri)) {
      ten_string_set_formatted(&protocol->uri, "%s",
                               ten_string_get_raw_str(&info->uri));
    }

    protocol->role = info->role;
  }

  if (info->cb) {
    info->cb(ten_env, protocol, info->user_data);
  }

  ten_addon_create_protocol_info_destroy(info);
}

bool ten_addon_create_protocol_with_uri(
    ten_env_t *ten_env, const char *uri, TEN_PROTOCOL_ROLE role,
    ten_env_addon_on_create_protocol_async_cb_t cb, void *user_data,
    ten_error_t *err) {
  TEN_ASSERT(uri && role > TEN_PROTOCOL_ROLE_INVALID, "Should not happen.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  TEN_ENV_ATTACH_TO attach_to = ten_env_get_attach_to(ten_env);
  if (attach_to != TEN_ENV_ATTACH_TO_APP &&
      attach_to != TEN_ENV_ATTACH_TO_ENGINE) {
    TEN_LOGE("Invalid ten_env attach_to: %d", attach_to);
    if (err) {
      ten_error_set(err, TEN_ERRNO_INVALID_ARGUMENT, "Invalid ten_env.");
    }
    return false;
  }

  ten_string_t *protocol_str = ten_uri_get_protocol(uri);
  ten_addon_host_t *addon_host =
      ten_addon_protocol_find(ten_string_get_raw_str(protocol_str));
  if (!addon_host) {
    TEN_LOGE("Can not handle protocol '%s' because no addon installed for it",
             uri);
    ten_string_destroy(protocol_str);

    if (err) {
      ten_error_set(err, TEN_ERRNO_GENERIC,
                    "No addon installed for the protocol.");
    }
    return false;
  }

  TEN_LOGD("Loading protocol addon: %s",
           ten_string_get_raw_str(&addon_host->name));

  ten_string_destroy(protocol_str);

  ten_addon_create_protocol_info_t *info =
      ten_addon_create_protocol_info_create(uri, role, cb, user_data);
  TEN_ASSERT(info, "Failed to allocate memory.");

  bool rc = ten_addon_create_instance_async(
      ten_env, ten_string_get_raw_str(&addon_host->name),
      ten_string_get_raw_str(&addon_host->name), TEN_ADDON_TYPE_PROTOCOL,
      proxy_on_addon_protocol_created, info);

  if (!rc) {
    TEN_LOGE("Failed to create protocol for %s", uri);
    ten_addon_create_protocol_info_destroy(info);

    if (err) {
      ten_error_set(err, TEN_ERRNO_GENERIC,
                    "Failed to create protocol for uri: %s.", uri);
    }
    return false;
  }

  return true;
}

bool ten_addon_create_protocol(ten_env_t *ten_env, const char *addon_name,
                               const char *instance_name,
                               TEN_PROTOCOL_ROLE role,
                               ten_env_addon_on_create_protocol_async_cb_t cb,
                               void *user_data, ten_error_t *err) {
  TEN_ASSERT(addon_name && instance_name, "Should not happen.");
  TEN_ASSERT(role > TEN_PROTOCOL_ROLE_INVALID, "Should not happen.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  TEN_ENV_ATTACH_TO attach_to = ten_env_get_attach_to(ten_env);
  if (attach_to != TEN_ENV_ATTACH_TO_APP &&
      attach_to != TEN_ENV_ATTACH_TO_ENGINE) {
    TEN_LOGE("Invalid ten_env attach_to: %d", attach_to);
    if (err) {
      ten_error_set(err, TEN_ERRNO_INVALID_ARGUMENT, "Invalid ten_env.");
    }
    return false;
  }

  TEN_LOGD("Loading protocol addon: %s", addon_name);

  ten_addon_create_protocol_info_t *info =
      ten_addon_create_protocol_info_create(NULL, role, cb, user_data);
  TEN_ASSERT(info, "Failed to allocate memory.");

  bool rc = ten_addon_create_instance_async(
      ten_env, addon_name, instance_name, TEN_ADDON_TYPE_PROTOCOL,
      proxy_on_addon_protocol_created, info);

  if (!rc) {
    TEN_LOGE("Failed to create protocol for %s", addon_name);
    ten_addon_create_protocol_info_destroy(info);

    if (err) {
      ten_error_set(err, TEN_ERRNO_GENERIC,
                    "Failed to create protocol for addon: %s.", addon_name);
    }
    return false;
  }

  return true;
}
