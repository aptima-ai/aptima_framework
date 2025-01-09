//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

typedef struct aptima_binding_handle_t aptima_binding_handle_t;

aptima_RUNTIME_API void aptima_binding_handle_set_me_in_target_lang(
    aptima_binding_handle_t *self, void *me_in_target_lang);

aptima_RUNTIME_API void *aptima_binding_handle_get_me_in_target_lang(
    aptima_binding_handle_t *self);
