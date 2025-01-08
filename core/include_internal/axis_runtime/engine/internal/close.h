//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/timer/timer.h"

typedef struct axis_engine_t axis_engine_t;
typedef struct axis_extension_context_t axis_extension_context_t;

typedef void (*axis_engine_on_closed_func_t)(axis_engine_t *self,
                                            void *on_closed_data);

axis_RUNTIME_PRIVATE_API void axis_engine_close_async(axis_engine_t *self);

axis_RUNTIME_PRIVATE_API void axis_engine_set_on_closed(
    axis_engine_t *self, axis_engine_on_closed_func_t on_closed,
    void *on_closed_data);

axis_RUNTIME_PRIVATE_API bool axis_engine_is_closing(axis_engine_t *self);

axis_RUNTIME_PRIVATE_API void axis_engine_on_close(axis_engine_t *self);

axis_RUNTIME_PRIVATE_API void axis_engine_on_timer_closed(axis_timer_t *timer,
                                                        void *on_closed_data);

axis_RUNTIME_PRIVATE_API void axis_engine_on_extension_context_closed(
    axis_extension_context_t *extension_context, void *on_closed_data);
