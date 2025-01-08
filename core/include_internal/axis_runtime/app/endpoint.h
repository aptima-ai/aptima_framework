//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

typedef struct axis_app_t axis_app_t;
typedef struct axis_protocol_t axis_protocol_t;

axis_RUNTIME_PRIVATE_API bool axis_app_endpoint_listen(axis_app_t *self);

axis_RUNTIME_PRIVATE_API bool axis_app_is_endpoint_closed(axis_app_t *self);
