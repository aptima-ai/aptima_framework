//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/addon/protocol/protocol.h"

#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/addon/addon_host.h"
#include "include_internal/axis_runtime/addon/common/store.h"
#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/protocol/protocol.h"
#include "include_internal/axis_runtime/axis_env/log.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/lib/uri.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_get.h"
#include "axis_utils/value/value_object.h"

static axis_addon_store_t g_protocol_store = {
    false,
    NULL,
    axis_LIST_INIT_VAL,
};

axis_addon_store_t *axis_protocol_get_global_store(void) {
  axis_addon_store_init(&g_protocol_store);
  return &g_protocol_store;
}

axis_addon_t *axis_addon_unregister_protocol(const char *name) {
  axis_ASSERT(name, "Should not happen.");

  return axis_addon_unregister(axis_protocol_get_global_store(), name);
}

axis_addon_host_t *axis_addon_register_protocol(const char *name,
                                              const char *base_dir,
                                              axis_addon_t *addon) {
  return axis_addon_register(axis_ADDON_TYPE_PROTOCOL, name, base_dir, addon,
                            NULL);
}

static bool axis_addon_protocol_match_protocol(axis_addon_host_t *self,
                                              const char *protocol) {
  axis_ASSERT(self && self->type == axis_ADDON_TYPE_PROTOCOL && protocol,
             "Should not happen.");

  axis_value_t *manifest = &self->manifest;
  axis_ASSERT(manifest, "Invalid argument.");
  axis_ASSERT(axis_value_check_integrity(manifest), "Invalid use of manifest %p.",
             manifest);

  bool found = false;
  axis_list_t *addon_protocols =
      axis_value_object_peek_array(manifest, axis_STR_PROTOCOL);
  axis_ASSERT(addon_protocols, "Should not happen.");

  axis_list_foreach (addon_protocols, iter) {
    axis_value_t *addon_protocol_value = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(
        addon_protocol_value && axis_value_check_integrity(addon_protocol_value),
        "Should not happen.");

    const char *addon_protocol =
        axis_value_peek_raw_str(addon_protocol_value, NULL);
    if (!strcmp(addon_protocol, protocol)) {
      found = true;
      break;
    }
  }

  return found;
}

axis_addon_host_t *axis_addon_protocol_find(const char *protocol) {
  axis_ASSERT(protocol, "Should not happen.");

  axis_addon_host_t *result = NULL;

  axis_addon_store_t *store = axis_protocol_get_global_store();
  axis_ASSERT(store, "Should not happen.");

  axis_mutex_lock(store->lock);

  axis_list_foreach (&store->store, iter) {
    axis_addon_host_t *addon = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(addon && addon->type == axis_ADDON_TYPE_PROTOCOL,
               "Should not happen.");

    if (!axis_addon_protocol_match_protocol(addon, protocol)) {
      continue;
    }

    result = addon;
    break;
  }

  axis_mutex_unlock(store->lock);

  return result;
}

static axis_addon_create_protocol_ctx_t *axis_addon_create_protocol_ctx_create(
    const char *uri, axis_PROTOCOL_ROLE role,
    axis_env_addon_on_create_protocol_async_cb_t cb, void *user_data) {
  axis_ASSERT(role > axis_PROTOCOL_ROLE_INVALID, "Should not happen.");

  axis_addon_create_protocol_ctx_t *ctx =
      (axis_addon_create_protocol_ctx_t *)axis_MALLOC(
          sizeof(axis_addon_create_protocol_ctx_t));
  axis_ASSERT(ctx, "Failed to allocate memory.");

  if (!uri || strlen(uri) == 0) {
    axis_string_init(&ctx->uri);
  } else {
    axis_string_init_formatted(&ctx->uri, "%s", uri);
  }

  ctx->role = role;
  ctx->cb = cb;
  ctx->user_data = user_data;

  return ctx;
}

static void axis_addon_create_protocol_ctx_destroy(
    axis_addon_create_protocol_ctx_t *ctx) {
  axis_ASSERT(ctx, "Should not happen.");

  axis_string_deinit(&ctx->uri);

  axis_FREE(ctx);
}

static void proxy_on_addon_protocol_created(axis_env_t *axis_env, void *instance,
                                            void *cb_data) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_addon_create_protocol_ctx_t *ctx =
      (axis_addon_create_protocol_ctx_t *)cb_data;
  axis_ASSERT(ctx, "Should not happen.");

  axis_protocol_t *protocol = instance;
  if (protocol) {
    axis_protocol_determine_default_property_value(protocol);

    if (!axis_string_is_empty(&ctx->uri)) {
      axis_string_set_formatted(&protocol->uri, "%s",
                               axis_string_get_raw_str(&ctx->uri));
    }

    protocol->role = ctx->role;
  }

  if (ctx->cb) {
    ctx->cb(axis_env, protocol, ctx->user_data);
  }

  axis_addon_create_protocol_ctx_destroy(ctx);
}

