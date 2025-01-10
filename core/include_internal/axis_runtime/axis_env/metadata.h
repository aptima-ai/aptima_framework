//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/metadata/metadata.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/axis_env/axis_env.h"

typedef enum axis_METADATA_LEVEL {
  axis_METADATA_LEVEL_INVALID,

  axis_METADATA_LEVEL_APP,
  axis_METADATA_LEVEL_EXTENSION_GROUP,
  axis_METADATA_LEVEL_EXTENSION,
  axis_METADATA_LEVEL_ADDON,
} axis_METADATA_LEVEL;

typedef void (*axis_env_peek_property_async_cb_t)(axis_env_t *axis_env,
                                                 axis_value_t *value,
                                                 void *cb_data,
                                                 axis_error_t *err);

typedef void (*axis_env_set_property_async_cb_t)(axis_env_t *axis_env, bool res,
                                                void *cb_data,
                                                axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_METADATA_LEVEL axis_determine_metadata_level(
    axis_ENV_ATTACH_TO attach_to_type, const char **p_name);

axis_RUNTIME_API bool axis_env_init_manifest_from_json(axis_env_t *self,
                                                     const char *json_string,
                                                     axis_error_t *err);

// This function is used to set prop on any threads.
axis_RUNTIME_PRIVATE_API bool axis_env_set_property_async(
    axis_env_t *self, const char *path, axis_value_t *value,
    axis_env_set_property_async_cb_t cb, void *cb_data, axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_env_peek_property_async(
    axis_env_t *self, const char *path, axis_env_peek_property_async_cb_t cb,
    void *cb_data, axis_error_t *err);
