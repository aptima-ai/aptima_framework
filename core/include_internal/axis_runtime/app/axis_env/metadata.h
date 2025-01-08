//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "axis_utils/lib/error.h"
#include "axis_utils/lib/string.h"

typedef struct axis_env_t axis_env_t;
typedef struct axis_value_t axis_value_t;
typedef struct axis_app_t axis_app_t;

typedef void (*axis_app_set_property_async_cb_t)(axis_app_t *self,
                                                axis_error_t *err,
                                                void *cb_data);

typedef struct axis_app_set_property_context_t {
  axis_string_t name;
  axis_value_t *value;
  axis_app_set_property_async_cb_t cb;
  void *cb_data;
  axis_error_t err;
} axis_app_set_property_context_t;

typedef void (*axis_app_peek_property_async_cb_t)(axis_app_t *self,
                                                 axis_value_t *value,
                                                 void *cb_data);

typedef struct axis_app_peek_property_context_t {
  axis_string_t name;
  axis_app_peek_property_async_cb_t cb;
  void *cb_data;
  axis_value_t *res;
} axis_app_peek_property_context_t;

typedef void (*axis_app_peek_manifest_async_cb_t)(axis_app_t *self,
                                                 axis_value_t *value,
                                                 void *cb_data);

typedef struct axis_app_peek_manifest_context_t {
  axis_string_t name;
  axis_app_peek_manifest_async_cb_t cb;
  void *cb_data;
  axis_value_t *res;
} axis_app_peek_manifest_context_t;

axis_RUNTIME_PRIVATE_API bool axis_app_set_property(axis_app_t *self,
                                                  const char *name,
                                                  axis_value_t *value,
                                                  axis_error_t *err);

axis_RUNTIME_PRIVATE_API void axis_app_set_property_async(
    axis_app_t *self, const char *name, axis_value_t *value,
    axis_app_set_property_async_cb_t cb, void *cb_data);

axis_RUNTIME_PRIVATE_API axis_value_t *axis_app_peek_property(axis_app_t *app,
                                                           const char *name);

axis_RUNTIME_PRIVATE_API void axis_app_peek_property_async(
    axis_app_t *self, const char *name, axis_app_peek_property_async_cb_t cb,
    void *cb_data);

axis_RUNTIME_PRIVATE_API axis_value_t *axis_app_peek_manifest(axis_app_t *self,
                                                           const char *name);

axis_RUNTIME_PRIVATE_API void axis_app_peek_manifest_async(
    axis_app_t *self, const char *name, axis_app_peek_manifest_async_cb_t cb,
    void *cb_data);