bool axis_addon_create_protocol_with_uri(
    axis_env_t *axis_env, const char *uri, axis_PROTOCOL_ROLE role,
    axis_env_addon_on_create_protocol_async_cb_t cb, void *user_data,
    axis_error_t *err) {
  axis_ASSERT(uri && role > axis_PROTOCOL_ROLE_INVALID, "Should not happen.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_ENV_ATTACH_TO attach_to = axis_env_get_attach_to(axis_env);
  if (attach_to != axis_ENV_ATTACH_TO_APP &&
      attach_to != axis_ENV_ATTACH_TO_ENGINE) {
    axis_ENV_LOG_ERROR_INTERNAL(axis_env, "Invalid axis_env attach_to: %d",
                               attach_to);
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT, "Invalid axis_env.");
    }
    return false;
  }

  axis_string_t *protocol_str = axis_uri_get_protocol(uri);
  axis_addon_host_t *addon_host =
      axis_addon_protocol_find(axis_string_get_raw_str(protocol_str));
  if (!addon_host) {
    axis_ENV_LOG_ERROR_INTERNAL(
        axis_env,
        "Failed to handle protocol '%s' because no addon installed for it",
        uri);
    axis_string_destroy(protocol_str);

    if (err) {
      axis_error_set(err, axis_ERRNO_GENERIC,
                    "No addon installed for the protocol.");
    }
    return false;
  }

  axis_ENV_LOG_INFO_INTERNAL(axis_env, "Loading protocol addon: %s",
                            axis_string_get_raw_str(&addon_host->name));

  axis_string_destroy(protocol_str);

  axis_addon_create_protocol_ctx_t *ctx =
      axis_addon_create_protocol_ctx_create(uri, role, cb, user_data);
  axis_ASSERT(ctx, "Failed to allocate memory.");

  bool rc =
      axis_addon_create_instance_async(axis_env, axis_ADDON_TYPE_PROTOCOL,
                                      axis_string_get_raw_str(&addon_host->name),
                                      axis_string_get_raw_str(&addon_host->name),
                                      proxy_on_addon_protocol_created, ctx);

  if (!rc) {
    axis_ENV_LOG_ERROR_INTERNAL(axis_env, "Failed to create protocol for %s",
                               uri);
    axis_addon_create_protocol_ctx_destroy(ctx);

    if (err) {
      axis_error_set(err, axis_ERRNO_GENERIC,
                    "Failed to create protocol for uri: %s.", uri);
    }
    return false;
  }

  return true;
}

bool axis_addon_create_protocol(axis_env_t *axis_env, const char *addon_name,
                               const char *instance_name,
                               axis_PROTOCOL_ROLE role,
                               axis_env_addon_on_create_protocol_async_cb_t cb,
                               void *user_data, axis_error_t *err) {
  axis_ASSERT(addon_name && instance_name, "Should not happen.");
  axis_ASSERT(role > axis_PROTOCOL_ROLE_INVALID, "Should not happen.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_ENV_ATTACH_TO attach_to = axis_env_get_attach_to(axis_env);
  if (attach_to != axis_ENV_ATTACH_TO_APP &&
      attach_to != axis_ENV_ATTACH_TO_ENGINE) {
    axis_LOGE("Invalid axis_env attach_to: %d", attach_to);
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT, "Invalid axis_env.");
    }
    return false;
  }

  axis_LOGD("Loading protocol addon: %s", addon_name);

  axis_addon_create_protocol_ctx_t *ctx =
      axis_addon_create_protocol_ctx_create(NULL, role, cb, user_data);
  axis_ASSERT(ctx, "Failed to allocate memory.");

  bool rc = axis_addon_create_instance_async(
      axis_env, axis_ADDON_TYPE_PROTOCOL, addon_name, instance_name,
      proxy_on_addon_protocol_created, ctx);

  if (!rc) {
    axis_LOGE("Failed to create protocol for %s", addon_name);
    axis_addon_create_protocol_ctx_destroy(ctx);

    if (err) {
      axis_error_set(err, axis_ERRNO_GENERIC,
                    "Failed to create protocol for addon: %s.", addon_name);
    }
    return false;
  }

  return true;
}

void axis_addon_unregister_all_protocol(void) {
  axis_addon_store_del_all(axis_protocol_get_global_store());
}
