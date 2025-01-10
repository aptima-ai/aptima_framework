//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/schema_store/store.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/value/value.h"

typedef struct axis_metadata_info_t axis_metadata_info_t;
typedef struct axis_env_t axis_env_t;

typedef void (*axis_object_on_configure_func_t)(axis_env_t *axis_env);

typedef enum axis_METADATA_TYPE {
  axis_METADATA_INVALID,

  axis_METADATA_JSON_FILENAME,
  axis_METADATA_JSON_STR,

  axis_METADATA_LAST,
} axis_METADATA_TYPE;

axis_RUNTIME_API bool axis_metadata_info_set(axis_metadata_info_t *self,
                                           axis_METADATA_TYPE type,
                                           const char *value);

axis_RUNTIME_API void axis_metadata_info_destroy(axis_metadata_info_t *self);

axis_RUNTIME_PRIVATE_API void axis_metadata_load(
    axis_object_on_configure_func_t on_configure, axis_env_t *axis_env);

axis_RUNTIME_PRIVATE_API bool axis_metadata_load_from_info(
    axis_value_t *metadata, axis_metadata_info_t *metadata_info,
    axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_value_t *axis_metadata_init_schema_store(
    axis_value_t *manifest, axis_schema_store_t *schema_store);

axis_RUNTIME_PRIVATE_API bool axis_manifest_json_string_is_valid(
    const char *json_str, axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_manifest_json_file_is_valid(
    const char *json_file, axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_property_json_string_is_valid(
    const char *json_str, axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_property_json_file_is_valid(
    const char *json_file, axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_handle_manifest_info_when_on_configure_done(
    axis_metadata_info_t **self, const char *base_dir, axis_value_t *manifest,
    axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_handle_property_info_when_on_configure_done(
    axis_metadata_info_t **self, const char *base_dir, axis_value_t *property,
    axis_error_t *err);
