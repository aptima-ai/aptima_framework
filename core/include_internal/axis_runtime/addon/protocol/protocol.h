//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/protocol/protocol.h"
#include "axis_runtime/addon/addon.h"

typedef struct axis_addon_host_t axis_addon_host_t;
typedef struct axis_addon_store_t axis_addon_store_t;

#define axis_REGISTER_ADDON_AS_PROTOCOL(PROTOCOL_NAME, ADDON) \
  axis_ADDON_REGISTER(protocol, PROTOCOL_NAME, ADDON)

typedef void (*axis_env_addon_on_create_protocol_async_cb_t)(
    axis_env_t *axis_env, axis_protocol_t *protocol, void *cb_data);

typedef struct axis_addon_create_protocol_ctx_t {
  axis_string_t uri;
  axis_PROTOCOL_ROLE role;
  axis_env_addon_on_create_protocol_async_cb_t cb;
  void *user_data;
} axis_addon_create_protocol_ctx_t;

axis_RUNTIME_PRIVATE_API bool axis_addon_create_protocol_with_uri(
    axis_env_t *axis_env, const char *uri, axis_PROTOCOL_ROLE role,
    axis_env_addon_on_create_protocol_async_cb_t cb, void *user_data,
    axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_addon_create_protocol(
    axis_env_t *axis_env, const char *addon_name, const char *instance_name,
    axis_PROTOCOL_ROLE role, axis_env_addon_on_create_protocol_async_cb_t cb,
    void *user_data, axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_addon_store_t *axis_protocol_get_global_store(void);

axis_RUNTIME_PRIVATE_API axis_addon_host_t *axis_addon_protocol_find(
    const char *protocol);

axis_RUNTIME_API axis_addon_host_t *axis_addon_register_protocol(
    const char *name, const char *base_dir, axis_addon_t *addon);

axis_RUNTIME_API axis_addon_t *axis_addon_unregister_protocol(const char *name);

axis_RUNTIME_PRIVATE_API void axis_addon_unregister_all_protocol(void);
