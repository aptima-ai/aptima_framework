//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/axis_env/metadata.h"
#include "axis_runtime/axis_env/internal/metadata.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/event.h"

typedef struct axis_env_t axis_env_t;
typedef struct axis_value_t axis_value_t;
typedef struct axis_extension_t axis_extension_t;
typedef struct axis_extension_group_t axis_extension_group_t;

typedef void (*axis_env_peek_manifest_async_cb_t)(axis_env_t *axis_env,
                                                 axis_value_t *value,
                                                 void *cb_data,
                                                 axis_error_t *err);

typedef struct axis_peek_manifest_sync_context_t {
  axis_value_t *res;
  axis_event_t *completed;
} axis_peek_manifest_sync_context_t;

typedef struct axis_env_peek_property_async_context_t {
  axis_env_t *axis_env;
  axis_env_peek_property_async_cb_t cb;
  void *cb_data;
  axis_value_t *res;

  union {
    axis_extension_t *extension;
    axis_extension_group_t *extension_group;
  } from;
} axis_env_peek_property_async_context_t;

typedef struct axis_env_peek_property_sync_context_t {
  axis_value_t *res;
  axis_event_t *completed;
} axis_env_peek_property_sync_context_t;

typedef struct axis_env_peek_manifest_async_context_t {
  axis_env_t *axis_env;
  axis_env_peek_manifest_async_cb_t cb;
  void *cb_data;
  axis_value_t *res;

  union {
    axis_extension_t *extension;
    axis_extension_group_t *extension_group;
  } from;
} axis_env_peek_manifest_async_context_t;

typedef struct axis_env_peek_manifest_sync_context_t {
  axis_value_t *res;
  axis_event_t *completed;
} axis_env_peek_manifest_sync_context_t;

typedef struct axis_env_set_property_async_context_t {
  axis_env_t *axis_env;
  axis_env_set_property_async_cb_t cb;
  void *cb_data;

  // TODO(Liu): Replace with axis_error_t.
  bool res;

  union {
    axis_extension_t *extension;
    axis_extension_group_t *extension_group;
  } from;
} axis_env_set_property_async_context_t;

typedef struct axis_env_set_property_sync_context_t {
  axis_error_t *err;
  axis_event_t *completed;
} axis_env_set_property_sync_context_t;
