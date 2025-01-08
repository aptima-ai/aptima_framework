//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "axis_utils/io/transport.h"

axis_UTILS_PRIVATE_API enum axis_TRANSPORT_DROP_TYPE axis_transport_get_drop_type(
    axis_transport_t *self);

axis_UTILS_PRIVATE_API void axis_transport_set_drop_type(
    axis_transport_t *self, axis_TRANSPORT_DROP_TYPE drop_type);

axis_UTILS_PRIVATE_API int axis_transport_drop_required(axis_transport_t *self);

axis_UTILS_PRIVATE_API void axis_transport_set_drop_when_full(
    axis_transport_t *self, int drop);

axis_UTILS_PRIVATE_API void axis_transport_on_close(axis_transport_t *self);
