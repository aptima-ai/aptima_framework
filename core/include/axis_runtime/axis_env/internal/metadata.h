//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include "aptima_runtime/aptima_env/aptima_env.h"
#include "aptima_utils/value/value.h"

typedef struct aptima_env_t aptima_env_t;
typedef struct aptima_error_t aptima_error_t;

/**
 * @brief Note that the ownership of @a value would be transferred into the
 * TEN runtime, so the caller of this function could _not_ consider the
 * value instance is still valid.
 */
aptima_RUNTIME_API bool aptima_env_set_property(aptima_env_t *self, const char *path,
                                          aptima_value_t *value, aptima_error_t *err);

aptima_RUNTIME_API aptima_value_t *aptima_env_peek_property(aptima_env_t *self,
                                                   const char *path,
                                                   aptima_error_t *err);

aptima_RUNTIME_API bool aptima_env_is_property_exist(aptima_env_t *self,
                                               const char *path,
                                               aptima_error_t *err);

aptima_RUNTIME_API bool aptima_env_init_property_from_json(aptima_env_t *self,
                                                     const char *json_str,
                                                     aptima_error_t *err);
