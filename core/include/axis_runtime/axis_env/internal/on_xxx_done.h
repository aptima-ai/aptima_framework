//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/lib/error.h"

typedef struct axis_env_t axis_env_t;
typedef struct axis_metadata_info_t axis_metadata_info_t;

axis_RUNTIME_API bool axis_env_on_configure_done(axis_env_t *self,
                                               axis_error_t *err);

axis_RUNTIME_API bool axis_env_on_init_done(axis_env_t *self, axis_error_t *err);

axis_RUNTIME_API bool axis_env_on_deinit_done(axis_env_t *self, axis_error_t *err);

axis_RUNTIME_API bool axis_env_on_start_done(axis_env_t *self, axis_error_t *err);

axis_RUNTIME_API bool axis_env_on_stop_done(axis_env_t *self, axis_error_t *err);

axis_RUNTIME_API bool axis_env_on_create_instance_done(axis_env_t *self,
                                                     void *instance,
                                                     void *context,
                                                     axis_error_t *err);

axis_RUNTIME_API bool axis_env_on_destroy_instance_done(axis_env_t *self,
                                                      void *context,
                                                      axis_error_t *err);
