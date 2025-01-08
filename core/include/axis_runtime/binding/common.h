//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

typedef struct axis_binding_handle_t axis_binding_handle_t;

axis_RUNTIME_API void axis_binding_handle_set_me_in_target_lang(
    axis_binding_handle_t *self, void *me_in_target_lang);

axis_RUNTIME_API void *axis_binding_handle_get_me_in_target_lang(
    axis_binding_handle_t *self);
