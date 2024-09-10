//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

typedef struct ten_binding_handle_t ten_binding_handle_t;

TEN_RUNTIME_API void ten_binding_handle_set_me_in_target_lang(
    ten_binding_handle_t *self, void *me_in_target_lang);

TEN_RUNTIME_API void *ten_binding_handle_get_me_in_target_lang(
    ten_binding_handle_t *self);
