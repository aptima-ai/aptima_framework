//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

typedef struct axis_protocol_t axis_protocol_t;

typedef void (*axis_protocol_on_closed_func_t)(axis_protocol_t *self,
                                              void *on_closed_data);

axis_RUNTIME_PRIVATE_API bool axis_protocol_is_closing(axis_protocol_t *self);

axis_RUNTIME_PRIVATE_API bool axis_protocol_is_closed(axis_protocol_t *self);

axis_RUNTIME_PRIVATE_API void axis_protocol_set_on_closed(
    axis_protocol_t *self, axis_protocol_on_closed_func_t on_closed,
    void *on_closed_data);

axis_RUNTIME_PRIVATE_API void axis_protocol_on_close(axis_protocol_t *self);

axis_RUNTIME_API void axis_protocol_close(axis_protocol_t *self);
