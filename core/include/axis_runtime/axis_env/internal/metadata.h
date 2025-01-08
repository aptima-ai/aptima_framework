//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/value/value.h"

typedef struct axis_env_t axis_env_t;
typedef struct axis_error_t axis_error_t;

/**
 * @brief Note that the ownership of @a value would be transferred into the
 * TEN runtime, so the caller of this function could _not_ consider the
 * value instance is still valid.
 */
axis_RUNTIME_API bool axis_env_set_property(axis_env_t *self, const char *path,
                                          axis_value_t *value, axis_error_t *err);

axis_RUNTIME_API axis_value_t *axis_env_peek_property(axis_env_t *self,
                                                   const char *path,
                                                   axis_error_t *err);

axis_RUNTIME_API bool axis_env_is_property_exist(axis_env_t *self,
                                               const char *path,
                                               axis_error_t *err);

axis_RUNTIME_API bool axis_env_init_property_from_json(axis_env_t *self,
                                                     const char *json_str,
                                                     axis_error_t *err);
