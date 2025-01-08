//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

typedef struct axis_engine_t axis_engine_t;
typedef struct axis_app_t axis_app_t;

axis_RUNTIME_PRIVATE_API void axis_engine_create_its_own_thread(
    axis_engine_t *self);

axis_RUNTIME_PRIVATE_API void axis_engine_init_individual_eventloop_relevant_vars(
    axis_engine_t *self, axis_app_t *app);
