//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_runtime/ten_env/ten_env.h"

TEN_RUNTIME_PRIVATE_API void ten_addon_on_init_done(ten_env_t *self);

TEN_RUNTIME_PRIVATE_API void ten_addon_on_deinit_done(ten_env_t *self);

TEN_RUNTIME_PRIVATE_API void ten_addon_on_create_instance_done(ten_env_t *self,
                                                               void *instance,
                                                               void *context);

TEN_RUNTIME_PRIVATE_API void ten_addon_on_destroy_instance_done(ten_env_t *self,
                                                                void *context);
