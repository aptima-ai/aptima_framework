//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

typedef struct ten_env_t ten_env_t;

TEN_RUNTIME_PRIVATE_API void ten_engine_on_all_extensions_added(void *self_,
                                                                void *arg);

TEN_RUNTIME_PRIVATE_API void ten_engine_on_extension_thread_closed(void *self_,
                                                                   void *arg);

TEN_RUNTIME_PRIVATE_API void ten_engine_on_extension_thread_initted(void *self_,
                                                                    void *arg);

TEN_RUNTIME_PRIVATE_API void ten_engine_on_addon_create_extension_group_done(
    void *self_, void *arg);

TEN_RUNTIME_PRIVATE_API void ten_engine_on_addon_destroy_extension_group_done(
    void *self_, void *arg);
