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
typedef struct axis_extension_t axis_extension_t;

typedef void (*axis_extension_set_property_async_cb_t)(axis_extension_t *self,
                                                      bool res, void *cb_data,
                                                      axis_error_t *err);

typedef struct axis_extension_set_property_context_t {
  axis_string_t path;
  axis_value_t *value;
  axis_extension_set_property_async_cb_t cb;
  void *cb_data;
  bool res;
} axis_extension_set_property_context_t;

typedef void (*axis_extension_peek_property_async_cb_t)(axis_extension_t *self,
                                                       axis_value_t *value,
                                                       void *cb_data,
                                                       axis_error_t *err);

typedef struct axis_extension_peek_property_context_t {
  axis_string_t path;
  axis_extension_peek_property_async_cb_t cb;
  void *cb_data;
  axis_value_t *res;
} axis_extension_peek_property_context_t;

typedef void (*axis_extension_peek_manifest_async_cb_t)(axis_extension_t *self,
                                                       axis_value_t *value,
                                                       void *cb_data,
                                                       axis_error_t *err);

typedef struct axis_extension_peek_manifest_context_t {
  axis_string_t path;
  axis_extension_peek_manifest_async_cb_t cb;
  void *cb_data;
  axis_value_t *res;
} axis_extension_peek_manifest_context_t;

axis_RUNTIME_PRIVATE_API bool axis_extension_set_property(axis_extension_t *self,
                                                        const char *path,
                                                        axis_value_t *value,
                                                        axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_extension_set_property_async(
    axis_extension_t *self, const char *path, axis_value_t *value,
    axis_extension_set_property_async_cb_t cb, void *cb_data, axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_value_t *axis_extension_peek_property(
    axis_extension_t *extension, const char *path, axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_extension_peek_property_async(
    axis_extension_t *self, const char *path,
    axis_extension_peek_property_async_cb_t cb, void *cb_data, axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_value_t *axis_extension_peek_manifest(
    axis_extension_t *self, const char *path, axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_extension_peek_manifest_async(
    axis_extension_t *self, const char *path,
    axis_extension_peek_manifest_async_cb_t cb, void *cb_data, axis_error_t *err);
