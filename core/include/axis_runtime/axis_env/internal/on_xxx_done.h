//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include "aptima_utils/lib/error.h"

typedef struct aptima_env_t aptima_env_t;
typedef struct aptima_metadata_info_t aptima_metadata_info_t;

aptima_RUNTIME_API bool aptima_env_on_configure_done(aptima_env_t *self,
                                               aptima_error_t *err);

aptima_RUNTIME_API bool aptima_env_on_init_done(aptima_env_t *self, aptima_error_t *err);

aptima_RUNTIME_API bool aptima_env_on_deinit_done(aptima_env_t *self, aptima_error_t *err);

aptima_RUNTIME_API bool aptima_env_on_start_done(aptima_env_t *self, aptima_error_t *err);

aptima_RUNTIME_API bool aptima_env_on_stop_done(aptima_env_t *self, aptima_error_t *err);

aptima_RUNTIME_API bool aptima_env_on_create_instance_done(aptima_env_t *self,
                                                     void *instance,
                                                     void *context,
                                                     aptima_error_t *err);

aptima_RUNTIME_API bool aptima_env_on_destroy_instance_done(aptima_env_t *self,
                                                      void *context,
                                                      aptima_error_t *err);
